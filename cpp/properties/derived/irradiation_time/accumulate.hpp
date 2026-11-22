#pragma once

#include "properties/derived/irradiation_time/irradiation_time_model_manager.hpp"

namespace trimuse
{

class AccumulateIrradiationTime final : public IrradiationTimeModel
{
public:
    explicit AccumulateIrradiationTime(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;

private:
    double initial_value_;
};

}
