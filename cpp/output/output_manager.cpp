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
    auto output = std::make_unique<mfem::ParaViewDataCollection>(field_name, &fem_system_.mesh_data().mfem_mesh());
    output->SetOwnData(false);
    output->SetPrefixPath(directory.string());
    output->RegisterField(field_name, &fem_system_.field_data(field_name).mfem_grid_function());
    const auto* nodes = fem_system_.mesh_data().mfem_mesh().GetNodes();
    const int geometry_order = nodes ? nodes->FESpace()->GetMaxElementOrder() : 1;
    const int field_order = fem_system_.finite_element_space(field_name).order();
    output->SetLevelsOfDetail(std::max(geometry_order, field_order));
    output->SetHighOrderOutput(true);
    output->SetDataFormat(mfem::VTKFormat::BINARY);
    output->SetPrecision(16);
    field_outputs_[field_name] = std::move(output);
    solve_information_.try_emplace(field_name); // 如果没有键就对 field_name 插入个空的 SolveInformation 否则什么也不做
}

void OutputManager::save(const std::string& field_name, int cycle, double time, double time_step)
{
    mfem::ParaViewDataCollection& output = *field_outputs_.at(field_name);
    output.SetCycle(cycle);
    output.SetTime(time);
    output.SetTimeStep(time_step);
    output.Save();
    if (output.Error() != mfem::DataCollection::No_Error)
    {
        throw std::runtime_error("场 ParaView 输出失败: " + field_name);
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
