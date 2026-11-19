#include "physics/thermal/common/thermal_domain_model.hpp"

namespace trimuse
{

double ThermalDomainModel::scalar(const PropertiesFieldData& properties, const std::string& name, std::size_t point) // 供本类和派生类使用的辅助函数，用来从属性缓存中读取一个标量值
{
    return std::get<double>(properties.integration_point_values.at(name)[point].value()); // 找到指定属性的全域积分数组 -> 找到当前积分点的 optional<StateValue> -> 取出 StateValue -> 从 variant 中取出 double
}

}
