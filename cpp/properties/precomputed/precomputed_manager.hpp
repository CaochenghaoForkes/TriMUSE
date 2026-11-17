#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "common/model_parameters.hpp"
#include "properties/precomputed/tensor_transform.hpp"

namespace trimuse
{

class PropertiesSystem;

class PrecomputedManager
{
public:
    // 保存物理卡参数，并按启用场一次性填入固定几何数据
    static void initialize_precomputed(PropertiesSystem& properties, const FemSystem& fem, const std::unordered_map<std::string, std::vector<std::string>>& field_required_properties, const ModelParameterMap& parameters);

private:
    struct ConditionalInitializer
    {
        std::string_view required_property;
        void (*initialize)(PropertiesFieldData&, const FemSystem&);
    };

    // 任一启用场需要表中的属性时，初始化该属性所需的预计算数据
    inline static constexpr ConditionalInitializer conditional_initializers_[] = {
        {"material.mechanics.elasticity_tensor", initialize_tensor_transform},
    };
};

}
