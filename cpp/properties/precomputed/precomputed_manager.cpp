#include <algorithm>
#include "properties/precomputed/precomputed_manager.hpp"
#include "properties/precomputed/geometry_center.hpp"
#include "properties/precomputed/initial_region_volume.hpp"
#include "properties/precomputed/initial_domain_quadrature_weight.hpp"
#include "properties/precomputed/initial_face_quadrature_weight.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

void PrecomputedManager::initialize_precomputed(PropertiesSystem& properties, const FemSystem& fem, const std::vector<std::string>& field_names, const ModelParameterMap& parameters)
{
    // 用户输入的预计算属性
    for (const auto& [name, value] : parameters)
    {
        properties.set_precomputed(name, value);
    }

    // 必定需要的预计算参数
    initialize_initial_region_volume(properties, fem);
    initialize_initial_domain_quadrature_weight(properties, fem);
    initialize_initial_face_quadrature_weight(properties, fem);
    initialize_geometry_center(properties, fem);

    // 场需要的预计算参数
    for (const auto& initializer : conditional_initializers_)
    {
        if (std::find(field_names.begin(), field_names.end(), initializer.field_name) != field_names.end())
        {
            initializer.initialize(properties, fem);
        }
    }
}

}
