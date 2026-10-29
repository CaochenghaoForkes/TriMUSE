#pragma once

#include <string>
#include <vector>

#include "common/model_parameters.hpp"
#include "fem/fem_system.hpp"

namespace trimuse
{

struct RadioactivityInitialConfig
{
    int region_attribute;
    std::string model_name;
    ModelParameterMap parameters;
};

// 初始盘存 atom 除以区域体积得到 atom/m³, 投影时不施加后续边界约束。
void initialize_concentration(const FemSystem& fem_system, FieldData& concentration_field, const std::vector<RadioactivityInitialConfig>& initial_configs);

void initialize_concentration(const FemSystem& fem_system, RegionFieldData& concentration_field, const std::vector<RadioactivityInitialConfig>& initial_configs);

}
