#include "properties/material/mechanics/lame_lambda/common/fixed_relation_lame_lambda.hpp"

namespace trimuse
{

double FixedRelationLameLambda::evaluate(const PropertiesStatePoint& state_point) const
{
    const double young_modulus = state_point.scalar("material.mechanics.young_modulus");
    const double poisson_ratio = state_point.scalar("material.mechanics.poisson_ratio");
    return young_modulus * poisson_ratio / ((1.0 + poisson_ratio) * (1.0 - 2.0 * poisson_ratio));
}

}
