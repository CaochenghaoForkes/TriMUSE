#include "physics/thermal/thermal_assembly.hpp"
#include <algorithm>
#include <stdexcept>

namespace trimuse
{
namespace
{

class PointCoefficient final : public mfem::Coefficient // 把预先计算好的积分点系数数组，包装成 MFEM 积分器能调用的系数对象，并在取值时乘上降维几何权重
{
public:
    PointCoefficient(const FemSystem& fem, const std::vector<double>& values) : fem_(fem), values_(values) 
    {
    }

    double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) override
    {
        return values_[fem_.domain_quadrature_offset(transformation.ElementNo) + point.index] * fem_.geometric_weight(transformation, point); // transformation.ElementNo 取得当前积分单元的编号 这里组装使用原网格变换，因此它是原网格域单元编号 + point.index 取得该单元在全域积分点数组中的起始偏移
    }

private:
    const FemSystem& fem_;

    const std::vector<double>& values_; // 全域积分点系数数组 如 k、rho*cp、Q
};

std::shared_ptr<mfem::SparseMatrix> sum_matrices(const std::map<int, std::shared_ptr<const mfem::SparseMatrix>>& parts, int size) // 把各区域或边界已经组装好的矩阵相加，生成一张新的整体矩阵 parts: 区域或边界编号 -> 该部分组装贡献的矩阵 size: 整体矩阵的行列数
{
    auto sum = std::make_shared<mfem::SparseMatrix>(size); // 创建 size × size 的稀疏矩阵，并由 shared_ptr(共享所有权智能指针) 管理
    for (const auto& [region, matrix] : parts) // 遍历各部分矩阵
    {
        const auto* rows = matrix->GetI(); // 每行数据在存储数组中的起止偏移 长度为 size+1
        const auto* columns = matrix->GetJ(); // 每个已存储条目的列编号
        const auto* data = matrix->GetData(); // 每个已存储条目的数值
        // CSR 稀疏矩阵格式
        /*
                        2 0 5
                    A = 0 3 0
                        4 0 6
            data    = {2, 5, 3, 4, 6}; // 已存储条目的数值
            columns = {0, 2, 1, 0, 2}; // 各条目的列编号
            rows    = {0, 2, 3, 5};    // 各行的起始偏移，末项是条目总数
        */
        for (int row = 0; row < size; ++row)
        for (int j = rows[row]; j < rows[row + 1]; ++j) // j 是 CSR 数据数组下标
        { 
            sum->Add(row, columns[j], data[j]); 
        }
    }
    sum->Finalize(); // 将新矩阵整理为 CSR 格式，供后续矩阵运算使用
    return sum;
}

std::shared_ptr<mfem::Vector> sum_vectors(const std::map<int, std::shared_ptr<const mfem::Vector>>& parts, int size) // 将各区域或边界的向量贡献相加，返回一个新的整体向量
{
    auto sum = std::make_shared<mfem::Vector>(size); // 创建长度为 size 的向量，由共享智能指针管理
    *sum = 0.0; // 全部分量置零
    for (const auto& [region, vector] : parts) 
    { 
        *sum += *vector;  // 逐个取出贡献向量，解引用后按分量相加
    }
    return sum;
}

}

ThermalAssembly::ThermalAssembly(FemSystem& fem, ThermalField& field, const std::vector<ThermalDomainConfig>& sources, const std::vector<ThermalBoundaryConfig>& boundaries, const std::vector<ThermalInterfaceConfig>& interfaces) // 初始化各物理贡献管理器，并建立已配置外边界的单元索引表
    : fem_(fem), field_(field), conduction_(fem), capacity_(fem), source_(fem, sources),
      boundary_(boundaries), interface_(fem, field, interfaces) // 分别传入有限元系统、试探场，以及热源、外边界和内部界面的模型配置
{
    const auto& mesh = fem.mesh_data().mfem_mesh(); // 取得原始网格
    for (int zone : boundary_.zones()) // 遍历已配置的边界分区 (不仅包含外边界 还有可能包含显式标记的内部材料界面)
    for (int b = 0; b < mesh.GetNBE(); ++b) // 遍历全部网格边界单元
    {
        int first, second;
        mesh.GetFaceElements(mesh.GetBdrElementFaceIndex(b), &first, &second); // 先取得边界单元 b 对应的原网格面编号 再查找这个两侧的域单元编号 first: 第一侧域单元 second: 第二侧域单元 若小于0表示没有第二侧 即为外部表面
        if (second < 0 && mesh.GetBdrAttribute(b) == zone) // 若是外部表面,且边界属性编号与当前的 zone 一致
        { 
            boundary_elements_[zone].push_back(b); // 形成 如  zone 41 → {边界单元 2, 5, 9}
        }
    }
}

