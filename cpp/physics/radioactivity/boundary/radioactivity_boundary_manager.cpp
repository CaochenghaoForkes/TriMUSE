#include "physics/radioactivity/boundary/radioactivity_boundary_manager.hpp"
#include "physics/radioactivity/boundary/radioactivity_robin_boundary.hpp"
#include "physics/radioactivity/boundary/radioactivity_neumann_boundary.hpp"
#include "physics/radioactivity/boundary/radioactivity_dirichlet_boundary.hpp"

#include <stdexcept>

namespace trimuse
{

namespace
{

class BoundaryMassIntegrator final : public mfem::MassIntegrator
{
public:
    BoundaryMassIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : mfem::MassIntegrator(coefficient), fem_system_(fem_system) {}

    void AssembleElementMatrix(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::DenseMatrix& matrix) override
    {
        SetIntRule(fem_system_.boundary_quadrature_rule(element.GetGeomType()));
        mfem::MassIntegrator::AssembleElementMatrix(element, transformation, matrix);
    }

private:
    const FemSystem& fem_system_;
};

class BoundaryLFIntegrator final : public mfem::BoundaryLFIntegrator
{
public:
    BoundaryLFIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : mfem::BoundaryLFIntegrator(coefficient), fem_system_(fem_system) {}

    using mfem::BoundaryLFIntegrator::AssembleRHSElementVect;

    void AssembleRHSElementVect(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::Vector& vector) override
    {
        SetIntRule(fem_system_.boundary_quadrature_rule(element.GetGeomType()));
        mfem::BoundaryLFIntegrator::AssembleRHSElementVect(element, transformation, vector);
    }

private:
    const FemSystem& fem_system_;
};

}

RadioactivityBoundaryManager::RadioactivityBoundaryManager(FemSystem& fem_system, const std::vector<RadioactivityBoundaryConfig>& boundary_configs)
    : fem_system_(fem_system),
      boundary_configs_(boundary_configs),
      boundary_markers_(boundary_configs.size())
{
    const int marker_size = fem_system_.mesh_data().mfem_mesh().bdr_attributes.Max();
    essential_boundary_marker_.SetSize(marker_size);
    essential_boundary_marker_ = 0;
    for (std::size_t index = 0; index < boundary_configs_.size(); ++index)
    {
        mfem::Array<int>& marker = boundary_markers_[index];
        marker.SetSize(marker_size);
        marker = 0;
        marker[boundary_configs_[index].boundary_attribute - 1] = 1;
        if (boundary_configs_[index].model_name == "dirichlet")
        {
            essential_boundary_marker_[boundary_configs_[index].boundary_attribute - 1] = 1;
        }
        boundary_coefficients_.push_back(std::make_unique<BoundaryCoefficients>(fem_system));
    }
}

void RadioactivityBoundaryManager::update(const PropertiesFieldData& properties_field_data)
{
    for (std::size_t index = 0; index < boundary_configs_.size(); ++index)
    {
        const RadioactivityBoundaryConfig& config = boundary_configs_[index];
        BoundaryCoefficients& coefficients = *boundary_coefficients_[index];
        if (config.model_name == "robin")
        {
            const RadioactivityRobinValues values = RadioactivityRobinBoundary::evaluate(properties_field_data, config.boundary_attribute);
            coefficients.matrix.constant = values.matrix_coefficient;
            coefficients.rhs.constant = values.rhs_coefficient;
        }
        else if (config.model_name == "neumann")
        {
            coefficients.rhs.constant = RadioactivityNeumannBoundary::evaluate(properties_field_data, config.boundary_attribute);
        }
        else if (config.model_name == "dirichlet")
        {
            coefficients.concentration.constant = RadioactivityDirichletBoundary::evaluate(properties_field_data, config.boundary_attribute);
        }
    }
}

void RadioactivityBoundaryManager::add_integrators(mfem::BilinearForm& bilinear_form)
{
    for (std::size_t index = 0; index < boundary_configs_.size(); ++index)
    {
        if (boundary_configs_[index].model_name == "robin")
        {
            bilinear_form.AddBoundaryIntegrator(new BoundaryMassIntegrator(boundary_coefficients_[index]->weighted_matrix, fem_system_), boundary_markers_[index]);
        }
    }
}

void RadioactivityBoundaryManager::add_integrators(mfem::LinearForm& linear_form)
{
    for (std::size_t index = 0; index < boundary_configs_.size(); ++index)
    {
        const std::string& model_name = boundary_configs_[index].model_name;
        if (model_name == "robin" || model_name == "neumann")
        {
            linear_form.AddBoundaryIntegrator(new BoundaryLFIntegrator(boundary_coefficients_[index]->weighted_rhs, fem_system_), boundary_markers_[index]);
        }
        else if (model_name != "dirichlet")
        {
            throw std::logic_error("尚未实现的核素边界模型: " + model_name);
        }
    }
}

void RadioactivityBoundaryManager::apply_essential_conditions(mfem::GridFunction& concentration, mfem::Array<int>& essential_true_dofs) const
{
    for (std::size_t index = 0; index < boundary_configs_.size(); ++index)
    {
        if (boundary_configs_[index].model_name == "dirichlet")
        {
            concentration.ProjectBdrCoefficient(boundary_coefficients_[index]->concentration, boundary_markers_[index]);
        }
    }
    concentration.FESpace()->GetEssentialTrueDofs(essential_boundary_marker_, essential_true_dofs);
}

}
