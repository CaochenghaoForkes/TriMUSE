#include "properties/properties_system.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace trimuse
{

PropertiesSystem::PropertiesSystem(const FemSystem& fem_system)
    : fem_system_(fem_system)
{
    for (const PropertyInfo& property : properties_)
    {
        if (property.scope == PropertyScope::Global)
        {
            properties_field_data_.global_values.emplace(property.name, std::nullopt);
        }
        else if (property.scope == PropertyScope::Region || property.scope == PropertyScope::Zone)
        {
            auto& properties = property.scope == PropertyScope::Region ? properties_field_data_.region_values : properties_field_data_.zone_values;
            const auto attributes = property.scope == PropertyScope::Region ? fem_system_.mesh_data().region_attributes() : fem_system_.mesh_data().boundary_attributes();
            auto& values = properties[property.name];
            for (const int attribute : attributes)
            {
                values.emplace(attribute, std::nullopt);
            }
        }
        else
        {
            properties_field_data_.integration_point_values.emplace(property.name, PropertiesFieldData::IntegrationPointValues{});
        }
    }
    for (const auto& [region_attribute, volume] : fem_system_.mesh_data().region_volumes())
    {
        properties_field_data_.region_values["region_volume"][region_attribute] = StateValue{volume};
    }
}

PropertiesSystem::PropertyInfo& PropertiesSystem::property_info(const std::string& name)
{
    return properties_[property_index(name)];
}

const PropertiesSystem::PropertyInfo& PropertiesSystem::property_info(const std::string& name) const
{
    return properties_[property_index(name)];
}

std::size_t PropertiesSystem::property_index(const std::string& name) const
{
    const auto property = std::find_if(properties_.begin(), properties_.end(), [&name](const PropertyInfo& item)
    {
        return item.name == name;
    });
    if (property == properties_.end())
    {
        throw std::out_of_range("未知属性: " + name);
    }
    return static_cast<std::size_t>(property - properties_.begin());
}

const std::vector<std::string>& PropertiesSystem::model_required_states(const PropertyInfo& property, const std::string& model_name) const
{
    if (property.type == PropertyType::External)
    {
        return external_model_manager_.required_states(property.name, model_name);
    }
    if (property.type == PropertyType::Derived)
    {
        return derived_model_manager_.required_states(property.name, model_name);
    }
    return material_model_manager_.required_states(property.name, model_name);
}

void PropertiesSystem::assign(int attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    if (model_name.rfind("material.", 0) == 0)
    {
        const std::size_t property_end = model_name.find('.', model_name.find('.', 9) + 1);
        const std::string property_name = model_name.substr(9, property_end - 9);
        const std::string specific_model_name = model_name.substr(property_end + 1);
        material_model_manager_.assign(attribute, property_name, specific_model_name, parameters);
        assigned_models_[property_name][attribute] = specific_model_name;
        const int location = property_info(property_name).scope == PropertyScope::Global ? 0 : attribute;
        computed_locations_[property_name].erase(location);
        return;
    }
    if (model_name.rfind("external.", 0) == 0)
    {
        const std::size_t property_end = model_name.find('.', 9);
        const std::string property_name = model_name.substr(9, property_end - 9);
        const std::string specific_model_name = model_name.substr(property_end + 1);
        external_model_manager_.assign(property_name, attribute, specific_model_name, parameters);
        assigned_models_[property_name][attribute] = specific_model_name;
        const int location = property_info(property_name).scope == PropertyScope::Global ? 0 : attribute;
        computed_locations_[property_name].erase(location);
        return;
    }
    if (model_name.rfind("derived.", 0) == 0)
    {
        const std::size_t property_end = model_name.find('.', 8);
        const std::string property_name = model_name.substr(8, property_end - 8);
        const std::string specific_model_name = model_name.substr(property_end + 1);
        derived_model_manager_.assign(property_name, attribute, specific_model_name, parameters);
        assigned_models_[property_name][attribute] = specific_model_name;
        const int location = property_info(property_name).scope == PropertyScope::Global ? 0 : attribute;
        computed_locations_[property_name].erase(location);
        return;
    }
    throw std::out_of_range("尚未支持的模型分类: " + model_name);
}

void PropertiesSystem::prepare_dependencies()
{
    for (PropertyInfo& property : properties_)
    {
        property.field_regions.clear();
        property.coupled_regions.clear();
    }
    for (std::size_t index = 0; index < properties_.size(); ++index)
    {
        PropertyInfo& property = properties_[index];
        const auto assigned = assigned_models_.find(property.name);
        if (assigned == assigned_models_.end())
        {
            continue;
        }
        for (const auto& [region_attribute, model_name] : assigned->second)
        {
            const int location = property.scope == PropertyScope::Global ? 0 : region_attribute;
            for (const std::string& required_name : model_required_states(property, model_name))
            {
                if (solution_field_states.count(required_name) != 0)
                {
                    if (property.type != PropertyType::Material)
                    {
                        throw std::logic_error("外部量和派生量不能依赖求解场: " + property.name);
                    }
                    property.field_regions[required_name].insert(location);
                    continue;
                }
                const std::size_t upstream_index = property_index(required_name);
                const PropertyInfo& upstream = properties_[upstream_index];
                if (upstream.type > property.type || (upstream.type == property.type && upstream_index >= index))
                {
                    throw std::logic_error("属性依赖必须遵守 external → derived → material 及类内先后顺序: " + property.name);
                }
                const int upstream_location = upstream.scope == PropertyScope::Global ? 0 : region_attribute;
                for (const auto& [field_name, regions] : upstream.field_regions)
                {
                    if (regions.count(upstream_location) != 0)
                    {
                        property.field_regions[field_name].insert(location);
                    }
                }
            }
        }
    }
}

void PropertiesSystem::mark_coupled(const std::string& field_name, const std::vector<std::string>& property_names)
{
    for (const std::string& name : property_names)
    {
        const auto assigned = assigned_models_.find(name);
        if (assigned == assigned_models_.end())
        {
            continue;
        }
        for (const auto& assignment : assigned->second)
        {
            PropertyInfo& property = property_info(name);
            const int location = property.scope == PropertyScope::Global ? 0 : assignment.first;
            if (depends_on_field(name, location, field_name))
            {
                property.coupled_regions[field_name].insert(location);
            }
        }
    }
    for (auto property = properties_.rbegin(); property != properties_.rend(); ++property)
    {
        const auto assigned = assigned_models_.find(property->name);
        if (assigned == assigned_models_.end())
        {
            continue;
        }
        for (const auto& [region_attribute, model_name] : assigned->second)
        {
            const int location = property->scope == PropertyScope::Global ? 0 : region_attribute;
            if (!coupled_with_field(property->name, location, field_name))
            {
                continue;
            }
            for (const std::string& required_name : model_required_states(*property, model_name))
            {
                if (solution_field_states.count(required_name) != 0)
                {
                    continue;
                }
                PropertyInfo& upstream = property_info(required_name);
                const int upstream_location = upstream.scope == PropertyScope::Global ? 0 : region_attribute;
                if (depends_on_field(required_name, upstream_location, field_name))
                {
                    upstream.coupled_regions[field_name].insert(upstream_location);
                }
            }
        }
    }
}

PropertiesStatePoint PropertiesSystem::make_state_point(PropertyScope scope, int attribute)
{
    PropertiesStatePoint point;
    point.region_attribute = scope == PropertyScope::Region ? attribute : 0;
    point.zone_attribute = scope == PropertyScope::Zone ? attribute : 0;
    return point;
}

template <typename Manager>
void PropertiesSystem::evaluate_and_store(const Manager& manager, const std::string& name, const PropertiesStatePoint& point)
{
    StateValue value = manager.evaluate(name, point);
    const PropertyScope scope = property_info(name).scope;
    if (scope == PropertyScope::Global)
    {
        properties_field_data_.global_values[name] = std::move(value);
    }
    else if (scope == PropertyScope::Zone)
    {
        properties_field_data_.zone_values[name][point.zone_attribute] = std::move(value);
    }
    else
    {
        properties_field_data_.region_values[name][point.region_attribute] = std::move(value);
    }
    const int location = scope == PropertyScope::Zone ? point.zone_attribute : point.region_attribute;
    computed_locations_[name].insert(location);
}

void PropertiesSystem::update_external(double time)
{
    properties_field_data_.global_values["time"] = StateValue{time};
    for (const PropertyInfo& property : properties_)
    {
        if (property.type != PropertyType::External)
        {
            continue;
        }
        const std::string& name = property.name;
        const auto assigned = assigned_models_.find(name);
        if (assigned == assigned_models_.end())
        {
            continue;
        }
        for (const auto& [attribute, model_name] : assigned->second)
        {
            const auto& required = external_model_manager_.required_states(name, model_name);
            const int location = property.scope == PropertyScope::Global ? 0 : attribute;
            if (computed_locations_[name].count(location) != 0 && !required_states_changed(attribute, required))
            {
                continue;
            }
            if (property.scope != PropertyScope::IntegrationPoint)
            {
                PropertiesStatePoint point = make_state_point(property.scope, attribute);
                load_required_states(point, required);
                evaluate_and_store(external_model_manager_, name, point);
                continue;
            }

            evaluate_and_store_points(external_model_manager_, property, attribute, required);
        }
    }
}

void PropertiesSystem::load_required_states(PropertiesStatePoint& point, const std::vector<std::string>& names,const std::optional<PropertiesFieldData::IntegrationPointKey>& key) const
{
    for (const std::string& name : names)
    {
        const PropertyScope scope = property_info(name).scope;
        if (scope == PropertyScope::Global)
        {
            point.values[name] = properties_field_data_.global_values.at(name).value();
        }
        else if (scope == PropertyScope::Region)
        {
            point.values[name] = properties_field_data_.region_values.at(name).at(point.region_attribute).value();
        }
        else if (scope == PropertyScope::Zone)
        {
            point.values[name] = properties_field_data_.zone_values.at(name).at(point.zone_attribute).value();
        }
        else
        {
            point.values[name] = properties_field_data_.integration_point_values.at(name).at(key.value()).value();
        }
    }
}

bool PropertiesSystem::required_states_changed(int attribute, const std::vector<std::string>& required_states) const
{
    const PropertiesFieldData& previous = required_state_snapshots_;
    const PropertiesFieldData& current = properties_field_data_;
    for (const std::string& name : required_states)
    {
        const PropertyScope scope = property_info(name).scope;
        if (scope == PropertyScope::Global)
        {
            const auto old_value = previous.global_values.find(name);
            const auto new_value = current.global_values.find(name);
            if (old_value == previous.global_values.end() || new_value == current.global_values.end() || old_value->second != new_value->second)
            {
                return true;
            }
        }
        else if (scope == PropertyScope::Region || scope == PropertyScope::Zone)
        {
            const auto& previous_values = scope == PropertyScope::Region ? previous.region_values : previous.zone_values;
            const auto& current_values = scope == PropertyScope::Region ? current.region_values : current.zone_values;
            const auto old_property = previous_values.find(name);
            const auto new_property = current_values.find(name);
            if (old_property == previous_values.end() || new_property == current_values.end())
            {
                return true;
            }
            const auto old_value = old_property->second.find(attribute);
            const auto new_value = new_property->second.find(attribute);
            if (old_value == old_property->second.end() || new_value == new_property->second.end() || old_value->second != new_value->second)
            {
                return true;
            }
        }
        else
        {
            const auto& old_values = previous.integration_point_values.at(name);
            const auto& new_values = current.integration_point_values.at(name);
            for (const int element : fem_system_.mesh_data().region_elements(attribute))
            {
                // 积分点表按单元编号排序, 只比较当前单元已有的积分点。
                for (auto point = new_values.lower_bound({element, 0, 0});
                     point != new_values.end() && std::get<0>(point->first) == element; ++point)
                {
                    const auto& [key, value] = *point;
                    if (old_values.at(key) != value)
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

template <typename Manager>
void PropertiesSystem::evaluate_and_store_points(const Manager& manager, const PropertyInfo& property, int region_attribute, const std::vector<std::string>& required, const FieldMap& trial_fields)
{
    const mfem::Mesh& mesh = fem_system_.mesh_data().mfem_mesh();
    std::vector<std::string> common_required;
    std::vector<std::string> point_required;
    std::vector<std::string> field_required;
    for (const std::string& required_name : required)
    {
        if (solution_field_states.count(required_name) != 0)
        {
            field_required.push_back(required_name);
            continue;
        }
        (property_info(required_name).scope == PropertyScope::IntegrationPoint ? point_required : common_required).push_back(required_name);
    }
    PropertiesStatePoint common_point;
    common_point.region_attribute = region_attribute;
    load_required_states(common_point, common_required);
    for (const int element : fem_system_.mesh_data().region_elements(region_attribute))
    {
        const mfem::Geometry::Type geometry = mesh.GetElementBaseGeometry(element);
        const mfem::IntegrationRule* rule = fem_system_.domain_quadrature_rule(geometry);
        mfem::IsoparametricTransformation transformation;
        mesh.GetElementTransformation(element, &transformation);
        mfem::Vector position(mesh.SpaceDimension());
        for (int point_index = 0; point_index < rule->GetNPoints(); ++point_index)
        {
            const mfem::IntegrationPoint& integration_point = rule->IntPoint(point_index);
            transformation.SetIntPoint(&integration_point);
            transformation.Transform(integration_point, position);
            const PropertiesFieldData::IntegrationPointKey key{element, rule->GetOrder(), point_index};

            PropertiesStatePoint point;
            point.region_attribute = region_attribute;
            point.x = position.Size() > 0 ? position[0] : 0.0;
            point.y = position.Size() > 1 ? position[1] : 0.0;
            point.z = position.Size() > 2 ? position[2] : 0.0;
            point.values = common_point.values;
            load_required_states(point, point_required, key);
            for (const std::string& name : field_required)
            {
                const auto trial = trial_fields.find(name);
                const mfem::GridFunction& field = trial != trial_fields.end()
                    ? *trial->second : fem_system_.field_data(name).mfem_grid_function();
                if (field.FESpace()->GetVDim() == 1)
                {
                    point.values[name] = field.GetValue(transformation, integration_point);
                }
                else
                {
                    mfem::Vector components;
                    field.GetVectorValue(transformation, integration_point, components);
                    point.values[name] = VectorValue(components.GetData(), components.GetData() + components.Size());
                }
            }
            properties_field_data_.integration_point_values[property.name][key] = manager.evaluate(property.name, point);
        }
    }
    computed_locations_[property.name].insert(region_attribute);
}

void PropertiesSystem::update_derived()
{
    for (const PropertyInfo& property : properties_)
    {
        if (property.type != PropertyType::Derived)
        {
            continue;
        }
        const std::string& name = property.name;
        const auto assigned = assigned_models_.find(name);
        if (assigned == assigned_models_.end())
        {
            continue;
        }
        for (const auto& [attribute, model_name] : assigned->second)
        {
            const auto& required = derived_model_manager_.required_states(name, model_name);
            const int location = property.scope == PropertyScope::Global ? 0 : attribute;
            if (computed_locations_[name].count(location) != 0 && !required_states_changed(attribute, required))
            {
                continue;
            }
            if (property.scope != PropertyScope::IntegrationPoint)
            {
                PropertiesStatePoint point = make_state_point(property.scope, attribute);
                load_required_states(point, required);
                evaluate_and_store(derived_model_manager_, name, point);
                continue;
            }

            evaluate_and_store_points(derived_model_manager_, property, attribute, required);
        }
    }
}

void PropertiesSystem::update_material()
{
    for (const PropertyInfo& property : properties_)
    {
        if (property.type != PropertyType::Material)
        {
            continue;
        }
        const std::string& name = property.name;
        const auto assigned = assigned_models_.find(name);
        if (assigned == assigned_models_.end())
        {
            continue;
        }
        for (const auto& [attribute, model_name] : assigned->second)
        {
            const int location = property.scope == PropertyScope::Global ? 0 : attribute;
            if (std::any_of(property.field_regions.begin(), property.field_regions.end(), [location](const auto& item)
            {
                return item.second.count(location) != 0;
            }))
            {
                continue;
            }
            const auto& required = material_model_manager_.required_states(name, model_name);
            if (computed_locations_[name].count(location) != 0 && !required_states_changed(attribute, required))
            {
                continue;
            }
            if (property.scope != PropertyScope::IntegrationPoint)
            {
                PropertiesStatePoint point = make_state_point(property.scope, attribute);
                load_required_states(point, required);
                evaluate_and_store(material_model_manager_, name, point);
                continue;
            }

            evaluate_and_store_points(material_model_manager_, property, attribute, required);
        }
    }
    required_state_snapshots_ = properties_field_data_;
}

void PropertiesSystem::update_field_dependent_materials(const std::string& field_name, const FieldMap& trial_fields, bool coupled_only)
{
    for (const PropertyInfo& property : properties_)
    {
        if (property.type != PropertyType::Material)
        {
            continue;
        }
        const auto assigned = assigned_models_.find(property.name);
        if (assigned == assigned_models_.end())
        {
            continue;
        }
        for (const auto& [region_attribute, model_name] : assigned->second)
        {
            if (!depends_on_field(property.name, region_attribute, field_name) ||
                (coupled_only && !coupled_with_field(property.name, region_attribute, field_name)))
            {
                continue;
            }
            const auto& required = material_model_manager_.required_states(property.name, model_name);
            evaluate_and_store_points(material_model_manager_, property, region_attribute, required, trial_fields);
        }
    }
}

bool PropertiesSystem::depends_on_field(const std::string& property_name, int region_attribute, const std::string& field_name) const
{
    const auto& tags = property_info(property_name).field_regions;
    const auto field = tags.find(field_name);
    return field != tags.end() && field->second.count(region_attribute) != 0;
}

bool PropertiesSystem::coupled_with_field(const std::string& property_name, int region_attribute, const std::string& field_name) const
{
    const auto& tags = property_info(property_name).coupled_regions;
    const auto field = tags.find(field_name);
    return field != tags.end() && field->second.count(region_attribute) != 0;
}

void PropertiesSystem::accept_current_state()
{
    previous_field_data_ = properties_field_data_;
}

const PropertiesFieldData& PropertiesSystem::previous_field_data() const
{
    return previous_field_data_;
}

PropertiesFieldData& PropertiesSystem::field_data()
{
    return properties_field_data_;
}

const PropertiesFieldData& PropertiesSystem::field_data() const
{
    return properties_field_data_;
}

}