void ThermalAssembly::mark_coupled(PropertiesSystem& properties, bool transient) const // 登记热方程实际用到的属性与温度场之间的耦合关系
{
    conduction_.mark_coupled(properties, "temperature");
    source_.mark_coupled(properties, "temperature");
    boundary_.mark_coupled(properties, "temperature");
    if (transient) 
    { 
        capacity_.mark_coupled(properties, "temperature"); 
    }
}

bool ThermalAssembly::coupled(const PropertiesSystem& properties, bool transient) const // 判断: 当前热方程中，是否存在已登记的温度耦合关系，用于决定是否需要非线性迭代
{
    return conduction_.coupled(properties, "temperature") || source_.coupled(properties, "temperature") // || 连接: 任意一项为 true 整个方法就返回 true
        || boundary_.coupled(properties, "temperature") || (transient && capacity_.coupled(properties, "temperature"));
}

std::shared_ptr<mfem::SparseMatrix> ThermalAssembly::domain_matrix(int region, const std::vector<double>& values, bool diffusion) // 组装指定 region 的域内矩阵贡献：根据 diffusion 生成导热矩阵或热容矩阵 region : 当前要组装的区域编号 values: 已经计算好的全域积分点系数数组(k 或 rho*cp) diffusion: 选择积分类型 (true: 导热项 false: 热容项)
{
    auto result = std::make_shared<mfem::SparseMatrix>(field_.size()); // 创建方形稀疏矩阵，行列数均为温度场的完整自由度数
    PointCoefficient coefficient(fem_, values); // 把积分点数组包装成 MFEM 系数对象 积分器求值时，它返回 当前积分点数 * 降维几何权重 它根据积分器调用 Eval() 时传入的当前单元编号和局部积分点编号确定下标
    std::unique_ptr<mfem::BilinearFormIntegrator> integrator; // 通过公共基类指针保存具体积分器 BilinearForm 双线性形式 通常对应左端矩阵项 指的是分别对试探函数(形函数构成的那个试探温度场) u 和测试函数 v 都是线性的
    if (diffusion)  // 如果是扩散项则对应 DiffusionIntegrator 积分 int k \Delta N_i \Delta N_j dOmega 
    { 
        integrator = std::make_unique<mfem::DiffusionIntegrator>(coefficient); // 组装系数乘形函数梯度内积
    }
    else // 如果是热容项则对应 MassIntegrator 积分 int rhp*cp N_i N_j dOmega
    { 
        integrator = std::make_unique<mfem::MassIntegrator>(coefficient); // 组装系数乘形函数值的乘积
    }
    mfem::IsoparametricTransformation transformation; // 准备每个单元重复使用的临时对象 当前单元的几何映射
    mfem::DenseMatrix local; // 准备每个单元重复使用的临时对象 当前单元的局部矩阵
    mfem::Array<int> dofs; // 准备每个单元重复使用的临时对象 当前单元对应的完整自由度编号
    for (int element : fem_.mesh_data().region_elements(region)) // 遍历当前区域的原网格单元
    {
        // 场的有限元与自由度 -> 通过 ThermalField 取得
        // 单元几何与积分点属性编号 -> 仍采用原网格
        const auto* fe = field_.space(region).GetFE(field_.local_element(element)); // 取得该单元的有限元对象，用来提供形函数 (连续场直接用原单元编号 分域场先转换为该区域子网格中的局部编号)
        fem_.mesh_data().mfem_mesh().GetElementTransformation(element, &transformation); // 从原网格读取这个单元的几何映射，写入 transformation
        integrator->SetIntRule(fem_.domain_quadrature_rule(fe->GetGeomType())); // 根据单元参考几何类型，选择统一的域内求积规则 这保证了 PointCoefficient 使用的积分点编号，与属性数组的编号一致
        integrator->AssembleElementMatrix(*fe, transformation, local); // 真正计算当前单元矩阵，结果写入 local
        field_.element_dofs(element, dofs); // 取得该单元在温度场完整向量中的自由度编号；分域场会包含区域偏移
        result->AddSubMatrix(dofs, dofs, local, 0); // 按照这些整体行列号累加： result(dofs[i], dofs[j]) += local(i, j);
    }
    result->Finalize(); // 将结果整理为 CSR 格式，再返回共享指针
    return result;
}

