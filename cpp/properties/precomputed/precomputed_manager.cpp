#include "properties/precomputed/precomputed_manager.hpp"

#include <algorithm>

#include "properties/precomputed/nuclide_symbol.hpp"
#include "properties/precomputed/region_volume.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

void PrecomputedManager::initialize_precomputed(PropertiesSystem& properties, const FemSystem& fem, const std::unordered_map<std::string, std::vector<std::string>>& field_required_properties, const ModelParameterMap& parameters)
{
    // 用户输入的预计算属性
    for (const auto& [name, value] : parameters)
    {
        properties.set_precomputed(name, value);
    }

    initialize_nuclide_symbol(properties, parameters);
    auto& field_data = properties.field_data();

    // 必定需要的预计算参数
    initialize_region_volume(field_data, fem);
    const auto& center = fem.geometry_center();
    properties.set_precomputed("geometry_center", VectorValue(center.begin(), center.end()));

    // 场需要的预计算参数
    for (const auto& initializer : conditional_initializers_)
    {
        const bool needed = std::any_of(field_required_properties.begin(), field_required_properties.end(),
            [&initializer](const auto& field)
            {
                const auto& required = field.second;
                return std::find(required.begin(), required.end(), initializer.required_property) != required.end();
            });
        if (needed)
        {
            initializer.initialize(field_data, fem);
        }
    }
}

}
