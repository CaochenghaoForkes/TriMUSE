#include "output/domain_output.hpp"
#include "properties/properties_system.hpp"

#include <algorithm>
#include <filesystem>
#include <limits>
#include <stdexcept>

namespace trimuse
{

DomainOutput::DomainOutput(FemSystem& fem_system, PropertiesSystem& properties, const std::string& field_name, const std::string& directory) // 建立标量 L2 空间及 ParaView 输出对象
    :
    fem_system_(fem_system),
    properties_(properties),
    field_name_(field_name)
{
    int order = 0;
    if (fem_system_.has_field(field_name_))
    {
        order = fem_system_.region_field(field_name_).finite_element_space(fem_system_.mesh_data().region_attributes().front()).order();
    }
    else
    {
        properties_.require_field(field_name_);
    }
    collection_ = std::make_unique<mfem::L2_FECollection>(order, fem_system_.mesh_data().dimension());
    space_ = std::make_unique<mfem::FiniteElementSpace>(&fem_system_.mesh_data().mfem_mesh(), collection_.get()); // 在原网格建立标量 L2 输出空间，保留材料界面两侧的跳变
    values_ = std::make_unique<mfem::GridFunction>(space_.get());
    std::filesystem::create_directories(directory); // 创建该场的输出目录
    auto output = std::make_unique<mfem::ParaViewDataCollection>(field_name, &fem_system_.mesh_data().mfem_mesh()); // 指定数据集合名称及其使用的原网格
    output->SetOwnData(false); // 表示输出对象不负责销毁登记的网格和场值
    output->SetPrefixPath(directory); // 设置输出目录
    output->RegisterField(field_name, values_.get()); // 把场名和场值对应起来
    const auto* nodes = fem_system_.mesh_data().mfem_mesh().GetNodes();
    const int geometry_order = nodes ? nodes->FESpace()->GetMaxElementOrder() : 1; // 取得几何映射阶次
    output->SetLevelsOfDetail(std::max(geometry_order, order)); // 配置输出细节级别
    output->SetHighOrderOutput(true); // 启用高阶 VTK 输出以表示曲边几何和高阶场
    output->SetDataFormat(mfem::VTKFormat::BINARY); // 指定二进制输出格式
    output->SetPrecision(16); // 设置输出精度参数
    collection_output_ = std::move(output); // 转交资源
}

void DomainOutput::save(int cycle, double time, double time_step)
{
    if (fem_system_.has_field(field_name_))
    {
        save_field(fem_system_.field_view(field_name_), cycle, time, time_step);
        return;
    }
    const auto& domain_values = properties_.field_data().domain_integration_point_values;
    const auto& weights = domain_values.at("precomputed.initial_domain_quadrature_weight");
    const auto& samples = domain_values.at(field_name_);
    std::vector<double> values;
    values.reserve(samples.size());
    for (const auto& value : samples)
    {
        values.push_back(value ? std::get<double>(*value) : std::numeric_limits<double>::quiet_NaN()); // 未配置区域保持缺失值，不将其解释为零
    }
    save_quadrature_values(values, weights, cycle, time, time_step);
}

void DomainOutput::write_paraview(int cycle, double time, double time_step) // 把指定场在当前时间步的结果保存成 ParaView 文件
{
    mfem::ParaViewDataCollection& output = *collection_output_; // 取得该场的 ParaView 输出对象引用
    output.SetCycle(cycle); // 设置本次输出的编号，用于区分各帧
    output.SetTime(time); // 设置当前物理时间
    output.SetTimeStep(time_step); // 设置当前物理时间步长
    output.Save(); // 实际写入结果文件，并更新 ParaView 数据集合中的时间序列信息
    if (output.Error() != mfem::DataCollection::No_Error)
    {
        throw std::runtime_error("场 ParaView 输出失败: " + field_name_);
    }
}

void DomainOutput::save_field(const FieldView& view, int cycle, double time, double time_step) // 正式有限元场在输出节点取值，保留单元内部变化与区域间跳变
{
    mfem::Array<int> dofs;
    mfem::Vector local;
    for (int element = 0; element < space_->GetNE(); ++element)
    {
        const auto& points = space_->GetFE(element)->GetNodes();
        space_->GetElementDofs(element, dofs);
        local.SetSize(dofs.Size());
        const int region = fem_system_.mesh_data().mfem_mesh().GetAttribute(element);
        for (int q = 0; q < points.GetNPoints(); ++q)
        {
            local[q] = std::get<double>(view.evaluate(region, element, points.IntPoint(q)));
        }
        values_->SetSubVector(dofs, local);
    }
    write_paraview(cycle, time, time_step);
}

void DomainOutput::save_quadrature_values(const std::vector<double>& values, const PropertiesFieldData::DomainIntegrationPointValues& weights, int cycle, double time, double time_step) // 对于传入全域求积点上的标量值数组，把它逐单元投影到已经创建好的 L2 输出场中
{
    // H1 连续有限元空间在相邻单元的公共界面上共享自由度，各单元的贡献会累加到相同的总体自由度上
    // 此处的不连续 L2 有限元空间中，各单元拥有独立的自由度，同一几何位置在不同单元中可以具有不同的值
    // 因此本投影的总体质量矩阵按单元分块对角，可以逐单元求解并直接写入输出场，保留材料界面两侧的跳变
    // 数学上 L2 要求函数平方可积，H1 还要求一阶弱导数平方可积；L2 允许不连续，但并不要求一定不连续
    auto& output = *values_;
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
    write_paraview(cycle, time, time_step);
}

}