std::shared_ptr<mfem::Vector> ThermalAssembly::domain_load(int region, const std::vector<double>& values) // 组装指定区域的体积热源向量，对应弱形式右端 int_omega_region Q Ni dOmega
{
    auto result = std::make_shared<mfem::Vector>(field_.size()); *result = 0.0; // 创建长度等于温度场完整自由度数的向量，并将所有分量置零
    PointCoefficient coefficient(fem_, values); // 包装积分点热源系数 积分器调用 Eval() 时，根据原单元编号和积分点编号读取 Q，并乘降维几何权重
    mfem::DomainLFIntegrator integrator(coefficient); // 创建域内线性形式积分器 int_omega_region Q Ni dOmega
    mfem::IsoparametricTransformation transformation; // 临时变量 当前单元的几何变换
    mfem::Vector local; // 临时变量 当前单元的局部热源向量
    mfem::Array<int> dofs; // 临时变量 当前单元在完整场中的自由度编号
    for (int element : fem_.mesh_data().region_elements(region)) // 遍历该区域的域单元
    {
        const auto* fe = field_.space(region).GetFE(field_.local_element(element)); // 取得场所使用的有限元对象；分域场会先将原单元编号转换为子网格局部编号
        fem_.mesh_data().mfem_mesh().GetElementTransformation(element, &transformation); // 从原网格取得该单元的几何映射
        integrator.SetIntRule(fem_.domain_quadrature_rule(fe->GetGeomType())); // 设置与属性缓存一致的域内求积规则
        integrator.AssembleRHSElementVect(*fe, transformation, local); // 计算单元右端向量并写入 local; RHS 表示右端项
        field_.element_dofs(element, dofs); // 取得当前单元的完整自由度编号
        result->AddElementVector(dofs, local); // 累加到整体编号对应的位置
    }
    return result;
}

std::shared_ptr<mfem::SparseMatrix> ThermalAssembly::boundary_matrix(int zone) // 组装指定外边界分区 zone 的矩阵贡献 当前主要对应 Robin 换热边界的左端项 int_gamma_zone h Ni Nj dGamma
{
    auto result = std::make_shared<mfem::SparseMatrix>(field_.size()); // 创建与温度场完整自由度数对应的方形稀疏矩阵，只保存当前 zone 的贡献
    const double h = boundary_.values(zone).matrix; // 读取边界模型返回的矩阵系数: Robin 返回 h Neumann、Dirichlet 返回 0
    if (h != 0.0) // 只有系数非零时，才遍历该 zone 的外边界单元
    for (int b : boundary_elements_.at(zone))
    {
        auto* t = fem_.mesh_data().mfem_mesh().GetBdrFaceTransformations(b); // 根据原网格边界单元编号 b，取得边界面的几何变换，以及与它相邻的域单元变换
        const int element = t->Elem1No; // 取得内侧相邻域单元的原网格编号 外边界只有这一侧
        const int region = fem_.mesh_data().mfem_mesh().GetAttribute(element); // 查询这个相邻域单元所属的区域属性编号
        const auto* fe = field_.space(region).GetFE(field_.local_element(element)); // 取得该相邻域单元的有限元对象 分域场会先映射到对应子网格的局部单元编号
        const auto* rule = fem_.boundary_quadrature_rule(t->GetGeometryType()); // 根据边界面的参考几何类型，取得求积规则
        mfem::Vector shape(fe->GetDof()); // 创建形函数值向量，长度等于相邻域单元的自由度数
        mfem::DenseMatrix local(fe->GetDof()); // 创建并清零该边界单元贡献的局部方阵
        local = 0.0;
        for (int q = 0; q < rule->GetNPoints(); ++q) // 遍历参考面上的积分点
        {
            const auto& ip = rule->IntPoint(q); // 取得参考面点
            t->SetAllIntPoints(&ip); // 将其映射到相邻参考域单元中的对应位置
            fe->CalcShape(t->GetElement1IntPoint(), shape); // 在该参考单元位置计算域形函数值 t->GetElement1IntPoint() 返回参考面点 ip，映射到第一侧参考域单元后得到的坐标
            mfem::AddMult_a_VVt(h * ip.weight * t->Weight() * fem_.geometric_weight(*t, ip), shape, local); // 执行 local += h wq J gq Nq Nq^T 也就是逐项累加 h wq J gq Ni Nj
        }
        mfem::Array<int> dofs; field_.element_dofs(element, dofs); // 取得相邻域单元在完整温度向量中的自由度编号
        result->AddSubMatrix(dofs, dofs, local, 0); // result(dofs[i], dofs[j]) += local(i, j); 将当前边界贡献累加到对应的整体行列位置
    }
    result->Finalize(); // 将矩阵整理为 CSR 格式并返回
    return result;
}

