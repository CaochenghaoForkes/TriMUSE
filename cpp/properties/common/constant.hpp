#pragma once

#include "common/model_parameters.hpp"
#include "common/state_value.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class ConstantModel
{
public:
    explicit ConstantModel(const ModelParameterMap& parameters, const std::string& value_type = "scalar");

    ConstantModel(const ModelParameterValue& value, const std::string& value_type); // value_type: [scalar,vector,tensor2d,tensor3d], 张量分量按行排列

    StateValue evaluate(const PropertiesStatePoint& point) const;

private:
    StateValue constant_value_;
};

}
