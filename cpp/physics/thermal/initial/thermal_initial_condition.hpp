#pragma once

#include <string>
#include <vector>

#include "common/model_parameters.hpp"
#include "fem/fem_system.hpp"

namespace trimuse
{

struct ThermalInitialConfig // 某个区域采用哪个初始温度模型，以及该模型需要的参数
{
    int region_attribute; // 原网格中的区域属性编号，不是单元编号
    std::string model_name; // 初始温度模型名称
    ModelParameterMap parameters; // 模型所需的参数字典
};

void initialize_temperature(const FemSystem& fem_system, FieldData& accepted_temperature_field, const std::vector<ThermalInitialConfig>& initial_configs);

void initialize_temperature(RegionFieldData& accepted_temperature_field, const std::vector<ThermalInitialConfig>& initial_configs);

}
