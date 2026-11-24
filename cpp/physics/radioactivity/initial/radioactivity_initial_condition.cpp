#include "physics/radioactivity/initial/radioactivity_initial_condition.hpp"

#include <stdexcept>
#include <unordered_map>

namespace trimuse
{

namespace
{

std::unordered_map<int, double> initial_values(const FemSystem& fem_system, const std::vector<RadioactivityInitialConfig>& initial_configs) // 将输入的区域盘存转换为区域均匀浓度
{
    std::unordered_map<int, double> region_values;
    for (const RadioactivityInitialConfig& config : initial_configs)
    {
        if (config.model_name != "uniform_from_inventory")
        {
            throw std::invalid_argument("尚未支持的初始浓度模型: " + config.model_name);
        }
        const double inventory = std::get<double>(config.parameters.at("inventory"));
        region_values[config.region_attribute] = inventory / fem_system.mesh_data().region_volumes().at(config.region_attribute); // mol / m³
    }
    return region_values;
}

}

void initialize_concentration(const FemSystem& fem_system, RegionFieldData& accepted_concentration_field, const std::vector<RadioactivityInitialConfig>& initial_configs) // 分域场各区域自由度独立，可直接按区域设置均匀初值
{
    for (const auto& [region, value] : initial_values(fem_system, initial_configs))
    {
        accepted_concentration_field.field_data(region).assign(value);
    }
}

}
