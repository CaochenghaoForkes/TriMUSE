#pragma once

#include <cstddef>

namespace trimuse
{

// result = left * right；数组尺寸由编译器推导，结果写入独立的输出数组
template <std::size_t Rows, std::size_t Inner, std::size_t Columns>
void matrix_multiply(const double (&left)[Rows][Inner], const double (&right)[Inner][Columns], double (&result)[Rows][Columns])
{
    for (std::size_t i = 0; i < Rows; ++i)
    for (std::size_t j = 0; j < Columns; ++j)
    {
        double value = 0.0;
        for (std::size_t k = 0; k < Inner; ++k)
        {
            value += left[i][k] * right[k][j];
        }
        result[i][j] = value;
    }
}

}
