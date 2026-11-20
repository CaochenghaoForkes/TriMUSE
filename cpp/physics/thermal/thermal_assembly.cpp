#include "physics/thermal/thermal_assembly.hpp"
#include "physics/common/assembly/assembly_utils.hpp"
#include <algorithm>
#include <stdexcept>

namespace trimuse
{

ThermalAssembly::ThermalAssembly(FemSystem& fem, TrialField& trial_field, const ThermalRegionConfigs& regions, const ThermalBoundaryConfigs& boundaries, const ThermalInterfaceConfigs& interfaces) // 初始化各物理贡献管理器，并建立已配置外边界的单元索引表
    :
    fem_(fem),
    trial_field_(trial_field),
    integrator_(fem, trial_field),
    constraints_(fem, trial_field),
    conduction_manager_(fem, regions.at("conduction")),
    capacity_manager_(fem, regions.at("capacity")),
    source_manager_(fem, regions.at("source")),
    boundary_manager_(boundaries.at("heat_boundary")),
    interface_heat_transfer_manager_(fem, trial_field, constraints_, interfaces.at("interface_heat_transfer")) // 分别传入有限元系统、试探场，以及热源、外边界和内部界面的模型配置
{
    constraints_.finalize(); // 全部界面相等关系已登记，构建 P，供后续初值投影、约化求解及恢复共用
    const auto& mesh = fem.mesh_data().mfem_mesh(); // 取得原始网格
    for (int zone : boundary_manager_.zones())
    {
        boundary_elements_[zone] = {};
    }
    for (int zone : boundary_manager_.zones()) // 遍历已配置的边界分区 (不仅包含外边界 还有可能包含显式标记的内部材料界面)
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
    conduction_manager_.mark_coupled(properties, "temperature");
    source_manager_.mark_coupled(properties, "temperature");
    boundary_manager_.mark_coupled(properties, "temperature");
    if (transient)
    {
        capacity_manager_.mark_coupled(properties, "temperature");
    }
}

bool ThermalAssembly::coupled(const PropertiesSystem& properties, bool transient) const // 判断: 当前热方程中，是否存在已登记的温度耦合关系，用于决定是否需要非线性迭代
{
    return conduction_manager_.coupled(properties, "temperature") || source_manager_.coupled(properties, "temperature") || boundary_manager_.coupled(properties, "temperature") || (transient && capacity_manager_.coupled(properties, "temperature")); // || 连接: 任意一项为 true 整个方法就返回 true
}

void ThermalAssembly::refresh_conduction(const PropertiesFieldData::DomainIntegrationPointValues& weights) // 按区域变化标记刷新conduction矩阵
{
    bool changed = !conduction_matrix_cache_.sum;
    for (int region : conduction_manager_.regions())
    {
        if (conduction_matrix_cache_.parts.count(region) && conduction_manager_.required_no_change(region))
        {
            continue;
        }
        conduction_matrix_cache_.parts[region] = integrator_.domain_matrix(region, conduction_manager_.values(), ScalarIntegrator::MatrixForm::Gradient, weights);
        conduction_manager_.assembled(region);
        ++counts_["conduction"];
        changed = true;
    }
    if (changed)
    {
        conduction_matrix_cache_.sum = sum_matrices(conduction_matrix_cache_.parts, trial_field_.size());
    }
}

void ThermalAssembly::refresh_capacity(const PropertiesFieldData::DomainIntegrationPointValues& weights) // 按区域变化标记刷新capacity矩阵
{
    bool changed = !capacity_matrix_cache_.sum;
    for (int region : capacity_manager_.regions())
    {
        if (capacity_matrix_cache_.parts.count(region) && capacity_manager_.required_no_change(region))
        {
            continue;
        }
        capacity_matrix_cache_.parts[region] = integrator_.domain_matrix(region, capacity_manager_.values(), ScalarIntegrator::MatrixForm::Mass, weights);
        capacity_manager_.assembled(region);
        ++counts_["capacity"];
        changed = true;
    }
    if (changed)
    {
        capacity_matrix_cache_.sum = sum_matrices(capacity_matrix_cache_.parts, trial_field_.size());
    }
}

void ThermalAssembly::refresh_source(const PropertiesFieldData::DomainIntegrationPointValues& weights) // 根据 source_manager 各区的 dirty 决定更新哪些区的源右端缓存，若有区域被更新还会重新求和得到总源右端，注意这里 cache.parts 和 cache.sum 存储的是共享指针，一旦发生更新，对应的指针会被替换 cache: 对应缓存 cache.parts: 区域编号->该区域的源右端贡献 cache.sum: 所有区域贡献之和
{
    bool changed = !source_load_cache_.sum; // 如果还没有热源总向量，标记为需要首次建立；否则先假设没有变化
    for (int region : source_manager_.regions()) // 遍历配置了热源模型的区域
    {
        if (source_load_cache_.parts.count(region) && source_manager_.required_no_change(region)) // 如果该区域已有热源向量缓存，且热源系数没有变化(dirty为false)，就跳过组装
        {
            continue;
        }
        source_load_cache_.parts[region] = integrator_.domain_load(region, source_manager_.values(), weights); // 用最新积分点热源值组装 int_Omega_region Q*Ni dOmega
        source_manager_.assembled(region); // 清除该区域的 dirty 标记，表示缓存已与当前热源系数一致
        ++counts_["source"];
        changed = true; // 记录一次区域热源向量组装，并标记总向量需要更新
    }
    if (changed)
    {
        source_load_cache_.sum = sum_vectors(source_load_cache_.parts, trial_field_.size()); // 首次建立或有区域重组时，将各区域贡献相加 让 source_load_cache_.sum 持有新向量的共享指针，改为指向新的向量对象
    }
}

void ThermalAssembly::refresh_boundary(const PropertiesFieldData::FaceIntegrationPointValues& weights) // 根据 boundary_manager_ 各 zone 的 dirty 分别更新外边界矩阵和右端向量的缓存 两部分独立判断，因此可以只重组发生变化的部分
{
    bool matrix_changed = !boundary_matrix_cache_.sum, load_changed = !boundary_load_cache_.sum; // 还没有边界总矩阵: matrix_changed=true 还没有边界总向量: load_changed=true
    for (int zone : boundary_manager_.zones()) // 遍历已配置模型的边界分区
    {
        if (boundary_manager_.essential(zone)) // 跳过本质边界
        {
            continue;
        }
        if (!boundary_matrix_cache_.parts.count(zone) || !boundary_manager_.required_no_change(zone, BoundaryPart::Matrix)) // 若这个 zone 尚无矩阵缓存 或 矩阵系数发生变化(dirty为true), 或尚未完成组装 就需要组装
        {
            boundary_matrix_cache_.parts[zone] = integrator_.boundary_matrix(boundary_elements_.at(zone), boundary_manager_.values(zone).matrix, weights); // 组装并保存该 zone 的矩阵贡献，Robin 对应 int_Gamma_zone h * Ni * Nj dGamma
            boundary_manager_.assembled(zone, BoundaryPart::Matrix); // 清除该边界的 Matrix 变化标记，不影响 Load 标记
            ++counts_["boundary_matrix"];
            matrix_changed = true; // 记录一次边界矩阵组装，并标记需要重新求和
        }
        if (!boundary_load_cache_.parts.count(zone) || !boundary_manager_.required_no_change(zone, BoundaryPart::Load)) // 同样检查: 是否没有缓存，或者右端系数需要更新
        {
            boundary_load_cache_.parts[zone] = integrator_.boundary_load(boundary_elements_.at(zone), boundary_manager_.values(zone).load, weights); // 依次执行：组装该 zone 的右端贡献、清除 Load 标记、增加统计次数、标记总向量需要更新
            boundary_manager_.assembled(zone, BoundaryPart::Load);
            ++counts_["boundary_load"];
            load_changed = true;
        }
    }
    if (matrix_changed)
    {
        boundary_matrix_cache_.sum = sum_matrices(boundary_matrix_cache_.parts, trial_field_.size());
    }
    if (load_changed)
    {
        boundary_load_cache_.sum = sum_vectors(boundary_load_cache_.parts, trial_field_.size());
    }
}

void ThermalAssembly::refresh_interface_heat_transfer(const PropertiesFieldData::FaceIntegrationPointValues& weights)
{
    bool changed = false;
    for (int attribute : interface_heat_transfer_manager_.interface_heat_transfer_attributes())
    {
        if (interface_heat_transfer_matrix_cache_.parts.count(attribute) && interface_heat_transfer_manager_.required_no_change(attribute))
        {
            continue;
        }
        interface_heat_transfer_matrix_cache_.parts[attribute] = interface_heat_transfer_manager_.assemble_matrix(attribute, fem_, trial_field_, constraints_, weights);
        interface_heat_transfer_manager_.assembled(attribute);
        ++counts_["interface_heat_transfer_matrix"];
        changed = true;
    }
    if (changed)
    {
        interface_heat_transfer_matrix_cache_.sum = sum_matrices(interface_heat_transfer_matrix_cache_.parts, trial_field_.size());
    }
}

ThermalAssembly::State ThermalAssembly::update(const PropertiesFieldData& properties, bool capacity) // 热方程组装层的总入口: 更新系数，按需刷新各部分缓存，再返回当前的 M、K、F
{
    const auto& weights = properties.domain_integration_point_values.at("precomputed.initial_domain_quadrature_weight"); // 固定参考网格上的域积分权重
    const auto& face_weights = properties.face_integration_point_values.at("precomputed.initial_face_quadrature_weight"); // 固定参考网格上的面求积点权重
    if (capacity) // 本次是否需要更新热容矩阵 (不是热容的值)
    {
        capacity_manager_.update(properties); // 计算最新的 rho*cp 维护区域变化标记 dirty
        refresh_capacity(weights); // 只重组需要更新的区域，并更新热容总矩阵
    }
    conduction_manager_.update(properties); // 更新导热系数及变化标记
    source_manager_.update(properties); // 更新体积热源及变化标记
    boundary_manager_.update(properties); // 更新外边界系数及变化标记
    interface_heat_transfer_manager_.update(properties); // 更新面片热阻及各界面的变化标记
    refresh_conduction(weights); // 按变化标记刷新域内导热系数矩阵缓存
    refresh_source(weights); // 按变化标记刷新体积热源向量缓存
    refresh_boundary(face_weights); // 按变化标记刷新外边界矩阵和右端向量缓存
    refresh_interface_heat_transfer(face_weights); // 按变化标记刷新界面矩阵缓存
    if (last_conduction_matrix_cache_sum_ != conduction_matrix_cache_.sum || last_boundary_matrix_cache_sum_ != boundary_matrix_cache_.sum || last_interface_heat_transfer_matrix_cache_sum_ != interface_heat_transfer_matrix_cache_.sum)
    {
        std::unique_ptr<mfem::SparseMatrix> conduction(mfem::Add(1.0, *conduction_matrix_cache_.sum, 1.0, *boundary_matrix_cache_.sum));
        if (interface_heat_transfer_matrix_cache_.sum)
        {
            state_.matrix.reset(mfem::Add(1.0, *conduction, 1.0, *interface_heat_transfer_matrix_cache_.sum));
        }
        else
        {
            state_.matrix = std::move(conduction);
        }
        last_conduction_matrix_cache_sum_ = conduction_matrix_cache_.sum;
        last_boundary_matrix_cache_sum_ = boundary_matrix_cache_.sum;
        last_interface_heat_transfer_matrix_cache_sum_ = interface_heat_transfer_matrix_cache_.sum;
    }
    if (last_source_load_cache_sum_ != source_load_cache_.sum || last_boundary_load_cache_sum_ != boundary_load_cache_.sum) // 通过指针判断体积源或外边界右端是否更新
    {
        auto load = std::make_shared<mfem::Vector>(*source_load_cache_.sum); // 先复制体积源向量，再加上外边界贡献
        *load += *boundary_load_cache_.sum; // F = F体积源 + F外边界
        state_.load = std::move(load); // 保存新右端，并记录本次使用的缓存指针
        last_source_load_cache_sum_ = source_load_cache_.sum;
        last_boundary_load_cache_sum_ = boundary_load_cache_.sum;
    }
    state_.mass = capacity_matrix_cache_.sum; // 共享引用当前热容缓存，不复制矩阵
    return state_; // 只共享不可变的组装结果, 后续更新替换指针, 不改写旧时刻的矩阵。
}

void ThermalAssembly::update_boundary(const PropertiesFieldData& properties) // 更新边界管理器的值: 单独提供此接口出要适用于三个场景 1.初始时刻(只需要施加初始时刻的表面温度，不需要执行时间推进、组装整套矩阵) 2.新时间步开始(如果 theta 法先用旧属性计算旧时刻贡献，边界管理器也会被更新成旧值 随后需要单独恢复新时刻的边界值，并先施加新表面温度) 3.显式格式theta=0(域内和自然边界贡献使用旧时刻数据，但指定温度仍施加新时刻的值 因此需要更新边界，却不需要组装新时刻的导热矩阵和热源向量)
{
    boundary_manager_.update(properties);
}

void ThermalAssembly::essential_values(std::map<int, double>& values) const // 收集 Dirichlet 边界的指定温度，并把边界自由度编号转换为约束后独立空间中的编号
{
    const auto& p = constraints_.prolongation(); // 取得完整空间到独立空间的映射关系
    for (int zone : boundary_manager_.zones())
    {
        if (!boundary_manager_.essential(zone)) // 只处理本质边界
        {
            continue;
        }
        for (int b : boundary_elements_.at(zone)) // 遍历该 zone 包含的原网格外边界单元
        {
            const auto& mesh = fem_.mesh_data().mfem_mesh(); // 原网格
            int first, second;
            const int face = mesh.GetBdrElementFaceIndex(b); // 边界单元 b 对应的面编号
            mesh.GetFaceElements(face, &first, &second); // 这个面两侧的域单元编号
            mfem::Array<int> dofs;
            trial_field_.face_dofs(mesh.GetAttribute(first), face, dofs); // 取得边界面上的完整自由度编号
            for (int dof : dofs) // 遍历边界面的完整自由度编号，也就是 P 的行号
                for (int j = p.GetI()[dof]; j < p.GetI()[dof + 1]; ++j) // 遍历该行已存储条目的数组下标 j
                {
                    if (p.GetData()[j] != 0.0) // GetJ()[j] 是列号，即与当前完整自由度关联的独立自由度编号
                    {
                        values[p.GetJ()[j]] = boundary_manager_.values(zone).temperature; // 将该独立自由度的指定温度写入 values
                    }
                }
        }
    }
}

void ThermalAssembly::apply_initial_constraints() // 在正式求解前，让试探温度满足自由度连续约束和当前 Dirichlet 指定温度
{
    auto solution = constraints_.reduced_values(); // 将当前完整试探温度转换为独立自由度向量
    std::map<int, double> essential;
    essential_values(essential); // 收集指定温度条件 独立自由度编号 -> 指定温度
    for (const auto& [dof, value] : essential) // 把指定温度写入当前的独立自由度温度向量
    {
        solution[dof] = value;
    }
    recover(solution); // 传入独立自由度温度向量 把独立自由度温度恢复成完整温度向量，并写入试探场
    for (int zone : boundary_manager_.zones())
    {
        boundary_manager_.assembled(zone, BoundaryPart::Essential); // 清除各 zone 的指定温度变化标记，表示这一部分已处理
    }
}

void ThermalAssembly::form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& load, mfem::SparseMatrix& system, mfem::Vector& system_load, mfem::Vector& solution) const // 收集一类条件，通过映射 P 整理成独立自由度下的一类条件，然后把完整自由度上的方程，通过 P^TAPu=P^Tb 约化成独立自由度系统，再对独立自由度系统上的 Dirichlet 条件执行消元
{
    std::map<int, double> essential;
    essential_values(essential); // 收集指定温度条件：独立自由度编号 → 指定温度
    constraints_.form_system(matrix, load, essential, system, system_load, solution);
}

