#include "output/output_manager.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace trimuse
{

OutputManager::OutputManager(FemSystem& fem_system, const std::string& output_directory)
    : fem_system_(fem_system), output_directory_(output_directory)
{
}

void OutputManager::add_field(const std::string& field_name, const std::string& directory_name) // 为一个场建立输出目录，准备要输出的 GridFunction，并注册到 ParaView 输出对象中
{
    const auto directory = std::filesystem::path(output_directory_) / directory_name; // 拼接该场的输出目录 如 output_directory_="/结果" directory_name="thermal" directory="/结果/thermal"
    std::filesystem::create_directories(directory); // 创建该场的输出目录
    FieldOutput field_output; // 创建一组输出资源
    const bool regional = fem_system_.has_region_field(field_name); // 判断是否存在该名称的分域场
    const auto& field_space = regional // 根据分域场/连续场 选择不同方式返回有限元空间
        ? fem_system_.region_field(field_name).finite_element_space(fem_system_.mesh_data().region_attributes().front()) // 分域场取第一个区域，是因为当前各区域使用相同阶次和分量数
        : fem_system_.finite_element_space(field_name); // 连续场直接根据场名返回其唯一有限元空间
    mfem::GridFunction* values; // 准备一个指针，最终指向交给 ParaView 输出的场值对象
    if (regional) // 对于分域场
    {
        field_output.collection = std::make_unique<mfem::L2_FECollection>(field_space.order(), fem_system_.mesh_data().dimension()); // 创建与计算场同阶的 L2 有限元集合 L2 允许单元之间不连续，可以保存界面两侧不同的值
        field_output.space = std::make_unique<mfem::FiniteElementSpace>(&fem_system_.mesh_data().mfem_mesh(), field_output.collection.get(), field_space.vector_dimension()); // 在原网格上建立输出空间，分量数与计算场一致
        field_output.values = std::make_unique<mfem::GridFunction>(field_output.space.get()); // 创建输出场值，并让 field_output.values 持有 这里仅准备存储，分域计算结果会在保存前通过 update_region_output() 填入
        values = field_output.values.get(); // 取得这个对象的普通指针，赋给局部变量 values
    }
    else 
    { 
        values = &fem_system_.field_data(field_name).mfem_grid_function(); // 连续场则直接使用正式场
    }
    auto output = std::make_unique<mfem::ParaViewDataCollection>(field_name, &fem_system_.mesh_data().mfem_mesh()); // 指定数据集合名称及其使用的原网格
    output->SetOwnData(false); // 表示输出对象不负责销毁登记的网格和场值
    output->SetPrefixPath(directory.string()); // 设置输出目录
    output->RegisterField(field_name, values); // 把场名和场值对应起来
    const auto* nodes = fem_system_.mesh_data().mfem_mesh().GetNodes(); 
    const int geometry_order = nodes ? nodes->FESpace()->GetMaxElementOrder() : 1; // 取得几何映射阶次
    const int field_order = field_space.order(); // 取得场插值阶次
    output->SetLevelsOfDetail(std::max(geometry_order, field_order)); // 配置输出细节级别
    output->SetHighOrderOutput(true); // 启用高阶 VTK 输出以表示曲边几何和高阶场
    output->SetDataFormat(mfem::VTKFormat::BINARY); // 指定二进制输出格式
    output->SetPrecision(16); // 设置输出精度参数
    field_output.collection_output = std::move(output); // 转交资源
    field_outputs_[field_name] = std::move(field_output); // 转交资源
    solve_information_.try_emplace(field_name); // 如果没有键就对 field_name 插入个空的 SolveInformation 否则什么也不做
}

void OutputManager::save(const std::string& field_name, int cycle, double time, double time_step) // 把指定场在当前时间步的结果保存成 ParaView 文件
{
    auto& field_output = field_outputs_.at(field_name); // 根据场名找到对应的输出资源
    if (field_output.values) // 检查是否存在专门的输出场
    { 
        update_region_output(field_name, *field_output.values); // 若是分域场且存在,则把各区域的最新计算结果整理到原网格上的 L2 输出场中 若是连续场 指针为空，输出对象已直接关联正式 GridFunction，可以读取最新值
    }
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

void OutputManager::update_region_output(const std::string& name, mfem::GridFunction& output) // 把各区域独立保存的计算结果，逐单元插值到原网格上的 L2 输出场中，以便统一保存成 ParaView 文件
{
    const auto view = fem_system_.field_view(name); // 根据场名取得统一的场访问接口。通过它可以读取对应区域的场值，并在内部完成原网格单元编号到子网格单元编号的映射
    auto* space = output.FESpace(); // 取得输出场的有限元空间，并准备两个临时容器
    mfem::Array<int> dofs; // 当前单元在输出场中的自由度编号
    mfem::Vector values; // 这些自由度对应的场值
    for (int element = 0; element < space->GetNE(); ++element) // 遍历输出空间所在原网格的全部域单元
    {
        const auto& points = space->GetFE(element)->GetNodes(); // 取得当前输出有限元的参考插值节点 些点用于插值，不是前面材料属性计算使用的求积点
        space->GetElementVDofs(element, dofs); // 取得当前单元的全部输出自由度编号，包括向量场的各分量
        values.SetSize(dofs.Size()); // 并准备同长度的局部值向量
        for (int q = 0; q < points.GetNPoints(); ++q) // 随后遍历每个输出插值节点
        {
            const auto value = view.evaluate(fem_system_.mesh_data().mfem_mesh().GetAttribute(element), element, points.IntPoint(q)); // 传入原网格所属区域编号 原网格单元编号 当前节点在参考单元中的坐标 view.evaluate() 从该区域的计算场中，计算这个参考位置上的场值
            if (space->GetVDim() == 1) 
            { 
                values[q] = std::get<double>(value); // 标量场直接保存
            }
            else
            {
                const auto& components = std::get<VectorValue>(value); // 向量场则逐分量保存：
                for (int c = 0; c < space->GetVDim(); ++c) 
                { 
                    values[c * points.GetNPoints() + q] = components[c]; 
                }
            }
        }
        output.SetSubVector(dofs, values); // 按照当前单元的自由度编号，把局部值写入完整输出向量 L2 输出空间的相邻单元自由度彼此独立，因此不会把界面两侧的值平均掉
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
