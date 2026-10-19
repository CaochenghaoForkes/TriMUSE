#pragma once

#include <mfem.hpp>

#include "fem/finite_element_space_data.hpp"

namespace trimuse
{

class FieldData // 保存某个物理场在有限元空间上的当前系数向量
{
public:
    FieldData(FiniteElementSpaceData& finite_element_space, double initial_value = 0.0);

    int size() const; // 包含全部场分量的系数向量长度

    void assign(double value); // 所有系数赋相同值, 当前节点型 H1 空间对应常数场

    double value(int index) const; // 读取指定总体自由度的系数

    void set_value(int index, double value);

    mfem::GridFunction& mfem_grid_function(); // 供物理模块求值, 投影初始边界条件及写入求解结果

    const mfem::GridFunction& mfem_grid_function() const;

private:
    FiniteElementSpaceData& finite_element_space_; // 空间必须比场对象存活更久

    mfem::GridFunction grid_function_;
};

}
