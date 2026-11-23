#include "properties/properties_state_point.hpp"

namespace trimuse
{

const std::vector<std::size_t>& PropertiesStatePoint::region_point_indices() const
{
    return *region_domain_point_indices;
}

const PropertiesFieldData::CachedValue& PropertiesStatePoint::global_value(const std::string& name) const
{
    return *std::get<const PropertiesFieldData::CachedValue*>(references.at(name));
}

const PropertiesFieldData::AttributeValues& PropertiesStatePoint::attribute_values(const std::string& name) const
{
    return *std::get<const PropertiesFieldData::AttributeValues*>(references.at(name));
}

const PropertiesFieldData::DomainIntegrationPointValues& PropertiesStatePoint::domain_values(const std::string& name) const
{
    return *std::get<const PropertiesFieldData::DomainIntegrationPointValues*>(references.at(name));
}

const PropertiesFieldData::FaceIntegrationPointValues& PropertiesStatePoint::face_values(const std::string& name) const
{
    return *std::get<const PropertiesFieldData::FaceIntegrationPointValues*>(references.at(name));
}

double PropertiesStatePoint::scalar(const std::string& name) const // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中取出其中存储的 double
{
    return std::get<double>(values.at(name));
}

const VectorValue& PropertiesStatePoint::vector(const std::string& name) const // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中取出其中存储的 vector
{
    return std::get<VectorValue>(values.at(name));
}

const Tensor2DValue& PropertiesStatePoint::tensor_2d(const std::string& name) const // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中取出其中存储的 Tensor2DValue
{
    return std::get<Tensor2DValue>(values.at(name));
}

const Tensor3DValue& PropertiesStatePoint::tensor_3d(const std::string& name) const // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中取出其中存储的 Tensor3DValue
{
    return std::get<Tensor3DValue>(values.at(name));
}

const ElasticityTensorValue& PropertiesStatePoint::elasticity_tensor(const std::string& name) const
{
    return std::get<ElasticityTensorValue>(values.at(name));
}

}
