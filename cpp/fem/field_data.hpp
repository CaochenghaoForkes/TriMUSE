#pragma once

#include <mfem.hpp>

#include "fem/finite_element_space_data.hpp"

namespace trimuse
{

class FieldData // 保存某个物理场在有限元空间上的当前系数向量
{
public:
    FieldData(FiniteElementSpaceData& finite_element_space, double initial_value = 0.0);

    int size() const; // 返回整个场自由度向量的长度 包含已知的 Dirichlet 边界自由度和待求解的未知自由度 对于向量场，包含所有分量的自由度

    void assign(double value); // 给全部自由度赋予一个相同的值

    double value(int index) const; // 是根据自由度索引，读取该自由度对应的值(节点上的温度或浓度或应力或应变)

    void set_value(int index, double value); // 是根据自由度索引，设置该自由度对应的值(节点上的温度或浓度或应力或应变)

    mfem::GridFunction& mfem_grid_function(); // 供物理模块求值, 投影初始边界条件及写入求解结果

    const mfem::GridFunction& mfem_grid_function() const;

private:
    FiniteElementSpaceData& finite_element_space_; // 空间必须比场对象存活更久

    mfem::GridFunction grid_function_; // grid_function 存储整个有限元空间中全部自由度值的向量(不仅包含未知节点,还包含施加Dirichlet条件的已知边界自由度)，同时它还关联着对应的有限元空间；它不只是普通向量：结合有限元空间，它能在单元内计算场值
};

}
