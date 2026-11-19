#include "common/state_value.hpp"

namespace trimuse
{

bool ElasticityTensorValue::operator==(const ElasticityTensorValue& other) const
{
    for (int row = 0; row < 6; ++row)
        for (int column = 0; column < 6; ++column)
        {
            if (data[row][column] != other.data[row][column])
            {
                return false;
            }
        }
    return true;
}

bool ElasticityTensorValue::operator!=(const ElasticityTensorValue& other) const
{
    return !(*this == other);
}

bool Tensor3DValue::operator==(const Tensor3DValue& other) const // 重载两个三维二阶张量的 == 运算符
{
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 3; ++column)
        {
            if (data[row][column] != other.data[row][column])
            {
                return false;
            }
        }
    }
    return true;
}

bool Tensor3DValue::operator!=(const Tensor3DValue& other) const // 重载两个三维二阶张量的 != 运算符
{
    return !(*this == other);
}

bool Tensor2DValue::operator==(const Tensor2DValue& other) const // 重载两个二维二阶张量的 == 运算符
{
    for (int row = 0; row < 2; ++row)
    {
        for (int column = 0; column < 2; ++column)
        {
            if (data[row][column] != other.data[row][column])
            {
                return false;
            }
        }
    }
    return true;
}

bool Tensor2DValue::operator!=(const Tensor2DValue& other) const // 重载两个二维二阶张量的 != 运算符
{
    return !(*this == other);
}

}
