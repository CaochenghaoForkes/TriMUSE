#pragma once

#include <vector>

#include "common/state_value.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class TimeSeriesExternalState
{
public:
    TimeSeriesExternalState(const std::vector<double>& times, const std::vector<StateValue>& values);

    StateValue evaluate(const PropertiesStatePoint& point) const;

private:
    std::vector<double> times_;
    
    std::vector<StateValue> values_;
};

}
