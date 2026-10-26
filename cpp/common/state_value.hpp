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

    bool operator==(const Tensor2DValue& other) const // 重载两个二维二阶张量的 == 运算符
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

    bool operator!=(const Tensor2DValue& other) const // 重载两个二维二阶张量的 != 运算符
    {
        return !(*this == other);
    }
};

// 三维二阶张量, 使用 data[row][column] 访问
struct Tensor3DValue
{
    double data[3][3] = {};

    bool operator==(const Tensor3DValue& other) const // 重载两个三维二阶张量的 == 运算符
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

    bool operator!=(const Tensor3DValue& other) const // 重载两个三维二阶张量的 != 运算符
    {
        return !(*this == other);
    }
};

// variant: 一个有明确候选清单、并记得当前内容类型的容器
using StateValue = std::variant<double, VectorValue, Tensor2DValue, Tensor3DValue>; // 单个时空点的值: 标量、向量、二维二阶张量或三维二阶张量

}
