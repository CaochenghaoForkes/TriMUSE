#pragma once

#include <memory>
#include <mfem.hpp>

namespace trimuse
{

// 固定协调网格上的区域子网格, 保留原单元参考坐标及父子编号关系
class RegionMeshData
{
public:
    RegionMeshData(const mfem::Mesh& parent, int region_attribute); // // 从原网格提取指定 region 的子网格，并建立原网格与子网格之间的编号映射

    mfem::Mesh& mfem_mesh(); // 返回当前区域的子网格对象的引用

    const mfem::Mesh& mfem_mesh() const; // 返回当前区域的子网格对象的引用

    int parent_element(int local_element) const; // 根据子网格域单元编号，返回对应的原网格域单元编号

    int local_element(int parent_element) const; // 反向查询：根据原网格域单元编号，返回它在当前子网格中的编号

    int local_face(int parent_face) const; // 根据原网格面编号，返回它在当前子网格中的面编号

private:
    void build_interval(const mfem::Mesh& parent, const mfem::Array<int>& attributes);

    std::unique_ptr<mfem::Mesh> mesh_; // 成员变量 mesh_ 管理的是当前 region 的子网格对象，不是原网格
    mfem::Array<int> parent_elements_; // “子网格单元编号 → 原网格单元编号”的映射表 (只包含域单元)
    mfem::Array<int> local_elements_; // “原网格域单元 → 子网格域单元”的映射 数组长度等于原网格的全部单元数,不属于该区域的原始网格单元对应子网格单元的索引为-1
    mfem::Array<int> local_faces_; // “原网格面 → 子网格面”的映射 数组长度等于原网格的全部面数,不属于该区域的原始网格面对应子网格面的索引为-1
};

}