std::shared_ptr<mfem::Vector> ThermalAssembly::boundary_load(int zone) // 组装指定外边界分区 zone 的右端向量贡献 int_gamma_zone value Ni dGamma
{
    auto result = std::make_shared<mfem::Vector>(field_.size()); *result = 0.0; // 向量长度对应整个温度场的完整自由度数
    const double value = boundary_.values(zone).load; // 读取边界模型返回的右端系数: Neumann -qout Robin h*T_infty Dirichlet
    if (value != 0.0) // 只有系数非零时，才遍历该 zone 包含的外边界单元
    for (int b : boundary_elements_.at(zone))
    {
        auto* t = fem_.mesh_data().mfem_mesh().GetBdrFaceTransformations(b); // 取得当前边界面及其内侧域单元的几何变换
        const int element = t->Elem1No; // 相邻域内单元的原网格编号
        const int region = fem_.mesh_data().mfem_mesh().GetAttribute(element); // 所属区域编号
        const auto* fe = field_.space(region).GetFE(field_.local_element(element)); // 取得该单元的有限元对象；分域场会先转换单元编号
        const auto* rule = fem_.boundary_quadrature_rule(t->GetGeometryType()); // 取得当前参考面上的求积规则
        mfem::Vector shape(fe->GetDof()), local(fe->GetDof()); local = 0.0; // 创建两个向量，长度均等于相邻域单元的自由度数 shape 存放当前积分点上的形函数值 local 累积当前边界单元的右端贡献，初始为零
        for (int q = 0; q < rule->GetNPoints(); ++q) // 逐积分点累加贡献
        {
            const auto& ip = rule->IntPoint(q); // 取得参考面上的积分点，并计算它在内侧参考域单元中的对应坐标
            t->SetAllIntPoints(&ip);
            fe->CalcShape(t->GetElement1IntPoint(), shape); // 在该位置计算域单元的所有形函数值
            local.Add(value * ip.weight * t->Weight() * fem_.geometric_weight(*t, ip), shape); // local_i += value * wq * J * gq * Ni
        }
        mfem::Array<int> dofs; field_.element_dofs(element, dofs); // 取得相邻域单元的完整自由度编号
        result->AddElementVector(dofs, local); // (*result)[dofs[i]] += local[i];
    }
    return result;
}

void ThermalAssembly::refresh_matrix(ThermalDomainManager& manager, Cache<mfem::SparseMatrix>& cache, bool diffusion, const std::string& counter) // 负责按需更新域内矩阵缓存：只重组发生变化的区域，再更新这些区域贡献的总和 导热矩阵和热容矩阵共用这套逻辑 cache: 对应缓存 cache.parts: 区域编号->该区域的矩阵贡献 cache.sum: 所有区域贡献之和
{
    bool changed = !cache.sum; // cache.sum 为空：设为 true，本次需要建立总矩阵; 已有总矩阵: 先设为 false，后续有区域重组时再改为 true
    for (int region : manager.regions()) // 逐个检查该管理器负责的区域
    {
        if (cache.parts.count(region) && manager.required_no_change(region)) // 若当前区域已有矩阵缓存 并且该区域系数域最近一次组装时一致则跳过 cache.parts 是 std::map，同一个键只能有一项，所以返回值是 0 表示无缓存 1 表示存在
        { 
            continue; 
        }
        cache.parts[region] = domain_matrix(region, manager.values(), diffusion); // 使用最新积分点系数组装该区域矩阵，并替换缓存指针
        manager.assembled(region); // 通知管理器：这个区域已经完成组装，将对应的 dirty 标记清为 false
        ++counts_[counter]; changed = true; // 将该类贡献的组装次数加一，并记录本次有区域矩阵发生更新
    }
    if (changed) 
    { 
        cache.sum = sum_matrices(cache.parts, field_.size()); // 只要首次建立总矩阵，或者至少一个区域重新组装，就把各区域缓存贡献重新相加 让 cache.sum 持有新矩阵的共享指针，改为指向新的矩阵对象
    }
}

