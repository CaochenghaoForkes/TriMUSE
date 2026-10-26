#pragma once

#include <map>
#include <memory>

#include "fem/field_data.hpp"

namespace trimuse
{

// 区域内 H1 连续, 区域之间独立; 不在此处规定界面传递或连续性条件。
class RegionFieldData
{
public:
    RegionFieldData(MeshData& mesh_data, int order, int vector_dimension = 1); // 把一个 region 对应的网格、有限元空间和场数据组织在一起

    mfem::Mesh& region_mesh(int region_attribute); // 该区域子网格

    const mfem::Mesh& region_mesh(int region_attribute) const; // 该区域子网格

    FiniteElementSpaceData& finite_element_space(int region_attribute); // 该区域有限元空间

    const FiniteElementSpaceData& finite_element_space(int region_attribute) const; // 该区域有限元空间

    FieldData& field_data(int region_attribute); // 该区域场数据
 
    const FieldData& field_data(int region_attribute) const; // 该区域场数据

    int local_element(int parent_element) const; // 输入原网格单元编号，返回它在所属区域子网格中的局部单元编号

    int local_face(int region_attribute, int parent_face) const; // 输入区域属性编号和原网格面编号, 返回该面在指定区域子网格中的局部面编号, 不属于该区域则返回 -1

    const mfem::Array<int>& dof_offsets() const; // 按 region_attributes() 顺序排列, 最后一项为总自由度数

    const mfem::Array<int>& true_dof_offsets() const;

    void element_dofs(int parent_element, mfem::Array<int>& dofs) const; // 输入一个原网格域单元编号，取得该单元全部分量的自由度，并转换成整个分区域场的整体编号，写入 dofs

    void interface_dofs(const MeshData::Interface& interface, int side, mfem::Array<int>& dofs) const; // 取得某个材料界面片指定一侧的面自由度，并转换为整个分区域场的整体编号，写入 dofs

    void get_values(mfem::Vector& values) const; // 把各区域分别保存的场自由度值，按区域块拼接成一个完整向量 values

    void set_values(const mfem::Vector& values); // 将整体分块向量里的场值，分别写回各区域的场数据

private:
    struct RegionObjects // 把一个 region 对应的网格、有限元空间和场数据组织在一起
    {
        // 接收区域网格、场插值阶次、场分量数和分块编号
        RegionObjects(RegionMeshData& mesh, int order, int vector_dimension, int block);
        RegionMeshData& mesh; // 引用该区域的子网格管理对象
        FiniteElementSpaceData finite_element_space; // 在该区域子网格上建立的 H1 有限元空间 不同区域分别建立空间，因此界面两侧自由度独立
        FieldData field_data; // 保存该区域场的自由度值，依附于上面的有限元空间
        int block; // 该区域在整个场的分块向量中的块编号，不是区域属性编号，也不是自由度偏移量
    };

    void offset_dofs(int region_attribute, mfem::Array<int>& dofs) const; // 把某一区域空间内的自由度编号，转换为整个分区域场的分块整体编号 它直接修改传入的 dofs 数组

    MeshData& mesh_data_; // 原始网格
    std::map<int, std::unique_ptr<RegionObjects>> regions_; // 区域属性编号 → RegionObjects，独占管理各区域的有限元空间、场数据等
    mfem::Array<int> dof_offsets_; // 各区域完整自由度块的起始位置，包含全部分量及已知边界自由度
    mfem::Array<int> true_dof_offsets_; // 各区域真实独立自由度块的起始位置，考虑空间约束，但尚未消去 Dirichlet 自由度
};

}
