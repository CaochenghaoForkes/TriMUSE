#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "fem/mesh_data.hpp"
#include "fem/finite_element_space_data.hpp"
#include "fem/field_data.hpp"

namespace trimuse
{

class FemSystem
{
public:
    using QuadraturePositions = std::vector<std::array<double, 3>>; // 一个长度可变的列表，每个元素存储三个 double，用于表示一个积分点的 {x, y, z} 坐标

    FemSystem(const std::string& mesh_path, int domain_quadrature_order, int boundary_quadrature_order); // 初始化公共网格和求积规则

    FemSystem(FemSystem&&) = delete; // 内部空间引用公共网格, 管理对象保持固定地址

    FemSystem& operator=(FemSystem&&) = delete; // 内部空间引用公共网格, 管理对象保持固定地址

    void add_field(const std::string& name, int order, int vector_dimension = 1); // 将名为 name 的场, 创建并初始化一个 FieldObjects 对象，然后返回拥有它的 unique_ptr 智能指针,并存入哈希表

    MeshData& mesh_data(); // 返回可写网格数据

    const MeshData& mesh_data() const; // 返回可读网格数据

    FiniteElementSpaceData& finite_element_space(const std::string& name); // 返回某场可写有限元空间

    const FiniteElementSpaceData& finite_element_space(const std::string& name) const; // 返回某场可读有限元空间

    FieldData& field_data(const std::string& name); // 返回某场可写场数据

    const FieldData& field_data(const std::string& name) const; // 返回某场可读场数据

    const mfem::IntegrationRule* domain_quadrature_rule(mfem::Geometry::Type geometry) const; // 返回域内 geometry 型参考单元的求积规则

    const mfem::IntegrationRule* boundary_quadrature_rule(mfem::Geometry::Type geometry) const; // 返回边界内 geometry 型参考单元的求积规则

    const QuadraturePositions& domain_quadrature_positions(int element) const; // 返回指定单元上所有积分点的实际空间坐标 const auto& positions = fem_system.domain_quadrature_positions(element); double x = positions[q][0]; // 该单元第 q 个积分点的 x 坐标

    std::size_t domain_quadrature_offset(int element) const; // 返回指定单元的积分点在全域连续积分点数组中的起始位置

    std::size_t domain_quadrature_point_count() const; // 返回所有域单元的积分点数量之和，通常用于给积分点属性数组分配空间

    double geometric_weight(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) const; // 返回降维模型积分时需要额外乘上的几何权重：一维球对称用 \(4\pi r^2\)，二维轴对称用 \(2\pi r\)，完整三维则用 \(1\)

private:
    using QuadratureRules = std::map<mfem::Geometry::Type, const mfem::IntegrationRule*>; // 映射：单元参考几何类型 → 求积规则指针 (如：三角形->三角形的求积规则[包含在参考三角形单元上的积分点坐标和权重])

    static QuadratureRules make_quadrature_rules(const MeshData& mesh_data, int dimension, int order); // 据网格中指定维度的单元几何类型，建立并返回一张求积规则表

    void cache_domain_quadrature_positions(); // 缓存每个域单元上所有积分点的实际空间坐标，同时建立积分点的全域连续编号。 对固定网格，后续更新材料属性时就可以直接读取，不必重复计算坐标映射

    struct FieldObjects // 创建一个场所需的对象组合，包含有限元空间(该场的有限元空间，确定插值阶次、分量数及自由度组织)，以及场数据(依附于上述空间，存储该场的全部自由度值)
    {
        FieldObjects(MeshData& mesh_data, int order, int vector_dimension);

        FiniteElementSpaceData finite_element_space; // 先建立空间

        FieldData field_data; // 再利用这个空间初始化场数据
    };

    MeshData mesh_data_; // 网格最先构造, 最后销毁

    double radial_origin_x_ = 0.0; // 按当前几何约定, 最小顶点 x 为径向原点或旋转轴位置

    QuadratureRules domain_quadrature_rules_; // 规则由 MFEM 管理 全部场共用

    QuadratureRules boundary_quadrature_rules_;

    std::vector<QuadraturePositions> domain_quadrature_positions_; // 该容器按照单元存储坐标 domain_quadrature_positions_[单元编号][积分点编号] = 该位置的 {x,y,z}

    std::vector<std::size_t> domain_quadrature_offsets_; // 它不仅存储每个单元的积分点起始位置，还要多存一个最后单元结束后的位置，也就是全域积分点总数 因此要 +1 (如：单元编号：    0    1    2 积分点数量：  3    4    2 offsets = [0,   3,   7,   9])

    std::unordered_map<std::string, std::unique_ptr<FieldObjects>> field_map_; // 按场名称管理各场对象的哈希表 场名称->独占拥有一个 FieldObjects 的智能指针；该对象包含有限元空间和场数据
};

}
