#include "properties/common/constant.hpp"

namespace trimuse
{

ConstantModel::ConstantModel(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters.at("constant_value"), value_type)
{
}

// 类型和分量数量由上层保证, 此处只组装状态值

ConstantModel::ConstantModel(const ModelParameterValue& value, const std::string& value_type)
{
    const VectorValue values = std::holds_alternative<double>(value)
        ? VectorValue{std::get<double>(value)} : std::get<VectorValue>(value);
    if (value_type == "scalar")
    {
        constant_value_ = values[0];
    }
    else if (value_type == "vector")
    {
        constant_value_ = values;
    }
    else if (value_type == "tensor2d")
    {
        Tensor2DValue tensor;
        for (int row = 0; row < 2; ++row)
        {
            for (int column = 0; column < 2; ++column)
            {
                tensor.data[row][column] = values[row * 2 + column];
            }
        }
        constant_value_ = tensor;
    }
    else if (value_type == "tensor3d")
    {
        Tensor3DValue tensor;
        for (int row = 0; row < 3; ++row)
        {
            for (int column = 0; column < 3; ++column)
            {
                tensor.data[row][column] = values[row * 3 + column];
            }
        }
        constant_value_ = tensor;
    }
}

StateValue ConstantModel::evaluate(const PropertiesStatePoint&) const
{
    return constant_value_;
}

}
