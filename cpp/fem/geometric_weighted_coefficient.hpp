#pragma once

#include "fem/fem_system.hpp"

namespace trimuse
{

class GeometricWeightedCoefficient final : public mfem::Coefficient
{
public:
    GeometricWeightedCoefficient(mfem::Coefficient& coefficient, const FemSystem& fem_system);
    double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) override;

private:
    mfem::Coefficient& coefficient_;
    const FemSystem& fem_system_;
};

}
