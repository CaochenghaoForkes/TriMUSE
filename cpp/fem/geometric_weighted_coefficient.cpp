#include "fem/geometric_weighted_coefficient.hpp"

namespace trimuse
{

GeometricWeightedCoefficient::GeometricWeightedCoefficient(mfem::Coefficient& coefficient, const FemSystem& fem_system)
    :
    coefficient_(coefficient),
    fem_system_(fem_system)
{
}

double GeometricWeightedCoefficient::Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point)
{
    return coefficient_.Eval(transformation, point) * fem_system_.geometric_weight(transformation, point);
}

}
