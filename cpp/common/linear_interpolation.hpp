#pragma once

#include <cstddef>
#include <vector>

#include "common/state_value.hpp"

namespace trimuse
{

class LinearInterpolator
{
public:
    static double evaluate(const std::vector<double>& coordinates, const std::vector<double>& values, double coordinate);

    static StateValue evaluate(const std::vector<double>& coordinates, const std::vector<StateValue>& values, double coordinate);

private:
    struct Interval
    {
        std::size_t lower;
        std::size_t upper;
        double fraction;
    };

    static Interval find_interval(const std::vector<double>& coordinates, double coordinate);

    static StateValue interpolate(const StateValue& left, const StateValue& right, double fraction);
};

}