void ThermalAssembly::recover(const mfem::Vector& solution) // 传入独立自由度温度向量，把独立自由度温度恢复成完整温度向量，并写入试探场
{
    const auto full = constraints_.recover(solution);
    trial_field_.set_values(full); // 将完整向量写入试探场 连续场：直接复制到试探 GridFunction 分域场：按各区域偏移拆分，写入各区域试探 GridFunction
}

void ThermalAssembly::project_initial(const PropertiesFieldData::DomainIntegrationPointValues& weights) // 把当前初温投影到满足界面连续约束的空间中
{
    // (P^T_original M P)u = P^T_original M T_original (M 是带几何权重的单位质量矩阵 M_ij = int_Omega g N_i N_j dOmega)
    const auto& p = constraints_.prolongation(); // Pu=T_original
    auto solution = constraints_.reduced_values(); // 根据当前完整温度生成独立自由度上的初始猜测
    mfem::Vector original = trial_field_.values(), projected(trial_field_.size());
    p.Mult(solution, projected);
    projected -= original; // projected = P solution - T_original (projected 保存的是恢复温度与原始温度的差)
    if (projected.Normlinf() <= 1e-12 * std::max(1.0, original.Normlinf()))
    {
        return; // 若这个差足够小，说明原始温度已经满足约束，直接返回
    }
    std::map<int, std::shared_ptr<const mfem::SparseMatrix>> parts; // 准备各区域的矩阵贡献
    std::vector<double> unit(fem_.domain_quadrature_point_count(), 1.0); // 以及所有积分点都为 1 的系数数组
    for (int region : fem_.mesh_data().region_attributes())
    {
        parts[region] = integrator_.domain_matrix(region, unit, ScalarIntegrator::MatrixForm::Mass, weights); // 采用质量型积分，逐区域组装后相加 M_ij = int_Omega g N_i N_j dOmega
    }
    auto mass = sum_matrices(parts, trial_field_.size()); // 得到 M
    mfem::Vector load(trial_field_.size()), system_load(p.Width());
    mass->Mult(original, load);
    p.MultTranspose(load, system_load); // load = M T_original system_load = P^T M T_original
    std::unique_ptr<mfem::SparseMatrix> system(mfem::RAP(p, *mass, p)); // system = P^T M P
    mfem::GSSmoother preconditioner(*system);
    mfem::CGSolver solver;
    solver.SetOperator(*system);
    solver.SetPreconditioner(preconditioner);
    solver.SetRelTol(1e-12);
    solver.SetAbsTol(0.0);
    solver.SetMaxIter(1000);
    solver.SetPrintLevel(0);
    solver.Mult(system_load, solution); // 求解 (P^T M P)u = P^T M T_original
    if (!solver.GetConverged())
    {
        throw std::runtime_error("界面连续约束下的初温投影未收敛");
    }
    recover(solution);
}

}
