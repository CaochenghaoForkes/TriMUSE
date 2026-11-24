#include "physics/thermal/initial/thermal_initial_condition.hpp"

#include <stdexcept>

namespace trimuse
{

void initialize_temperature(RegionFieldData& accepted_temperature_field, const std::vector<ThermalInitialConfig>& initial_configs) // 给分域温度场中的各个区域分别设置均匀初温
{
    for (const auto& config : initial_configs)
    {
        if (config.model_name != "uniform")
        {
            throw std::invalid_argument("尚未支持的初始温度模型: " + config.model_name);
        }
        // 分域场两侧的自由度独立，所以这里可以直接逐域赋值 若界面采用理想接触，后续再由约束下的初值投影处理两侧初温的一致性
        accepted_temperature_field.field_data(config.region_attribute).assign(std::get<double>(config.parameters.at("uniform_value"))); // field_data(config.region_attribute): 取得指定区域的 FieldData parameters.at("uniform_value")：从参数字典取出初温参数 std::get<double>(...)：从 variant 中取出 double 数值 .assign(...)：把该区域场向量的全部自由度设为这个温度
    }
}

}
