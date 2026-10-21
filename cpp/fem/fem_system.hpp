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
    using QuadraturePositions = std::vector<std::array<double, 3>>;

    FemSystem(const std::string& mesh_path, int domain_quadrature_order, int boundary_quadrature_order); // 初始化公共网格和求积规则

    FemSystem(FemSystem&&) = delete; // 内部空间引用公共网格, 管理对象保持固定地址

    FemSystem& operator=(FemSystem&&) = delete; // 内部空间引用公共网格, 管理对象保持固定地址

    void add_field(const std::string& name, int order, int vector_dimension = 1); // 按求解变量名(如 temperature)创建有限元空间与场数据

    MeshData& mesh_data(); // 返回可写网格数据

    const MeshData& mesh_data() const; // 返回可读网格数据

    FiniteElementSpaceData& finite_element_space(const std::string& name); // 返回某场可写有限元空间

    const FiniteElementSpaceData& finite_element_space(const std::string& name) const; // 返回某场可读有限元空间

    FieldData& field_data(const std::string& name); // 返回某场可写场数据

    const FieldData& field_data(const std::string& name) const; // 返回某场可读场数据

    const mfem::IntegrationRule* domain_quadrature_rule(mfem::Geometry::Type geometry) const;

    const mfem::IntegrationRule* boundary_quadrature_rule(mfem::Geometry::Type geometry) const;

    const QuadraturePositions& domain_quadrature_positions(int element) const; // 按局部积分点编号读取实际 xyz, 低维缺省分量为 0

    std::size_t domain_quadrature_offset(int element) const; // 单元积分点在全域连续数组中的起始偏移, element=单元数时返回总数

    std::size_t domain_quadrature_point_count() const;

    double geometric_weight(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) const; // 1D 球对称, 2D 轴对称, 3D 原始体积或面积积分

private:
    using QuadratureRules = std::map<mfem::Geometry::Type, const mfem::IntegrationRule*>;

    static QuadratureRules make_quadrature_rules(const MeshData& mesh_data, int dimension, int order);

    void cache_domain_quadrature_positions();

    struct FieldObjects // 含有限元空间与场数据
    {
        FieldObjects(MeshData& mesh_data, int order, int vector_dimension);

        FiniteElementSpaceData finite_element_space; // 成员按声明顺序构造, 反序销毁, 场数据先于空间释放

        FieldData field_data; // 场数据
    };

    // 网格最先构造, 最后销毁
    MeshData mesh_data_;

    double radial_origin_x_ = 0.0; // 按当前几何约定, 最小顶点 x 为径向原点或旋转轴位置

    QuadratureRules domain_quadrature_rules_; // 规则由 MFEM 管理 全部场共用

    QuadratureRules boundary_quadrature_rules_;

    std::vector<QuadraturePositions> domain_quadrature_positions_; // 固定网格和域求积规则下, 初始化一次, 全部场共用

    std::vector<std::size_t> domain_quadrature_offsets_; // 长度为单元数+1, 最后一项是全域积分点总数

    std::unordered_map<std::string, std::unique_ptr<FieldObjects>> field_map_;
};

}
