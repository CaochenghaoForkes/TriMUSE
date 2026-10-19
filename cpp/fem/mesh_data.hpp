#pragma once

#include <mfem.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace trimuse
{

class MeshData // 划分计算域 读取网格 得到各单元及连接关系 保留材料区域与边界的属性编号
{
public:
    explicit MeshData(const std::string& mesh_path);

    int dimension() const;

    int space_dimension() const;

    int vertex_count() const; // 拓扑顶点数, 不等于高阶场的全部自由度数

    int element_count() const;

    int boundary_element_count() const;

    std::vector<int> region_attributes() const; // Gmsh 最高维物理组编号对应 MFEM 区域属性编号

    std::vector<int> boundary_attributes() const;

    const std::unordered_map<int, double>& region_volumes() const; // 各材料区域的实际体积, 单位 m³

    mfem::Mesh& mfem_mesh();

    const mfem::Mesh& mfem_mesh() const;

private:
    void calculate_region_volumes();

    mfem::Mesh mfem_mesh_;

    std::unordered_map<int, double> region_volumes_;
};

}
