#include "physics/thermal/thermal_conduction_manager.hpp"

namespace trimuse
{

namespace
{

class DomainDiffusionIntegrator final : public mfem::DiffusionIntegrator // DiffusionIntegrator 用于积域内的\[K_{ij}=\int_\Omega (\nabla N_i)^{\mathrm T}k\,\nabla N_j\,\mathrm d\Omega\]
{
public:
    DomainDiffusionIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : mfem::DiffusionIntegrator(coefficient), fem_system_(fem_system) {}

    void AssembleElementMatrix(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::DenseMatrix& matrix) override
    {
        SetIntRule(fem_system_.domain_quadrature_rule(element.GetGeomType()));
        mfem::DiffusionIntegrator::AssembleElementMatrix(element, transformation, matrix);
    }

private:
    const FemSystem& fem_system_;
};

}

ThermalConductionManager::ThermalConductionManager(FemSystem& fem_system)
    : fem_system_(fem_system),
      conductivity_coefficient_(fem_system.mesh_data().element_count()),
      weighted_conductivity_coefficient_(conductivity_coefficient_, fem_system)
{
    const mfem::Mesh& mesh = fem_system_.mesh_data().mfem_mesh();
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        const mfem::IntegrationRule* rule = fem_system_.domain_quadrature_rule(mesh.GetElementBaseGeometry(element));
        conductivity_coefficient_.values[element].resize(rule->GetNPoints(), 0.0);
    }
}

void ThermalConductionManager::update(const PropertiesFieldData& properties_field_data)
{
    const mfem::Mesh& mesh = fem_system_.mesh_data().mfem_mesh();
    const auto& conductivity_values = properties_field_data.integration_point_values.at("thermal.conductivity");
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        const mfem::IntegrationRule* rule = fem_system_.domain_quadrature_rule(mesh.GetElementBaseGeometry(element));
        const std::size_t offset = fem_system_.domain_quadrature_offset(element);
        for (int point_index = 0; point_index < rule->GetNPoints(); ++point_index)
        {
            conductivity_coefficient_.values[element][point_index] = std::get<double>(conductivity_values[offset + point_index].value());
        }
    }
}

void ThermalConductionManager::add_integrators(mfem::BilinearForm& bilinear_form)
{
    bilinear_form.AddDomainIntegrator(new DomainDiffusionIntegrator(weighted_conductivity_coefficient_, fem_system_));
}

}
