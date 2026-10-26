#include "physics/radioactivity/radioactivity_diffusion_manager.hpp"

namespace trimuse
{

namespace
{

class DomainDiffusionIntegrator final : public mfem::DiffusionIntegrator
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

RadioactivityDiffusionManager::RadioactivityDiffusionManager(FemSystem& fem_system)
    : fem_system_(fem_system),
      diffusion_coefficient_(fem_system.mesh_data().element_count()),
      weighted_diffusion_coefficient_(diffusion_coefficient_, fem_system)
{
    const mfem::Mesh& mesh = fem_system_.mesh_data().mfem_mesh();
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        const mfem::IntegrationRule* rule = fem_system_.domain_quadrature_rule(mesh.GetElementBaseGeometry(element));
        diffusion_coefficient_.values[element].resize(rule->GetNPoints(), 0.0);
    }
}

void RadioactivityDiffusionManager::update(const PropertiesFieldData& properties_field_data)
{
    const mfem::Mesh& mesh = fem_system_.mesh_data().mfem_mesh();
    const auto& values = properties_field_data.integration_point_values.at("material.radioactivity.nuclide_diffusion_coef");
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        const mfem::IntegrationRule* rule = fem_system_.domain_quadrature_rule(mesh.GetElementBaseGeometry(element));
        const std::size_t offset = fem_system_.domain_quadrature_offset(element);
        for (int point_index = 0; point_index < rule->GetNPoints(); ++point_index)
        {
            diffusion_coefficient_.values[element][point_index] = std::get<double>(values[offset + point_index].value());
        }
    }
}

void RadioactivityDiffusionManager::add_integrators(mfem::BilinearForm& bilinear_form)
{
    bilinear_form.AddDomainIntegrator(new DomainDiffusionIntegrator(weighted_diffusion_coefficient_, fem_system_));
}

}
