#include "fem/fem_system.hpp"

#include <algorithm>
#include <stdexcept>

namespace trimuse
{

FemSystem::FemSystem(const std::string& mesh_path, int domain_quadrature_order, int boundary_quadrature_order, const std::string& geometry_method, const std::array<double, 3>& center, const RegionLayerGeometryMap& region_layers)
    : mesh_data_(mesh_path),
      domain_quadrature_rules_(make_quadrature_rules(mesh_data_, mesh_data_.dimension(), domain_quadrature_order)), // 建立域中单元类型的求积规则表
      boundary_quadrature_rules_(make_quadrature_rules(mesh_data_, mesh_data_.dimension() - 1, boundary_quadrature_order)) // 建立边界中单元类型的求积规则表
{
    if (mesh_data_.dimension() < 3)
    {
        const mfem::Mesh& mesh = mesh_data_.mfem_mesh();
        radial_origin_x_ = mesh.GetVertex(0)[0];
        for (int vertex = 1; vertex < mesh.GetNV(); ++vertex)
        {
            radial_origin_x_ = std::min(radial_origin_x_, mesh.GetVertex(vertex)[0]); // 网格顶点中最小 x 坐标
        }
    }
    cache_domain_quadrature_positions(); // 缓存每个域单元上所有积分点的实际空间坐标，同时建立积分点的全域连续编号。 对固定网格，后续更新材料属性时就可以直接读取，不必重复计算坐标映射
    const MaterialCoordinateSystem coordinates(geometry_method, mesh_data_.dimension(), center, region_layers);
    material_bases_.resize(domain_quadrature_point_count());
    for (int element = 0; element < mesh_data_.element_count(); ++element)
    {
        const int region = mesh_data_.mfem_mesh().GetAttribute(element);
        const auto& positions = domain_quadrature_positions(element);
        const auto offset = domain_quadrature_offset(element);
        for (std::size_t point = 0; point < positions.size(); ++point)
        {
            material_bases_[offset + point] = coordinates.basis(region, positions[point]);
        }
    }
}

const Tensor3DValue& FemSystem::material_basis(std::size_t point_index) const
{
    return material_bases_.at(point_index);
}

FemSystem::FieldObjects::FieldObjects(MeshData& mesh_data, int order, int vector_dimension) // 创建一个场所需的对象组合，包含有限元空间(该场的有限元空间，确定插值阶次、分量数及自由度组织)，以及场数据(依附于上述空间，存储该场的全部自由度值)
    : finite_element_space(mesh_data, order, vector_dimension), // 先建立空间
      field_data(finite_element_space) // 再用这个空间初始化场数据
{
}

FemSystem::QuadratureRules FemSystem::make_quadrature_rules(const MeshData& mesh_data, int dimension, int order) // 据网格中指定维度的单元几何类型，建立并返回一张求积规则表
{
    QuadratureRules rules; // 映射：单元参考几何类型 → 求积规则指针 (如：三角形->三角形的求积规则[包含在参考三角形单元上的积分点坐标和权重])
    mfem::Array<mfem::Geometry::Type> geometries;
    mesh_data.mfem_mesh().GetGeometries(dimension, geometries); // 把这套网格中所有 dimension 单元涉及的几何类型，去重后填入 geometries
    for (const mfem::Geometry::Type geometry : geometries) // 为网格中每种单元几何类型获取对应的求积规则，并放进 rules 映射表
    {
        rules.emplace(geometry, &mfem::IntRules.Get(geometry, order)); // mfem::IntRules.Get(geometry, order) 从 MFEM 的求积规则库中获取适用于该参考几何、指定积分精度阶次 order 的规则
    }
    return rules;
}

void FemSystem::add_field(const std::string& name, int order, int vector_dimension) // 将名为 name 的场, 创建并初始化一个 FieldObjects 对象，然后返回拥有它的 unique_ptr 智能指针存入哈希表
{
    if (region_field_map_.count(name))
    {
        throw std::invalid_argument("场已使用分区域空间: " + name);
    }
    field_map_.emplace(name, std::make_unique<FieldObjects>(mesh_data_, order, vector_dimension));
}

void FemSystem::add_region_field(const std::string& name, int order, int vector_dimension) // 将名为 name 的场, 创建并初始化一个分域场对象，然后返回拥有它的 unique_ptr 智能指针存入哈希表
{
    if (field_map_.count(name))
    {
        throw std::invalid_argument("场已使用全域空间: " + name);
    }
    region_field_map_.emplace(name, std::make_unique<RegionFieldData>(mesh_data_, order, vector_dimension));
}

RegionFieldData& FemSystem::region_field(const std::string& name) // 返回名为 name 的场的子域场数据
{
    return *region_field_map_.at(name);
}

const RegionFieldData& FemSystem::region_field(const std::string& name) const // 返回名为 name 的场的子域场数据
{
    return *region_field_map_.at(name);
}

MeshData& FemSystem::mesh_data() // 返回场使用的网格
{
    return mesh_data_;
}

const MeshData& FemSystem::mesh_data() const // 返回场使用的网格
{
    return mesh_data_;
}

FiniteElementSpaceData& FemSystem::finite_element_space(const std::string& name) // 返回名为 name 的场的有限元空间
{
    return field_map_.at(name)->finite_element_space;
}

const FiniteElementSpaceData& FemSystem::finite_element_space(const std::string& name) const // 返回名为 name 的场的有限元空间
{
    return field_map_.at(name)->finite_element_space;
}

FieldData& FemSystem::field_data(const std::string& name) // 返回名为 name 的场的场数据
{
    return field_map_.at(name)->field_data;
}

const FieldData& FemSystem::field_data(const std::string& name) const // 返回名为 name 的场的场数据
{
    return field_map_.at(name)->field_data;
}

FieldView FemSystem::field_view(const std::string& name) const // 根据场名找到对应的连续场或分域场，并返回一个统一的 FieldView，不复制场数据
{
    const auto region = region_field_map_.find(name); // region 是迭代器，不是分域场对象
    if (region != region_field_map_.end()) // 如果找到了，说明这个场使用分区域空间
    {
        return region->second.get(); // region->second.get() 取得 std::unique_ptr<RegionFieldData>  但函数要求返回 FieldView 所以编译器会调用对应的转换构造函数 相当于 return FieldView(region->second.get());
    }
    return &field_data(name).mfem_grid_function(); // 如果没找到分域场 首先 field_data(name): 取得同名连续场的数据对象 然后 .mfem_grid_function(): 取得其中的 GridFunction 引用 最后调用 FieldView(const mfem::GridFunction*)，构造返回的视图
}

const mfem::IntegrationRule* FemSystem::domain_quadrature_rule(mfem::Geometry::Type geometry) const // 返回域内 geometry 型参考单元的求积规则
{
    return domain_quadrature_rules_.at(geometry);
}

const mfem::IntegrationRule* FemSystem::boundary_quadrature_rule(mfem::Geometry::Type geometry) const // 返回边界内 geometry 型参考单元的求积规则
{
    return boundary_quadrature_rules_.at(geometry);
}

void FemSystem::cache_domain_quadrature_positions() // 缓存每个域单元上所有积分点的实际空间坐标，同时建立积分点的全域连续编号。 对固定网格，后续更新材料属性时就可以直接读取，不必重复计算坐标映射
{
    const mfem::Mesh& mesh = mesh_data_.mfem_mesh(); // 得到网格
    domain_quadrature_positions_.resize(mesh.GetNE()); // GetNE() 是域单元总数,该容器按照单元存储坐标 domain_quadrature_positions_[单元编号][积分点编号] = 该位置的 {x,y,z}
    domain_quadrature_offsets_.resize(mesh.GetNE() + 1); // 它不仅存储每个单元的积分点起始位置，还要多存一个最后单元结束后的位置，也就是全域积分点总数 因此要 +1 (如：单元编号：0 1 2 积分点数量：3 4 2 offsets = [0,3,7,9]) 这里由于原始 vector 是空的所以新增的 std::size_t 元素都会初始化为 0
    mfem::Vector position(mesh.SpaceDimension()); // 网格所在坐标空间的维数
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        const mfem::IntegrationRule* rule = domain_quadrature_rule(mesh.GetElementBaseGeometry(element)); // 返回域内该形状参考单元的求积规则 mesh.GetElementBaseGeometry 根据实际网格中单元的索引返回其参考几何形状
        domain_quadrature_offsets_[element + 1] = domain_quadrature_offsets_[element] + rule->GetNPoints(); // 下一个单元的第一个积分点的索引等于上一个单元的第一个积分点的索引+下一个单元的总积分点数
        mfem::IsoparametricTransformation transformation; // 创建一个 MFEM 的等参几何变换对象(这里指它使用形函数插值几何映射的方式，不代表它强制几何阶次和温度、浓度的插值阶次相同；形函数插值几何映射的方式由单元形状类型、几何插值阶次及基函数类型、几何节点坐标决定，我们的程序中 Gmsh 生成并导出的网格携带这些几何信息[包括可能的二次几何等]，MFEM 读取后保存它们) 用于描述从参考单元坐标到实际网格单元坐标的映射
        mesh.GetElementTransformation(element, &transformation); // 根据该单元已有的几何信息设置映射，并不是根据温度等场的有限元阶次重新决定几何映射
        QuadraturePositions& positions = domain_quadrature_positions_[element];
        positions.resize(rule->GetNPoints()); // 用引用取出当前单元的坐标缓存，并为它分配与积分点数量相同的存储空间
        for (int point_index = 0; point_index < rule->GetNPoints(); ++point_index)
        {
            transformation.Transform(rule->IntPoint(point_index), position); // 取出第 point_index 个参考积分点，通过当前单元的几何映射，将实际坐标写入 position
            positions[point_index] = {position[0], position.Size() > 1 ? position[1] : 0.0, position.Size() > 2 ? position[2] : 0.0}; // 统一保存成三个分量
        }
    }
}

