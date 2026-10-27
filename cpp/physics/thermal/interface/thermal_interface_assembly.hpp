#pragma once

#include "physics/thermal/interface/thermal_interface_context.hpp"

namespace trimuse
{

class ThermalInterfaceAssembly // 构建界面自由度约束
{
public:
    ThermalInterfaceAssembly(FemSystem& fem, const ThermalField& field);

    void connect(const ThermalInterfaceContext& context);

    std::unique_ptr<mfem::SparseMatrix> finalize();

private:
    int root(int dof);

    const ThermalField& field_; // 试探场

    std::unique_ptr<mfem::H1_FECollection> collection_; // 拥有一套辅助 H1 有限元集合，与分域场采用相同阶次，用于建立下面的父连续空间

    std::unique_ptr<mfem::FiniteElementSpace> parent_space_; // 拥有建立在原网格上的辅助连续有限元空间 它为共享界面上的同一自由度提供共同编号，用这个编号配对两侧分域自由度，避免比较浮点坐标

    std::vector<int> representative_; // 保存并查集的父节点编号，下标是分域场的完整自由度编号
};

}
