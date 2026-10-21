#include "output/output_manager.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace trimuse
{

OutputManager::OutputManager(FemSystem& fem_system, const std::string& output_directory)
    : temperature_output_("temperature", &fem_system.mesh_data().mfem_mesh())
{
    const auto directory = std::filesystem::path(output_directory) / "thermal";
    std::filesystem::create_directories(directory);
    temperature_output_.SetOwnData(false);
    temperature_output_.SetPrefixPath(directory.string());
    temperature_output_.RegisterField("temperature", &fem_system.field_data("temperature").mfem_grid_function());
    const auto* nodes = fem_system.mesh_data().mfem_mesh().GetNodes();
    const int geometry_order = nodes ? nodes->FESpace()->GetMaxElementOrder() : 1;
    const int field_order = fem_system.finite_element_space("temperature").order();
    temperature_output_.SetLevelsOfDetail(std::max(geometry_order, field_order));
    temperature_output_.SetHighOrderOutput(true);
    temperature_output_.SetDataFormat(mfem::VTKFormat::BINARY);
    temperature_output_.SetPrecision(16);
}

void OutputManager::save(int cycle, double time, double time_step)
{
    temperature_output_.SetCycle(cycle);
    temperature_output_.SetTime(time);
    temperature_output_.SetTimeStep(time_step);
    temperature_output_.Save();
    if (temperature_output_.Error() != mfem::DataCollection::No_Error)
    {
        throw std::runtime_error("温度场 ParaView 输出失败");
    }
}

}
