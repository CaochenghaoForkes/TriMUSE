#include "physics/thermal/source/thermal_source_manager.hpp"
#include "physics/thermal/source/volumetric_power_source.hpp"

#include <stdexcept>

namespace trimuse
{

namespace
{

class DomainLFIntegrator final : public mfem::DomainLFIntegrator
{
public:
    DomainLFIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : mfem::DomainLFIntegrator(coefficient), fem_system_(fem_system) {}

    using mfem::DomainLFIntegrator::AssembleRHSElementVect;

    void AssembleRHSElementVect(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::Vector& vector) override
    {
        SetIntRule(fem_system_.domain_quadrature_rule(element.GetGeomType()));
        mfem::DomainLFIntegrator::AssembleRHSElementVect(element, transformation, vector);
    }

private:
    const FemSystem& fem_system_;
};

}

ThermalSourceManager::ThermalSourceManager(FemSystem& fem_system, const std::vector<ThermalSourceConfig>& source_configs)
    : fem_system_(fem_system),
      source_configs_(source_configs),
      region_markers_(source_configs.size()),
      source_coefficient_(fem_system.mesh_data().element_count()),
      weighted_source_coefficient_(source_coefficient_, fem_system)
{
    const mfem::Mesh& mesh = fem_system_.mesh_data().mfem_mesh();
    for (std::size_t index = 0; index < source_configs_.size(); ++index)
    {
        const int region_attribute = source_configs_[index].region_attribute;
        mfem::Array<int>& marker = region_markers_[index];
        marker.SetSize(mesh.attributes.Max());
        marker = 0;
        marker[region_attribute - 1] = 1;
        for (const int element : fem_system_.mesh_data().region_elements(region_attribute))
        {
            const mfem::IntegrationRule* rule = fem_system_.domain_quadrature_rule(mesh.GetElementBaseGeometry(element));
            source_coefficient_.values[element].resize(rule->GetNPoints(), 0.0);
        }
    }
}

void ThermalSourceManager::update(const PropertiesFieldData& properties_field_data)
{
    const mfem::Mesh& mesh = fem_system_.mesh_data().mfem_mesh();
    for (const ThermalSourceConfig& config : source_configs_)
    {
        if (config.model_name != "volumetric_power")
        {
            continue;
        }
        for (const int element : fem_system_.mesh_data().region_elements(config.region_attribute))
        {
            const mfem::IntegrationRule* rule = fem_system_.domain_quadrature_rule(mesh.GetElementBaseGeometry(element));
            for (int point_index = 0; point_index < rule->GetNPoints(); ++point_index)
            {
                const PropertiesFieldData::IntegrationPointKey key{element, rule->GetOrder(), point_index};
                source_coefficient_.values[element][point_index] = VolumetricPowerSource::evaluate(properties_field_data, key);
            }
        }
    }
}

void ThermalSourceManager::add_integrators(mfem::LinearForm& linear_form)
{
    for (std::size_t index = 0; index < source_configs_.size(); ++index)
    {
        if (source_configs_[index].model_name == "volumetric_power")
        {
            linear_form.AddDomainIntegrator(new DomainLFIntegrator(weighted_source_coefficient_, fem_system_), region_markers_[index]);
        }
        else
        {
            throw std::logic_error("尚未实现的热源模型: " + source_configs_[index].model_name);
        }
    }
}

}
