#pragma once

#include "properties/derived/domain_quadrature_weight/domain_quadrature_weight_model_manager.hpp"

namespace trimuse
{

class UndeformedDomainQuadratureWeight final : public DomainQuadratureWeightModel
{
public:
    explicit UndeformedDomainQuadratureWeight(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
