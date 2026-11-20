#include "properties/derived/domain_quadrature_weight/undeformed.hpp"

namespace trimuse
{

UndeformedDomainQuadratureWeight::UndeformedDomainQuadratureWeight(const ModelParameterMap&)
{
}

StateValue UndeformedDomainQuadratureWeight::evaluate(const PropertiesStatePoint& point) const
{
    return point.scalar("precomputed.initial_domain_quadrature_weight");
}

}
