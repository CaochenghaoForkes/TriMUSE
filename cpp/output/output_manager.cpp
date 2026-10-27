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

void OutputManager::add_field(const std::string& field_name, const std::string& directory_name)
{
    const auto directory = std::filesystem::path(output_directory_) / directory_name;
    std::filesystem::create_directories(directory);
    FieldOutput field_output;
    const bool regional = fem_system_.has_region_field(field_name);
    const auto& field_space = regional
        ? fem_system_.region_field(field_name).finite_element_space(fem_system_.mesh_data().region_attributes().front())
        : fem_system_.finite_element_space(field_name);
    mfem::GridFunction* values;
    if (regional)
    {
        field_output.collection = std::make_unique<mfem::L2_FECollection>(field_space.order(), fem_system_.mesh_data().dimension());
        field_output.space = std::make_unique<mfem::FiniteElementSpace>(&fem_system_.mesh_data().mfem_mesh(),
            field_output.collection.get(), field_space.vector_dimension());
        field_output.values = std::make_unique<mfem::GridFunction>(field_output.space.get());
        values = field_output.values.get();
    }
    else { values = &fem_system_.field_data(field_name).mfem_grid_function(); }
    auto output = std::make_unique<mfem::ParaViewDataCollection>(field_name, &fem_system_.mesh_data().mfem_mesh());
    output->SetOwnData(false);
    output->SetPrefixPath(directory.string());
    output->RegisterField(field_name, values);
    const auto* nodes = fem_system_.mesh_data().mfem_mesh().GetNodes();
    const int geometry_order = nodes ? nodes->FESpace()->GetMaxElementOrder() : 1;
    const int field_order = field_space.order();
    output->SetLevelsOfDetail(std::max(geometry_order, field_order));
    output->SetHighOrderOutput(true);
    output->SetDataFormat(mfem::VTKFormat::BINARY);
    output->SetPrecision(16);
    field_output.collection_output = std::move(output);
    field_outputs_[field_name] = std::move(field_output);
    solve_information_.try_emplace(field_name); // 如果没有键就对 field_name 插入个空的 SolveInformation 否则什么也不做
}

void OutputManager::save(const std::string& field_name, int cycle, double time, double time_step)
{
    auto& field_output = field_outputs_.at(field_name);
    if (field_output.values) { update_region_output(field_name, *field_output.values); }
    mfem::ParaViewDataCollection& output = *field_output.collection_output;
    output.SetCycle(cycle);
    output.SetTime(time);
    output.SetTimeStep(time_step);
    output.Save();
    if (output.Error() != mfem::DataCollection::No_Error)
    {
        throw std::runtime_error("场 ParaView 输出失败: " + field_name);
    }
}

void OutputManager::update_region_output(const std::string& name, mfem::GridFunction& output)
{
    const auto view = fem_system_.field_view(name);
    auto* space = output.FESpace();
    mfem::Array<int> dofs;
    mfem::Vector values;
    for (int element = 0; element < space->GetNE(); ++element)
    {
        const auto& points = space->GetFE(element)->GetNodes();
        space->GetElementVDofs(element, dofs);
        values.SetSize(dofs.Size());
        for (int q = 0; q < points.GetNPoints(); ++q)
        {
            const auto value = view.evaluate(fem_system_.mesh_data().mfem_mesh().GetAttribute(element), element, points.IntPoint(q));
            if (space->GetVDim() == 1) { values[q] = std::get<double>(value); }
            else
            {
                const auto& components = std::get<VectorValue>(value);
                for (int c = 0; c < space->GetVDim(); ++c) { values[c * points.GetNPoints() + q] = components[c]; }
            }
        }
        output.SetSubVector(dofs, values);
    }
}

void OutputManager::clear_solve_information(const std::string& field_name)
{
    solve_information_[field_name].clear(); // 清空场对应的列表
}

void OutputManager::append_solve_information(const std::string& field_name, const std::string& name, double value)
{
    solve_information_[field_name].emplace_back(name, value); // 追加
}

const OutputManager::SolveInformation& OutputManager::solve_information(const std::string& field_name) const
{
    return solve_information_.at(field_name); // 读取
}

}
