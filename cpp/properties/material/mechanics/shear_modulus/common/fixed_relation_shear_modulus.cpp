#include "properties/material/mechanics/shear_modulus/common/fixed_relation_shear_modulus.hpp"

namespace trimuse
{

double FixedRelationShearModulus::evaluate(const PropertiesStatePoint& state_point) const
{
    const double young_modulus = state_point.scalar("material.mechanics.young_modulus");
    const double poisson_ratio = state_point.scalar("material.mechanics.poisson_ratio");
    return young_modulus / (2.0 * (1.0 + poisson_ratio));
}

}
