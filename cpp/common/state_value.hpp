#pragma once

#include <string>
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

struct ElasticityTensorValue // 六分量弹性矩阵，顺序为 [11,22,33,23,13,12]；应变使用张量剪应变 sigma_V = C_V * epsilon_V；双点乘另需 W = diag(1,1,1,2,2,2)
{
    double data[6][6] = {};

    bool operator==(const ElasticityTensorValue& other) const
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

    bool operator!=(const ElasticityTensorValue& other) const
    {
        return !(*this == other);
    }
};

// variant: 一个有明确候选清单、并记得当前内容类型的容器
using StateValue = std::variant<double, VectorValue, Tensor2DValue, Tensor3DValue, ElasticityTensorValue, std::string>; // 单个时空点的值: 数值或预计算的文本信息

}