void ThermalAssembly::refresh_source() // 按需组装体积热源向量缓存
{
    bool changed = !source_cache_.sum; // 如果还没有热源总向量，标记为需要首次建立；否则先假设没有变化
    for (int region : source_.regions()) // 遍历配置了热源模型的区域
    {
        if (source_cache_.parts.count(region) && source_.required_no_change(region)) // 如果该区域已有热源向量缓存，且热源系数没有变化，就跳过组装
        { 
            continue; 
        }
        source_cache_.parts[region] = domain_load(region, source_.values()); // 用最新积分点热源值组装 int_Omega_region Q*Ni dOmega
        source_.assembled(region); // 清除该区域的 dirty 标记，表示缓存已与当前热源系数一致
        ++counts_["source"]; changed = true; // 记录一次区域热源向量组装，并标记总向量需要更新
    }
    if (changed) 
    { 
        source_cache_.sum = sum_vectors(source_cache_.parts, field_.size()); // 首次建立或有区域重组时，将各区域贡献相加 让 source_cache_.sum 持有新向量的共享指针，改为指向新的向量对象
    }
}

void ThermalAssembly::refresh_boundary() // 分别更新外边界矩阵和右端向量的缓存 两部分独立判断，因此可以只重组发生变化的部分
{
    bool matrix_changed = !boundary_matrix_cache_.sum, load_changed = !boundary_load_cache_.sum; // 还没有边界总矩阵: matrix_changed=true 还没有边界总向量: load_changed=true
    for (int zone : boundary_.zones()) // 遍历已配置模型的边界分区
    {
        if (boundary_.essential(zone)) // 跳过本质边界
        { 
            continue; 
        }
        if (!boundary_matrix_cache_.parts.count(zone) || !boundary_.required_no_change(zone, BoundaryPart::Matrix)) // 若这个 zone 尚无矩阵缓存 或 矩阵系数发生变化, 或尚未完成组装 就需要组装
        {
            boundary_matrix_cache_.parts[zone] = boundary_matrix(zone); // 组装并保存该 zone 的矩阵贡献，Robin 对应 int_Gamma_zone h * Ni * Nj dGamma
            boundary_.assembled(zone, BoundaryPart::Matrix); // 清除该边界的 Matrix 变化标记，不影响 Load 标记
            ++counts_["boundary_matrix"]; matrix_changed = true; // 记录一次边界矩阵组装，并标记需要重新求和
        }
        if (!boundary_load_cache_.parts.count(zone) || !boundary_.required_no_change(zone, BoundaryPart::Load)) // 同样检查: 是否没有缓存，或者右端系数需要更新
        {
            boundary_load_cache_.parts[zone] = boundary_load(zone); // 依次执行：组装该 zone 的右端贡献、清除 Load 标记、增加统计次数、标记总向量需要更新
            boundary_.assembled(zone, BoundaryPart::Load);
            ++counts_["boundary_load"]; load_changed = true;
        }
    }
    if (matrix_changed) 
    { 
        boundary_matrix_cache_.sum = sum_matrices(boundary_matrix_cache_.parts, field_.size()); 
    }
    if (load_changed) 
    { 
        boundary_load_cache_.sum = sum_vectors(boundary_load_cache_.parts, field_.size()); 
    }
}

