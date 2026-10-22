#include "physics/radioactivity/radioactivity_capacity_manager.hpp"

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

RadioactivityCapacityManager::RadioactivityCapacityManager(const FemSystem& fem_system)
    : fem_system_(fem_system), weighted_unit_coefficient_(unit_coefficient_, fem_system)
{
}

void RadioactivityCapacityManager::add_integrators(mfem::BilinearForm& capacity_form)
{
    capacity_form.AddDomainIntegrator(new DomainMassIntegrator(weighted_unit_coefficient_, fem_system_));
}

}
