#pragma once

#include <variant>
#include <vector>

namespace trimuse
{

using VectorValue = std::vector<double>;

// 二维二阶张量, 使用 data[row][column] 访问
struct Tensor2DValue
{
    double data[2][2] = {};

    bool operator==(const Tensor2DValue& other) const
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

    bool operator!=(const Tensor2DValue& other) const
    {
        return !(*this == other);
    }
};

// 三维二阶张量, 使用 data[row][column] 访问
struct Tensor3DValue
{
    double data[3][3] = {};

    bool operator==(const Tensor3DValue& other) const
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

    bool operator!=(const Tensor3DValue& other) const
    {
        return !(*this == other);
    }
};

// 单个时空点的值, 与时间表或空间分布的数据来源分开
using StateValue = std::variant<double, VectorValue, Tensor2DValue, Tensor3DValue>;

}
