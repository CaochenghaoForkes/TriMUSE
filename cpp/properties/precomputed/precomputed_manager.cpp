#include <algorithm>
#include "properties/precomputed/precomputed_manager.hpp"
#include "properties/precomputed/geometry_center.hpp"
#include "properties/precomputed/initial_region_volume.hpp"
#include "properties/precomputed/initial_domain_quadrature_weight.hpp"
#include "properties/precomputed/initial_interface_area.hpp"
#include "properties/precomputed/initial_face_quadrature_weight.hpp"
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

    // 必定需要的预计算参数
    initialize_initial_region_volume(properties, fem);
    initialize_initial_domain_quadrature_weight(properties, fem);
    initialize_initial_interface_area(properties, fem);
    initialize_initial_face_quadrature_weight(properties, fem);
    initialize_geometry_center(properties, fem);

    // 场需要的预计算参数
    for (const auto& initializer : conditional_initializers_)
    {
        const bool needed = std::any_of(field_required_properties.begin(), field_required_properties.end(), [&initializer](const auto& field) { const auto& required = field.second; return field.first == initializer.requirement || std::find(required.begin(), required.end(), initializer.requirement) != required.end(); });
        if (needed)
        {
            initializer.initialize(properties, fem);
        }
    }
}

}
