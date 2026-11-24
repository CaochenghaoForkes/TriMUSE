#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>

#include "common/state_value.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

// 当前计算点与已取好的数值; 属性键使用完整名称, 求解场仍使用 temperature/concentration 等场名
// 求解场只在组装此临时状态时读取
struct PropertiesStatePoint
{
    using PropertyReference = std::variant<const PropertiesFieldData::CachedValue*, const PropertiesFieldData::AttributeValues*, const PropertiesFieldData::DomainIntegrationPointValues*, const PropertiesFieldData::FaceIntegrationPointValues*>;

    const std::vector<std::size_t>* region_domain_point_indices = nullptr; // 当前区域的全域求积点编号，只引用 FemSystem 中的缓存
    int region_attribute = 0;
    int zone_attribute = 0; // 边界物理组编号，与材料区域编号分别使用
    int interface_attribute = 0; // 内部界面物理组编号
    int face_attribute = 0; // 外边界或内部界面的物理组编号
    int face_index = 0; // 原网格面编号
    int face_point_index = 0; // 面内求积点编号
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    std::optional<StateValue> previous_value; // 本属性上一个已接受时刻的值，首次计算时为空
    std::unordered_map<std::string, StateValue> values; // 属性名或场名 -> 单时空点上的 <double/vector/2d_tensor/3d_tensor>
    std::unordered_map<std::string, PropertyReference> references; // 属性名 -> 原始存储容器的只读引用

    const std::vector<std::size_t>& region_point_indices() const; // 读取当前区域包含的求积点编号
    const PropertiesFieldData::CachedValue& global_value(const std::string& name) const; // 取全局属性引用
    const PropertiesFieldData::AttributeValues& attribute_values(const std::string& name) const; // 取区域/边界分区属性引用
    const PropertiesFieldData::DomainIntegrationPointValues& domain_values(const std::string& name) const; // 取全域求积点属性引用
    const PropertiesFieldData::FaceIntegrationPointValues& face_values(const std::string& name) const; // 取全域面片求积点属性引用
    double scalar(const std::string& name) const; // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中期望取出其中存储的 double
    const VectorValue& vector(const std::string& name) const; // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中期望取出其中存储的 vector
    const Tensor2DValue& tensor_2d(const std::string& name) const; // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中期望取出其中存储的 Tensor2DValue
    const Tensor3DValue& tensor_3d(const std::string& name) const; // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中期望取出其中存储的 Tensor3DValue
    const ElasticityTensorValue& elasticity_tensor(const std::string& name) const; // 读取六分量弹性矩阵
};

}
