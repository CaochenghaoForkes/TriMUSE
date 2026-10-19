#pragma once

#include <string>
#include <vector>

#include "common/model_parameters.hpp"
#include "fem/fem_system.hpp"

namespace trimuse
{

struct ThermalInitialConfig
{
    int region_attribute;
    std::string model_name;
    ModelParameterMap parameters;
};

// 仅在初始时刻调用, 不施加后续的温度边界约束
void initialize_temperature(const FemSystem& fem_system, FieldData& temperature_field, const std::vector<ThermalInitialConfig>& initial_configs);

}
