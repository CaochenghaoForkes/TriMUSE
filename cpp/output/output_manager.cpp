#include "output/output_manager.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace trimuse
{

OutputManager::OutputManager(FemSystem& fem_system, const std::string& output_directory)
    :
    fem_system_(fem_system),
    output_directory_(output_directory)
{
}

void OutputManager::add_field(const std::string& field_name, const std::string& directory_name, bool project_to_l2, int order) // 为一个场建立输出目录，建立 L2 输出空间，并注册到 ParaView 输出对象中
{
    FieldOutput field_output; // 创建一组输出资源
    if (!project_to_l2) // 正式场沿用计算阶次；积分点数组使用传入的输出阶次
    {
        order = fem_system_.region_field(field_name).finite_element_space(fem_system_.mesh_data().region_attributes().front()).order();
    }
    field_output.collection = std::make_unique<mfem::L2_FECollection>(order, fem_system_.mesh_data().dimension());
    field_output.space = std::make_unique<mfem::FiniteElementSpace>(&fem_system_.mesh_data().mfem_mesh(), field_output.collection.get()); // 在原网格建立标量 L2 输出空间，保留材料界面两侧的跳变
    field_output.values = std::make_unique<mfem::GridFunction>(field_output.space.get());
    const auto directory = std::filesystem::path(output_directory_) / directory_name; // 拼接该场的输出目录 如 output_directory_="/结果" directory_name="thermal" directory="/结果/thermal"
    std::filesystem::create_directories(directory); // 创建该场的输出目录
    auto output = std::make_unique<mfem::ParaViewDataCollection>(field_name, &fem_system_.mesh_data().mfem_mesh()); // 指定数据集合名称及其使用的原网格
    output->SetOwnData(false); // 表示输出对象不负责销毁登记的网格和场值
    output->SetPrefixPath(directory.string()); // 设置输出目录
    output->RegisterField(field_name, field_output.values.get()); // 把场名和场值对应起来
    const auto* nodes = fem_system_.mesh_data().mfem_mesh().GetNodes();
    const int geometry_order = nodes ? nodes->FESpace()->GetMaxElementOrder() : 1; // 取得几何映射阶次
    output->SetLevelsOfDetail(std::max(geometry_order, order)); // 配置输出细节级别
    output->SetHighOrderOutput(true); // 启用高阶 VTK 输出以表示曲边几何和高阶场
    output->SetDataFormat(mfem::VTKFormat::BINARY); // 指定二进制输出格式
    output->SetPrecision(16); // 设置输出精度参数
    field_output.collection_output = std::move(output); // 转交资源
    field_outputs_[field_name] = std::move(field_output); // 转交资源
    solve_information_.try_emplace(field_name); // 如果没有键就对 field_name 插入个空的 SolveInformation 否则什么也不做
}

void OutputManager::save(const std::string& field_name, const FieldView& values, int cycle, double time, double time_step) // 对于传入的场view，将输入分域场插值到 L2 输出场，并输出场 ParaView 文件
{
    update_field_output(values, *field_outputs_.at(field_name).values); // 将传入的正式场逐单元插值到输出节点，分域场按原网格单元读取
    write_field(field_name, cycle, time, time_step); // 输出场 ParaView
}

void OutputManager::write_field(const std::string& field_name, int cycle, double time, double time_step) // 把指定场在当前时间步的结果保存成 ParaView 文件
{
    auto& field_output = field_outputs_.at(field_name); // 根据场名找到对应的输出资源
    mfem::ParaViewDataCollection& output = *field_output.collection_output; // 取得该场的 ParaView 输出对象引用
    output.SetCycle(cycle); // 设置本次输出的编号，用于区分各帧
    output.SetTime(time); // 设置当前物理时间
    output.SetTimeStep(time_step); // 设置当前物理时间步长
    output.Save(); // 实际写入结果文件，并更新 ParaView 数据集合中的时间序列信息
    if (output.Error() != mfem::DataCollection::No_Error)
    {
        throw std::runtime_error("场 ParaView 输出失败: " + field_name);
    }
}

void OutputManager::save(const std::string& field_name, const std::vector<double>& values, const PropertiesFieldData::DomainIntegrationPointValues& weights, int cycle, double time, double time_step) // 对于传入全域求积点上的标量值数组，把它逐单元投影到已经创建好的 L2 输出场中
{
    // H1 连续有限元空间在相邻单元的公共界面上共享自由度，各单元的贡献会累加到相同的总体自由度上
    // 此处的不连续 L2 有限元空间中，各单元拥有独立的自由度，同一几何位置在不同单元中可以具有不同的值
    // 因此本投影的总体质量矩阵按单元分块对角，可以逐单元求解并直接写入输出场，保留材料界面两侧的跳变
    // 数学上 L2 要求函数平方可积，H1 还要求一阶弱导数平方可积；L2 允许不连续，但并不要求一定不连续
    auto& output = *field_outputs_.at(field_name).values;
    auto* space = output.FESpace();
    const auto& mesh = fem_system_.mesh_data().mfem_mesh();
    mfem::Array<int> dofs;
    mfem::DenseMatrix mass;
    mfem::Vector shape, rhs, local;
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        // 使得连续形式的 L2 投影的误差最小
        // 误差 = 1/2 int_{Omega_e} (\sum_{1}^{n_e} Nj^e*uj^e - v)^2 dV
        // 于是误差对自由度求导
        // \sum_{j=1}^{n_e} (int_{Omega_e} NiNj dV) uj = int_{Omega_e} Ni^e * v dV
        const auto* fe = space->GetFE(element);
        const int count = fe->GetDof(); // 插值节点数
        shape.SetSize(count);
        rhs.SetSize(count);
        local.SetSize(count);
        mass.SetSize(count);
        mass = 0.0;
        rhs = 0.0;
        const auto* rule = fem_system_.domain_quadrature_rule(fe->GetGeomType()); // 取得积分规则
        const auto offset = fem_system_.domain_quadrature_offset(element); // 取得全域积分点偏移
        for (int q = 0; q < rule->GetNPoints(); ++q) // 遍历积分点
        {
            const auto& point = rule->IntPoint(q); // 取得当前积分点位置
            fe->CalcShape(point, shape); // 计算当前位置的全部形函数值
            const double weight = std::get<double>(weights[offset + q].value());
            mfem::AddMult_a_VVt(weight, shape, mass); // Mij = sum_q(w(q)*Ni(q)*Nj(q))
            rhs.Add(weight * values[offset + q], shape); // b = sum_q(w(q)*values(q)*Ni(q))
        }
        mfem::DenseMatrixInverse inverse(mass); // M^{-1}
        inverse.Mult(rhs, local); // M^{-1}b 单元内 L2 投影，材料界面两侧不共享输出自由度
        space->GetElementDofs(element, dofs);
        output.SetSubVector(dofs, local);
    }
    write_field(field_name, cycle, time, time_step);
}

