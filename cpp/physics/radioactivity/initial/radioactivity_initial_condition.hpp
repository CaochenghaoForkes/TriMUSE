#pragma once

#include <string>
#include <vector>

#include "common/model_parameters.hpp"
#include "fem/fem_system.hpp"

namespace trimuse
{

struct RadioactivityInitialConfig // 一个材料区域的初始核素盘存模型配置
{
    int region_attribute; // 原网格材料区域编号
    std::string model_name; // 当前支持 uniform_from_inventory
    ModelParameterMap parameters; // 模型参数，包括该区域的初始盘存 inventory
};

// 初始盘存 atom 除以区域体积得到 atom/m³, 投影时不施加后续边界约束。
void initialize_concentration(const FemSystem& fem_system, FieldData& accepted_concentration_field, const std::vector<RadioactivityInitialConfig>& initial_configs);

void initialize_concentration(const FemSystem& fem_system, RegionFieldData& accepted_concentration_field, const std::vector<RadioactivityInitialConfig>& initial_configs); // 分域场逐区域直接赋初值，后续再处理完美接触约束

}
