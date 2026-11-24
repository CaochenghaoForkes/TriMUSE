#pragma once

#include <string>
#include <string_view>
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
    static void initialize_precomputed(PropertiesSystem& properties, const FemSystem& fem, const std::vector<std::string>& field_names, const ModelParameterMap& parameters);

private:
    struct ConditionalInitializer
    {
        std::string_view field_name;
        void (*initialize)(PropertiesSystem&, const FemSystem&);
    };

    // 按启用场初始化对应的预计算数据
    inline static constexpr ConditionalInitializer conditional_initializers_[] = {
        {"displacement", initialize_tensor_transform},
        {"concentration", initialize_nuclide_symbol},
        {"concentration", initialize_decay_constant},
    };
};

}