void OutputManager::update_field_output(const FieldView& view, mfem::GridFunction& output) // 将传入的正式场逐单元插值到输出节点，分域场按原网格单元读取 输入场的节点场值 -> 输出场的节点场值
{
    // 根据输入场 view -> 确定输出场的节点场值
    // 计算 u_i^{out} = u_h^{in}(x_j^{out}) = sum_{i}(Ni^{in}(x_j^{out})u_i^{in})
    // 输出空间在 add_field 中建立，各单元的输出自由度独立
    auto* space = output.FESpace(); // 取得输出场的有限元空间，并准备两个临时容器
    mfem::Array<int> dofs; // 当前单元在输出场中的自由度编号
    mfem::Vector values; // 这些自由度对应的场值
    for (int element = 0; element < space->GetNE(); ++element) // 遍历输出空间所在原网格的全部域单元
    {
        const auto& points = space->GetFE(element)->GetNodes(); // 取得当前输出有限元的参考插值节点 些点用于插值，不是前面材料属性计算使用的求积点
        space->GetElementDofs(element, dofs); // 取得当前单元的标量输出自由度编号
        values.SetSize(dofs.Size()); // 并准备同长度的局部值向量
        for (int q = 0; q < points.GetNPoints(); ++q) // 随后遍历每个输出插值节点
        {
            const auto value = view.evaluate(fem_system_.mesh_data().mfem_mesh().GetAttribute(element), element, points.IntPoint(q)); // 传入原网格所属区域编号 原网格单元编号 当前节点在参考单元中的坐标 view.evaluate() 从该区域的输入场中，计算这个参考位置上的输入场值
            values[q] = std::get<double>(value); // 输出仅接收标量场
        }
        output.SetSubVector(dofs, values); // 按照当前单元的自由度编号，把局部值写入完整输出向量 分域场采用 L2 输出空间，相邻单元的自由度彼此独立，因此不会把界面两侧的值平均掉
    }
}

void OutputManager::clear_solve_information(const std::string& field_name) // 清空场对应的求解信息列表
{
    solve_information_[field_name].clear(); // 清空场对应的列表
}

void OutputManager::append_solve_information(const std::string& field_name, const std::string& name, double value) // 追加场对应的求解信息列表
{
    solve_information_[field_name].emplace_back(name, value); // 追加
}

const OutputManager::SolveInformation& OutputManager::solve_information(const std::string& field_name) const // 读取场对应的求解信息列表
{
    return solve_information_.at(field_name); // 读取
}

}
