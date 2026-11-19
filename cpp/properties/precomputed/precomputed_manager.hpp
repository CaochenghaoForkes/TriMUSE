#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "common/model_parameters.hpp"
#include "properties/precomputed/decay_constant.hpp"
#include "properties/precomputed/nuclide_symbol.hpp"
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
        std::string_view requirement;
        void (*initialize)(PropertiesSystem&, const FemSystem&);
    };

    // 启用指定场或需要指定属性时，初始化对应的预计算数据
    inline static constexpr ConditionalInitializer conditional_initializers_[] = {
        {"material.mechanics.elasticity_tensor", initialize_tensor_transform},
        {"concentration", initialize_nuclide_symbol},
        {"concentration", initialize_decay_constant},
    };
};

}
