#include "physics/radioactivity/common/radioactivity_domain_model.hpp"

namespace trimuse
{

double RadioactivityDomainModel::scalar(const PropertiesFieldData& properties, const std::string& name, std::size_t point) // 读取指定属性在全域积分点编号 point 处的标量值
{
    return std::get<double>(properties.domain_integration_point_values.at(name)[point].value());
}

}
