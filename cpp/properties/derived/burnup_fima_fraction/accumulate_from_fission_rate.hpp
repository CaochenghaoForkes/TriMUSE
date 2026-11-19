#pragma once

#include "properties/derived/burnup_fima_fraction/burnup_fima_fraction_model_manager.hpp"

namespace trimuse
{

class AccumulateFromFissionRate final : public BurnupFimaFractionModel
{
public:
    explicit AccumulateFromFissionRate(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;

private:
    double initial_value_;
};

}
