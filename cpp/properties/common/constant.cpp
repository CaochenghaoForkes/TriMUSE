#include "properties/common/constant.hpp"

namespace trimuse
{

ConstantModel::ConstantModel(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters.at("constant_value"), value_type) // 委托构造 调用下面的构造函数
{
}

ConstantModel::ConstantModel(const ModelParameterValue& value, const std::string& value_type)
{
    // 如果 value 本就是 vector 就不需要变, 如果是 double 则改成 [value] 方便后面按标量、向量或张量处理
    const VectorValue values = std::holds_alternative<double>(value) ? VectorValue{std::get<double>(value)} : std::get<VectorValue>(value);
    if (value_type == "scalar") // constant_value_ 整理成标量的形式
    {
        constant_value_ = values[0];
    }
    else if (value_type == "vector") // constant_value_ 整理成向量的形式
    {
        constant_value_ = values;
    }
    else if (value_type == "tensor2d") // constant_value_ 整理成三维二阶张量的形式
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
    else if (value_type == "tensor3d") // constant_value_ 整理成二维二阶张量的形式
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
    return constant_value_; // 返回常值
}

}