ThermalAssembly::State ThermalAssembly::update(const PropertiesFieldData& properties, bool capacity) // 热方程组装层的总入口: 更新系数，按需刷新各部分缓存，再返回当前的 M、K、F
{
    if (capacity) // 本次是否需要更新热容矩阵 (不是热容的值)
    {
        capacity_.update(properties); // 计算最新的 rho*cp 维护区域变化标记
        refresh_matrix(capacity_, mass_cache_, false, "capacity"); // 只重组需要更新的区域，并更新热容总矩阵
    }
    conduction_.update(properties); // 更新导热系数及变化标记
    source_.update(properties); // 更新体积热源及变化标记
    boundary_.update(properties); // 更新外边界系数及变化标记
    refresh_matrix(conduction_, conduction_cache_, true, "conduction"); // 按变化标记刷新域内导热系数矩阵缓存
    refresh_source(); // 按变化标记刷新体积热源向量缓存
    refresh_boundary(); // 按变化标记刷新外边界矩阵和右端向量缓存
    if (last_domain_ != conduction_cache_.sum || last_boundary_ != boundary_matrix_cache_.sum) // 比较共享指针是否指向同一个对象,并非逐项比较矩阵数值 前面的刷新操作只在贡献发生变化时创建新的总矩阵并持有新矩阵的共享指针 所以指针变化意味着需要重新组合
    {
        std::unique_ptr<mfem::SparseMatrix> conduction(mfem::Add(1.0, *conduction_cache_.sum, 1.0, *boundary_matrix_cache_.sum)); // 创建新矩阵 K临时 = K域内 + KRobin mfem::Add() 返回新矩阵指针，由 unique_ptr 接管
        if (const auto* interface_matrix = interface_.matrix()) // 取得界面传热矩阵指针，并判断是否存在热阻贡献 
        {
            state_.conduction.reset(mfem::Add(1.0, *conduction, 1.0, *interface_matrix)); // 存在时相加得: K = K临时 + K界面 reset() 让 state_.conduction 改为持有新矩阵
        }
        else 
        { 
            state_.conduction = std::move(conduction); // 没有界面传热矩阵时，直接将临时矩阵的所有权转交给状态
        }
        last_domain_ = conduction_cache_.sum; last_boundary_ = boundary_matrix_cache_.sum; // 记录本次组合使用的两个缓存指针，供下次比较
    }
    if (last_source_ != source_cache_.sum || last_boundary_load_ != boundary_load_cache_.sum) // 通过指针判断体积源或外边界右端是否更新
    {
        auto load = std::make_shared<mfem::Vector>(*source_cache_.sum); // 先复制体积源向量，再加上外边界贡献
        *load += *boundary_load_cache_.sum; // F = F体积源 + F外边界
        state_.load = std::move(load); // 保存新右端，并记录本次使用的缓存指针
        last_source_ = source_cache_.sum; last_boundary_load_ = boundary_load_cache_.sum;
    }
    state_.mass = mass_cache_.sum; // 共享引用当前热容缓存，不复制矩阵
    return state_; // 只共享不可变的组装结果, 后续更新替换指针, 不改写旧时刻的矩阵。
}

void ThermalAssembly::update_boundary(const PropertiesFieldData& properties) // 更新边界值: 单独提供此接口出要适用于三个场景 1.初始时刻(只需要施加初始时刻的表面温度，不需要执行时间推进、组装整套矩阵) 2.新时间步开始(如果 theta 法先用旧属性计算旧时刻贡献，边界管理器也会被更新成旧值 随后需要单独恢复新时刻的边界值，并先施加新表面温度) 3.显式格式theta=0(域内和自然边界贡献使用旧时刻数据，但指定温度仍施加新时刻的值 因此需要更新边界，却不需要组装新时刻的导热矩阵和热源向量)
{ 
    boundary_.update(properties); 
}

