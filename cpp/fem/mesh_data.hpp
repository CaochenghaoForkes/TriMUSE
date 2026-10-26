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

    int dimension() const; // 网格单元拓扑维数 (问题的维数)

    int space_dimension() const; // 网格所在坐标空间的维数，即一个点的位置由几个分量描述，当前网格下该数值与网格单元拓扑维数一致

    int vertex_count() const; // 拓扑顶点数, 不等于高阶场的全部自由度数，不包含曲边中点等高阶几何节点 (高阶几何节点由 mesh.GetNodes() 管理)

    int element_count() const; // 域内单元总数，不包含边界单元

    int boundary_element_count() const; // 边界单元总数，不包含域内单元

    std::vector<int> region_attributes() const; // 网格中所有域区域属性编号的列表 (同一区域包含很多单元) 

    std::vector<int> boundary_attributes() const; // 网格中所有边界属性编号的列表 (内部材料界面也作为边界单元导入了 MFEM，它们的属性编号也会包含在里面；二维的对称轴同样可能包含在内)

    const std::vector<int>& region_elements(int region_attribute) const; // 指定材料区域的域内单元编号 

    const std::unordered_map<int, double>& region_volumes() const; // 各材料区域的实际体积, 单位 m³

    mfem::Mesh& mfem_mesh();

    const mfem::Mesh& mfem_mesh() const;

private:
    void build_region_elements(); // 建立体积区域编号与其中包含的域内单元编号列表的关系

    void calculate_region_volumes(); // 计算各个体积区域的体积

    mfem::Mesh mfem_mesh_;

    std::unordered_map<int, std::vector<int>> region_elements_; // 体积区域编号->体积区域中包含网格单元索引列表

    std::unordered_map<int, double> region_volumes_; // 体积区域编号->体积区域的体积
};

}
