#pragma once

#include <vector>

#include "common/model_parameters.hpp"
#include "common/state_value.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class TimeSeriesModel
{
public:
    // values 按时间逐样本排列; 向量和张量的各分量展平成一维列表, 张量按行排列。
    explicit TimeSeriesModel(const ModelParameterMap& parameters, const std::string& value_type = "scalar");

    StateValue evaluate(const PropertiesStatePoint& point) const;

private:
    std::vector<double> times_;
    
    std::vector<StateValue> values_;
};

}