mfem::Vector ThermalAssembly::reduced_values() const // 用于把试探场的完整温度向量，转换成约束后的独立温度向量，供后续求解使用
{
    const auto& p = interface_.prolongation(); // 取得映射 P T_full = P * T_independent
    mfem::Vector full = field_.values(), reduced(p.Width()), ones(p.Height()), weights(p.Width()); // full 当前试探温度的完整向量 reduced 要返回的独立温度向量 ones 辅助的完整空间全一向量 weights 记录每组包含多少个完整自由度 P.Height()：完整自由度数 P.Width() ：独立自由度数
    if (!field_.regional()) // 如果是连续场
    {
        const auto* restriction = field_.space(0).GetConformingRestriction(); // 取得 MFEM 的限制矩阵 R，用于将完整自由度转换为协调约束下的独立自由度
        if (restriction) 
        { 
            restriction->Mult(full, reduced); // 存在约束时计算 T独立 = RT完整
            return reduced; 
        }
        return full;
    }
    if (p.Height() == p.Width()) // 分域场没有自由度合并时直接返回 例如全部界面都是有限热阻时，两侧自由度独立，就属于这种情况
    { 
        return full; 
    }
    ones = 1.0; // 辅助向量全部元素设为1
    p.MultTranspose(full, reduced); // 计算 reduced = P^T full
    p.MultTranspose(ones, weights); // 计算 weights = P^T 1 因此每一项就是对应组的成员数量
    // 原始关系 T_full = Pu
    // P^T T_full = P^TPu
    // u = (P^T P)^{-1} P^T T_full
    // P^T P 是对角阵,对角值就是各组成员数, 所以 reduced 还得除 weights 
    for (int i = 0; i < reduced.Size(); ++i)
    { 
        reduced[i] /= weights[i]; 
    }
    // 例：区域 a、b 各有两个自由度，完整向量按 [a0, a1, b0, b1] 排列。
    // 理想接触要求 T_a1 == T_b0，因此独立向量为 [T_a0, T_interface, T_b1]。
    //
    // P = [ 1  0  0 ]
    //     [ 0  1  0 ]
    //     [ 0  1  0 ]
    //     [ 0  0  1 ]
    //
    // 从独立向量恢复完整向量：
    // u    = [300, 350, 400]^T
    // P*u  = [300, 350, 350, 400]^T
    //
    // 反过来提取独立向量时，P^T 会将同组自由度的温度相加：
    // P^T * full = [300, 700, 400]^T
    //
    // 因为 P^T*P = diag(1, 2, 1)，不能直接把上述结果作为独立温度。
    // 用全一向量计算各组成员数：
    // weights = P^T * ones = [1, 2, 1]^T
    //
    // 再逐项相除，得到组内平均温度：
    // reduced = [300/1, 700/2, 400/1]^T = [300, 350, 400]^T
    //
    // 若完整向量暂为 [300, 340, 360, 400]^T，界面独立温度同样得到：
    // T_interface = (340 + 360) / 2 = 350。
    // 整个过程对应 reduced = (P^T*P)^(-1) * P^T * full
    return reduced;
}

void ThermalAssembly::essential_values(std::map<int, double>& values) const // 收集 Dirichlet 边界的指定温度，并把边界自由度编号转换为约束后独立空间中的编号
{
    const auto& p = interface_.prolongation(); // 取得完整空间到独立空间的映射关系
    for (int zone : boundary_.zones())
    {
        if (!boundary_.essential(zone)) // 只处理本质边界
        { 
            continue; 
        } 
        for (int b : boundary_elements_.at(zone)) // 遍历该 zone 包含的原网格外边界单元
        {
            const auto& mesh = fem_.mesh_data().mfem_mesh(); // 原网格
            int first, second;
            const int face = mesh.GetBdrElementFaceIndex(b); // 边界单元 b 对应的面编号
            mesh.GetFaceElements(face, &first, &second); // 这个面两侧的域单元编号
            mfem::Array<int> dofs; field_.face_dofs(mesh.GetAttribute(first), face, dofs); // 取得边界面上的完整自由度编号
            for (int dof : dofs) // 遍历边界面的完整自由度编号，也就是 P 的行号
            for (int j = p.GetI()[dof]; j < p.GetI()[dof + 1]; ++j) // 遍历该行已存储条目的数组下标 j
            {
                if (p.GetData()[j] != 0.0) // GetJ()[j] 是列号，即与当前完整自由度关联的独立自由度编号
                { 
                    values[p.GetJ()[j]] = boundary_.values(zone).temperature; // 将该独立自由度的指定温度写入 values
                }
            }
        }
    }
}

void ThermalAssembly::apply_initial_constraints() // 在正式求解前，让试探温度满足自由度连续约束和当前 Dirichlet 指定温度
{
    auto temperature = reduced_values(); // 将当前完整试探温度转换为独立自由度向量
    std::map<int, double> essential; 
    essential_values(essential); // 收集指定温度条件 独立自由度编号 -> 指定温度
    for (const auto& [dof, value] : essential) 
    { 
        temperature[dof] = value; 
    }
    recover(temperature); // 传入独立自由度温度向量 把独立自由度温度恢复成完整温度向量，并写入试探场 
    for (int zone : boundary_.zones()) 
    { 
        boundary_.assembled(zone, BoundaryPart::Essential); // 清除各 zone 的指定温度变化标记，表示这一部分已处理
    }
}

