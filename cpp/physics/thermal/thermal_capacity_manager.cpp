#include "physics/thermal/thermal_capacity_manager.hpp"

namespace trimuse
{

namespace
{

class DomainMassIntegrator final : public mfem::MassIntegrator
{
public:
    DomainMassIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : mfem::MassIntegrator(coefficient), fem_system_(fem_system) {}

    void AssembleElementMatrix(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::DenseMatrix& matrix) override
    {
        SetIntRule(fem_system_.domain_quadrature_rule(element.GetGeomType()));
        mfem::MassIntegrator::AssembleElementMatrix(element, transformation, matrix);
    }

private:
    const FemSystem& fem_system_;
};

}

ThermalCapacityManager::ThermalCapacityManager(FemSystem& fem_system)
    : fem_system_(fem_system),
      capacity_coefficient_(fem_system.mesh_data().element_count()),
      weighted_capacity_coefficient_(capacity_coefficient_, fem_system)
{
    const mfem::Mesh& mesh = fem_system_.mesh_data().mfem_mesh();
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        const mfem::IntegrationRule* rule = fem_system_.domain_quadrature_rule(mesh.GetElementBaseGeometry(element));
        capacity_coefficient_.values[element].resize(rule->GetNPoints(), 0.0);
    }
}

void ThermalCapacityManager::update(const PropertiesFieldData& properties_field_data)
{
    const mfem::Mesh& mesh = fem_system_.mesh_data().mfem_mesh();
    const auto& density_values = properties_field_data.integration_point_values.at("general.mass_density");
    const auto& specific_heat_values = properties_field_data.integration_point_values.at("thermal.specific_heat");
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        const mfem::IntegrationRule* rule = fem_system_.domain_quadrature_rule(mesh.GetElementBaseGeometry(element));
        for (int point_index = 0; point_index < rule->GetNPoints(); ++point_index)
        {
            const PropertiesFieldData::IntegrationPointKey key{element, rule->GetOrder(), point_index};
            const double mass_density = std::get<double>(density_values.at(key).value());
            const double specific_heat = std::get<double>(specific_heat_values.at(key).value());
            capacity_coefficient_.values[element][point_index] = mass_density * specific_heat;
        }
    }
}

void ThermalCapacityManager::add_integrators(mfem::BilinearForm& capacity_form)
{
    capacity_form.AddDomainIntegrator(new DomainMassIntegrator(weighted_capacity_coefficient_, fem_system_));
}

}