const FemSystem::QuadraturePositions& FemSystem::domain_quadrature_positions(int element) const // 返回指定单元上所有积分点的实际空间坐标 const auto& positions = fem_system.domain_quadrature_positions(element); double x = positions[q][0]; // 该单元第 q 个积分点的 x 坐标
{
    return domain_quadrature_positions_[element];
}

std::size_t FemSystem::domain_quadrature_offset(int element) const // 传入原网格单元编号返回指定单元的积分点在全域连续积分点数组中的起始位置 全域积分点偏移基于原网格的单元编号和求积规则，与场采用连续空间还是分域空间无关
{
    return domain_quadrature_offsets_[element];
}

std::size_t FemSystem::domain_quadrature_point_count() const // 返回所有域单元的积分点数量之和，通常用于给积分点属性数组分配空间
{
    return domain_quadrature_offsets_.back();
}

double FemSystem::geometric_weight(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) const // 返回降维模型积分时需要额外乘上的几何权重：一维球对称用 \(4\pi r^2\)，二维轴对称用 \(2\pi r\)，完整三维则用 \(1\)
{
    const int dimension = mesh_data_.dimension();
    if (dimension == 3)
    {
        return 1.0; // 完整三维网格已经直接对体积积分，不需要通过旋转或球对称补全体积
    }
    constexpr double pi = 3.14159265358979323846;
    double coordinates[3]; // 向量长度是实际空间维数
    mfem::Vector position(coordinates, mesh_data_.space_dimension()); // 创建一个 MFEM 向量，直接使用已有数组 coordinates 作为内存，不另行分配数据空间 coordinates：向量数据的存储地址 mesh_data_.space_dimension()：向量长度
    transformation.Transform(point, position); // 把当前参考积分点映射到实际空间，得到坐标
    const double radius = position[0] - radial_origin_x_; // 计算径向距离
    return dimension == 1 ? 4.0 * pi * radius * radius : 2.0 * pi * radius; // 根据问题维数返回对应的几何权重
}

}