void ThermalAssembly::form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs, mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& temperature) const // 把完整自由度上的方程，转换成独立自由度系统，并施加 Dirichlet 温度约束 输入 AT=b 输出 system(最终系统矩阵)、load(最终右端)、temperature(独立自由度空间中的温度初始猜测); matrix 与 rhs: 已经组合好的完整矩阵与右端 
{
    const auto& p = interface_.prolongation(); // 取得约束映射 P T=Pu
    if (p.Height() == p.Width()) // 没有自由度约化时直接复制
    {
        system = matrix;
        load = rhs;
    }
    else // 有约束时转换到独立空间 约化方程：把相互约束的自由度对应的平衡方程合并；约化温度初始值：给同组自由度选出一个代表温度
    {
        // 约束要求 T = Pu
        // 但加入约束后，原来各个自由度的方程不再独立 不能只代入后，仍要求所有完整空间方程分别成立；理想接触会在两侧之间产生内部交换
        // 从弱形式看，测试函数也必须满足相同的连续约束。若独立测试系数为 z，完整测试系数就是 w = Pz
        // 带入离散弱形式 w^T(AT-B) = 0
        // 得到 z^TP^T(APu-b) = 0
        // 因为是对任意的 z 都成立 索引 P^T A P u = P^T b
        std::unique_ptr<mfem::SparseMatrix> reduced(mfem::RAP(p, matrix, p)); // 该方法计算 A独立 = P^T A P
        system = *reduced; // 将约化矩阵保存到输出 system
        load.SetSize(p.Width()); p.MultTranspose(rhs, load); // 右端转换为 b独立 = P^T b 这里右端不除以组成员数，因为约化方程需要将同组自由度的方程贡献相加，与前面构造温度初始猜测时的组内平均不同
    }
    temperature = reduced_values(); // 将当前完整试探温度转换为独立自由度向量
    std::map<int, double> essential; essential_values(essential); // 收集指定温度条件 独立自由度编号 → 指定温度
    for (const auto& [dof, value] : essential) // 遍历所有 Dirichlet 约束
    {
        temperature[dof] = value; // 先让求解初始猜测满足指定温度
        system.EliminateRowCol(dof, value, load); // 将 udof = value 施加到系统中 从其它行中消去该列 将已知温度移动到右端 将对应行改成固定该温度的方程 同时处理行与列，保持矩阵对称性 Dirichlet 消元后，这个自由度仍保留在向量里，只是其温度已经固定
    }
}

void ThermalAssembly::recover(const mfem::Vector& temperature) // 传入独立自由度温度向量 把独立自由度温度恢复成完整温度向量，并写入试探场 
{
    mfem::Vector full(field_.size()); // 创建长度等于完整自由度数的向量
    interface_.prolongation().Mult(temperature, full); // 执行矩阵乘法 T完整 = P T独立
    field_.set_values(full); // 将完整向量写入试探场 连续场：直接复制到试探 GridFunction 分域场：按各区域偏移拆分，写入各区域试探 GridFunction
}

void ThermalAssembly::project_initial()
{
    const auto& p = interface_.prolongation();
    auto guess = reduced_values();
    mfem::Vector original = field_.values(), projected(field_.size());
    p.Mult(guess, projected); projected -= original;
    if (projected.Normlinf() <= 1e-12 * std::max(1.0, original.Normlinf())) { return; }
    std::map<int, std::shared_ptr<const mfem::SparseMatrix>> parts;
    std::vector<double> unit(fem_.domain_quadrature_point_count(), 1.0);
    for (int region : fem_.mesh_data().region_attributes()) { parts[region] = domain_matrix(region, unit, false); }
    auto mass = sum_matrices(parts, field_.size());
    mfem::Vector full_rhs(field_.size()), rhs(p.Width());
    mass->Mult(original, full_rhs); p.MultTranspose(full_rhs, rhs);
    std::unique_ptr<mfem::SparseMatrix> system(mfem::RAP(p, *mass, p));
    mfem::GSSmoother preconditioner(*system);
    mfem::CGSolver solver; solver.SetOperator(*system); solver.SetPreconditioner(preconditioner);
    solver.SetRelTol(1e-12); solver.SetAbsTol(0.0); solver.SetMaxIter(1000); solver.SetPrintLevel(0);
    solver.Mult(rhs, guess);
    if (!solver.GetConverged()) { throw std::runtime_error("界面连续约束下的初温投影未收敛"); }
    recover(guess);
}

}
