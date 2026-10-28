#pragma once

#include <mfem.hpp>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "fem/region_mesh_data.hpp"

namespace trimuse
{

class MeshData // 划分计算域 读取网格 得到各单元及连接关系 保留材料区域与边界的属性编号
{
public:
    struct InterfaceSide // 记录内部界面某一侧所属的在原网格中的区域和单元,一个内部界面有两侧,因此需要两个这样的记录
    {
        int region_attribute; // 这一侧属于哪个区域
        int element; // 这一侧紧邻界面的域单元编号 (原网格编号)
    };

    struct Interface // 记录一个内部界面网格面，以及它两侧的区域和单元 注意这是一小块网格面 不是整个交界面
    {
        int face; // 该界面在原网格中的面索引; 1D 为点, 2D 为边, 3D 为面
        int boundary_attribute; // 输入网格给这个界面指定的边界属性编号, 未标记时为 0, 一个完整的材料界面通常包含多个网格面，它们可以具有相同的 boundary_attribute，但各自的 face 不同
        std::array<InterfaceSide, 2> sides; // sides[0]：第一侧的原网格中区域编号和单元编号 sides[1]：第二侧的原网格中信息
    };

    explicit MeshData(const std::string& mesh_path);

    MeshData(MeshData&&) = delete; // 区域子网格保留对原网格的引用

    MeshData& operator=(MeshData&&) = delete;

    int dimension() const; // 网格单元拓扑维数 (问题的维数)

    int space_dimension() const; // 网格所在坐标空间的维数，即一个点的位置由几个分量描述，当前网格下该数值与网格单元拓扑维数一致

    int vertex_count() const; // 拓扑顶点数, 不等于高阶场的全部自由度数，不包含曲边中点等高阶几何节点 (高阶几何节点由 mesh.GetNodes() 管理)

    int element_count() const; // 域内单元总数，不包含边界单元

    int boundary_element_count() const; // 边界单元总数，不包含域内单元

    std::vector<int> region_attributes() const; // 网格中所有域区域属性编号的列表 (同一区域包含很多单元) 

    std::vector<int> boundary_attributes() const; // 网格中所有边界属性编号的列表 (内部材料界面也作为边界单元导入了 MFEM，它们的属性编号也会包含在里面；二维的对称轴同样可能包含在内)

    const std::vector<int>& region_elements(int region_attribute) const; // 指定材料区域的域内单元编号 

    RegionMeshData& region_mesh(int region_attribute); // 按区域编号取得子网格管理对象：没有就创建，有了就直接返回

    const std::vector<Interface>& interfaces() const; // 让外部代码只读访问已经建立的内部界面列表

    mfem::FaceElementTransformations& interface_transformations(int interface_index); // 获取某个内部界面片及其两侧单元的几何映射，方便后续在同一个界面积分点上计算两侧场值

    const std::unordered_map<int, double>& region_volumes() const; // 各材料区域的实际体积, 单位 m³

    mfem::Mesh& mfem_mesh();

    const mfem::Mesh& mfem_mesh() const;

private:
    void build_region_elements(); // 建立体积区域编号与其中包含的域内单元编号列表的关系

    void build_interfaces(); // 遍历网格中的所有面，找出两侧属于不同 region 的内部面，并把它们保存到 interfaces_

    void calculate_region_volumes(); // 计算各个体积区域的体积

    mfem::Mesh mfem_mesh_;

    std::unordered_map<int, std::vector<int>> region_elements_; // 体积区域编号->体积区域中包含网格单元索引列表

    std::unordered_map<int, double> region_volumes_; // 体积区域编号->体积区域的体积

    std::vector<Interface> interfaces_; // 网格中所有连接不同 region 的内部界面片 每个元素对应一个网格面，不是整个材料界面

    std::unordered_map<int, std::unique_ptr<RegionMeshData>> region_meshes_; // 区域编号 → unique_ptr<RegionMeshData>
};

}
