#include "common/linear_interpolation.hpp"

#include <algorithm>
#include <type_traits>

namespace trimuse
{

LinearInterpolator::Interval LinearInterpolator::find_interval(const std::vector<double>& coordinates, double coordinate)
{
    if (coordinate <= coordinates.front())
    {
        return {0, 0, 0.0}; 
    }
    if (coordinate >= coordinates.back())
    {
        const std::size_t last = coordinates.size() - 1;
        return {last, last, 0.0};
    }
    const auto upper = std::upper_bound(coordinates.begin(), coordinates.end(), coordinate);
    const std::size_t upper_index = upper - coordinates.begin();
    const std::size_t lower_index = upper_index - 1;
    const double fraction = (coordinate - coordinates[lower_index]) / (coordinates[upper_index] - coordinates[lower_index]);
    return {lower_index, upper_index, fraction};
}

double LinearInterpolator::evaluate(const std::vector<double>& coordinates, const std::vector<double>& values, double coordinate)
{
    const Interval interval = find_interval(coordinates, coordinate);
    if (interval.lower == interval.upper)
    {
        return values[interval.lower];
    }
    return values[interval.lower] + interval.fraction * (values[interval.upper] - values[interval.lower]);
}

StateValue LinearInterpolator::evaluate(const std::vector<double>& coordinates, const std::vector<StateValue>& values, double coordinate)
{
    const Interval interval = find_interval(coordinates, coordinate);
    if (interval.lower == interval.upper)
    {
        return values[interval.lower];
    }
    return interpolate(values[interval.lower], values[interval.upper], interval.fraction);
}

StateValue LinearInterpolator::interpolate(const StateValue& left, const StateValue& right, double fraction)
{
    return std::visit([&right, fraction](const auto& left_value) -> StateValue
    {
        using ValueType = std::decay_t<decltype(left_value)>;
        const ValueType& right_value = std::get<ValueType>(right);
        if constexpr (std::is_same_v<ValueType, double>)
        {
            return left_value + fraction * (right_value - left_value);
        }
        else if constexpr (std::is_same_v<ValueType, VectorValue>)
        {
            VectorValue result(left_value.size());
            for (std::size_t index = 0; index < result.size(); ++index)
            {
                result[index] = left_value[index] + fraction * (right_value[index] - left_value[index]);
            }
            return result;
        }
        else
        {
            ValueType result;
            constexpr std::size_t dimension = std::extent_v<decltype(left_value.data), 0>;
            for (std::size_t row = 0; row < dimension; ++row)
            {
                for (std::size_t column = 0; column < dimension; ++column)
                {
                    result.data[row][column] = left_value.data[row][column]
                        + fraction * (right_value.data[row][column] - left_value.data[row][column]);
                }
            }
            return result;
        }
    }, left);
}

}
