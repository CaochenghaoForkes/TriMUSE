#include "properties/material/mechanics/shear_modulus/common/isotropy_shear_modulus.hpp"

namespace trimuse
{

double IsotropyShearModulus::evaluate(const PropertiesStatePoint& state_point) const
{
    const double young_modulus = state_point.vector("material.mechanics.young_modulus")[0];
    const double poisson_ratio = state_point.vector("material.mechanics.poisson_ratio")[0];
    return young_modulus / (2.0 * (1.0 + poisson_ratio));
}

}
