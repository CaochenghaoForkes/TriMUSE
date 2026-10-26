#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "fem/fem_system.hpp"
#include "properties/derived/derived_model_manager.hpp"
#include "properties/external/external_model_manager.hpp"
#include "properties/material/material_model_manager.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class PropertiesSystem
{
public:
    using FieldMap = std::unordered_map<std::string, const mfem::GridFunction*>;

    explicit PropertiesSystem(const FemSystem& fem_system);

    // 完整模型路径: 属性全名 + 模型名, 如 external.power.common.constant。
    void assign(int attribute, const std::string& model_name, const ModelParameterMap& parameters);

    void prepare_dependencies();

    void mark_coupled(const std::string& field_name, const std::vector<std::string>& property_names);

    void update_external(double time);

    void update_derived();

    void update_material();

    void update_field_dependent_materials(const std::string& field_name, const FieldMap& trial_fields = {}, bool coupled_only = false);

    void accept_current_state();

    const PropertiesFieldData& previous_field_data() const;

    bool coupled_with_field(const std::string& property_name, int region_attribute, const std::string& field_name) const;

    PropertiesFieldData& field_data();

    const PropertiesFieldData& field_data() const;

private:
    enum class PropertyType { Precomputed, External, Derived, Material };
    enum class PropertyScope { Global, Region, Zone, IntegrationPoint };

    struct PropertyInfo
    {
        std::string name;
        PropertyType type;
        PropertyScope scope;
        std::unordered_map<std::string, std::unordered_set<int>> field_regions;
        std::unordered_map<std::string, std::unordered_set<int>> coupled_regions;
    };

    PropertyInfo& property_info(const std::string& name);

    const PropertyInfo& property_info(const std::string& name) const;

    std::size_t property_index(const std::string& name) const;

    const std::vector<std::string>& model_required_states(const PropertyInfo& property, const std::string& model_name) const;

    bool depends_on_field(const std::string& property_name, int region_attribute, const std::string& field_name) const;

    static PropertiesStatePoint make_state_point(PropertyScope scope, int attribute);

    template <typename Manager>
    void evaluate_and_store(const Manager& manager, const std::string& name, const PropertiesStatePoint& point);

    template <typename Manager>
    void evaluate_and_store_points(const Manager& manager, const PropertyInfo& property, int region_attribute, const std::vector<std::string>& required, const FieldMap& trial_fields = {});

    void load_required_states(PropertiesStatePoint& point, const std::vector<std::string>& names, std::optional<std::size_t> point_index = std::nullopt) const;

    bool required_states_changed(int attribute, const std::vector<std::string>& required_states) const;

    std::vector<PropertyInfo> properties_ = {
        {"precomputed.region_volume", PropertyType::Precomputed, PropertyScope::Region},
        {"external.time", PropertyType::External, PropertyScope::Global},
        {"external.environment_temperature", PropertyType::External, PropertyScope::Zone},
        {"external.environment_nuclide_concentration", PropertyType::External, PropertyScope::Zone}, // atom/m³
        {"external.surface_nuclide_concentration", PropertyType::External, PropertyScope::Zone}, // atom/m³, 指定表面浓度
        {"external.nuclide_flux", PropertyType::External, PropertyScope::Zone}, // atom/(m²·s), 外流为正、流入为负
        {"external.surface_temperature", PropertyType::External, PropertyScope::Zone}, // K, 指定表面温度
        {"external.heat_flux", PropertyType::External, PropertyScope::Zone}, // W/m², 外流为正、流入为负
        {"external.burnup_fima_fraction", PropertyType::External, PropertyScope::Global},
        {"external.power", PropertyType::External, PropertyScope::Region},
        {"external.nuclide_birth_rate", PropertyType::External, PropertyScope::Global}, // atom/s, 全局核素产生率
        {"external.uranium_fraction", PropertyType::External, PropertyScope::Region}, // 各区域占全局产生率的份额
        {"derived.heat_transfer_coef", PropertyType::Derived, PropertyScope::Zone},
        {"derived.mass_transfer_coef", PropertyType::Derived, PropertyScope::Zone}, // m/s
        {"derived.volumetric_power", PropertyType::Derived, PropertyScope::IntegrationPoint},
        {"derived.volumetric_nuclide_birth_rate", PropertyType::Derived, PropertyScope::IntegrationPoint}, // atom/(m³·s)
        {"material.general.porosity", PropertyType::Material, PropertyScope::IntegrationPoint},
        {"material.general.mass_density", PropertyType::Material, PropertyScope::IntegrationPoint},
        {"material.thermal.conductivity", PropertyType::Material, PropertyScope::IntegrationPoint},
        {"material.thermal.specific_heat", PropertyType::Material, PropertyScope::IntegrationPoint},
        {"material.radioactivity.nuclide_diffusion_coef", PropertyType::Material, PropertyScope::IntegrationPoint} // m²/s
    };

    inline static const std::unordered_set<std::string> solution_field_states = {
        "temperature", "concentration", "displacement", "stress", "strain"
    };

    const FemSystem& fem_system_;

    MaterialModelManager material_model_manager_;
    ExternalModelManager external_model_manager_;
    DerivedModelManager derived_model_manager_;
    PropertiesFieldData properties_field_data_;
    PropertiesFieldData previous_field_data_;
    std::unordered_map<std::string, std::unordered_map<int, std::string>> assigned_models_;
    PropertiesFieldData required_state_snapshots_;
    std::unordered_map<std::string, std::unordered_set<int>> computed_locations_;
};

}
