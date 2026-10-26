#include "fem/region_field_data.hpp"

namespace trimuse
{

RegionFieldData::RegionObjects::RegionObjects(RegionMeshData& mesh, int order, int vector_dimension, int block) // 把一个 region 对应的网格、有限元空间和场数据组织在一起
    : mesh(mesh), finite_element_space(mesh.mfem_mesh(), order, vector_dimension),
      field_data(finite_element_space), block(block)
{
}

RegionFieldData::RegionFieldData(MeshData& mesh_data, int order, int vector_dimension) // 为每个区域建立独立的空间和场数据，并计算各区域自由度在整体向量中的起始位置
    : mesh_data_(mesh_data)
{
    const auto attributes = mesh_data.region_attributes(); // 获取全部区域编号
    // 分配自由度偏移表 长度加 1，是为了让最后一项存放总自由度数
    dof_offsets_.SetSize(attributes.size() + 1); // 各区域完整存储自由度的起始位置，包含所有场分量
    true_dof_offsets_.SetSize(attributes.size() + 1); // 区域真实独立自由度的起始位置，处理有限元空间约束后计数，但还没有消去 Dirichlet 边界条件
    dof_offsets_[0] = true_dof_offsets_[0] = 0; // 两张表的第一项都设为 0，表示第一个区域块从整体向量的第 0 项开始
    for (int block = 0; block < static_cast<int>(attributes.size()); ++block) // 遍历区域列表 static_cast<int> 将列表长度转换为 int，与循环变量类型一致
    {
        const int attribute = attributes[block]; // 取得当前块对应的区域属性编号
        auto region = std::make_unique<RegionObjects>(mesh_data.region_mesh(attribute), order, vector_dimension, block); // 创建当前区域的对象组合：1. region_mesh(attribute)：取得区域子网格，没创建过则先创建 2. 在这个子网格上建立指定阶次、分量数的 H1 空间 3. 用该空间初始化 FieldData 4. 记录该区域的分块编号
        dof_offsets_[block + 1] = dof_offsets_[block] + region->finite_element_space.dof_count(); // 下一个区域块的起始位置，等于当前块的起始位置加当前块的自由度数
        true_dof_offsets_[block + 1] = true_dof_offsets_[block] + region->finite_element_space.true_dof_count(); // 真实自由度同理
        regions_.emplace(attribute, std::move(region)); // 将区域对象按区域属性编号保存到映射表中
    }
}

mfem::Mesh& RegionFieldData::region_mesh(int region_attribute) // 该区域子网格
{
    return regions_.at(region_attribute)->mesh.mfem_mesh();
}

const mfem::Mesh& RegionFieldData::region_mesh(int region_attribute) const // 该区域子网格
{
    return regions_.at(region_attribute)->mesh.mfem_mesh();
}

FiniteElementSpaceData& RegionFieldData::finite_element_space(int region_attribute) // 该区域有限元空间
{
    return regions_.at(region_attribute)->finite_element_space;
}

const FiniteElementSpaceData& RegionFieldData::finite_element_space(int region_attribute) const // 该区域有限元空间
{
    return regions_.at(region_attribute)->finite_element_space;
}

FieldData& RegionFieldData::field_data(int region_attribute) // 该区域场数据
{
    return regions_.at(region_attribute)->field_data;
}

const FieldData& RegionFieldData::field_data(int region_attribute) const // 该区域场数据
{
    return regions_.at(region_attribute)->field_data;
}

int RegionFieldData::local_element(int parent_element) const // 输入原网格单元编号，返回它在所属区域子网格中的局部单元编号
{
    const int attribute = mesh_data_.mfem_mesh().GetAttribute(parent_element);
    return regions_.at(attribute)->mesh.local_element(parent_element);
}

int RegionFieldData::local_face(int region_attribute, int parent_face) const // 输入区域属性编号和原网格面编号, 返回该面在指定区域子网格中的局部面编号, 不属于该区域则返回 -1
{
    return regions_.at(region_attribute)->mesh.local_face(parent_face);
}

const mfem::Array<int>& RegionFieldData::dof_offsets() const
{
    return dof_offsets_;
}

const mfem::Array<int>& RegionFieldData::true_dof_offsets() const
{
    return true_dof_offsets_;
}

void RegionFieldData::offset_dofs(int region_attribute, mfem::Array<int>& dofs) const // 把某一区域空间内的自由度编号，转换为整个分区域场的分块整体编号 它直接修改传入的 dofs 数组
{
    const int offset = dof_offsets_[regions_.at(region_attribute)->block]; // 先找到该区域的 block 编号，再查询它在整体向量中的起始位置
    for (int& dof : dofs)
    {
        dof = dof >= 0 ? dof + offset : dof - offset; // 普通非负编号直接加偏移量 我们常用的 H1 拉格朗日空间，通常得到的是非负编号；这里对负自由度 (不属于该子网格的自由度) 保留 MFEM 的通用有符号编号规则
    }
}

void RegionFieldData::element_dofs(int parent_element, mfem::Array<int>& dofs) const // 输入一个原网格域单元编号，取得该单元全部分量的自由度，并转换成整个分区域场的整体编号，写入 dofs
{
    const int attribute = mesh_data_.mfem_mesh().GetAttribute(parent_element);
    finite_element_space(attribute).mfem_space().GetElementVDofs(local_element(parent_element), dofs); // 1. 找到该区域的有限元空间 2. 将原单元编号转换为子网格单元编号 3. GetElementVDofs(...)：取得这个单元在区域有限元空间中的全部自由度编号，包含所有场分量，写入 dofs
    offset_dofs(attribute, dofs); // 再加上该区域块的偏移量，变成整个分区域场中的编号
}

void RegionFieldData::interface_dofs(const MeshData::Interface& interface, int side, mfem::Array<int>& dofs) const // 取得某个材料界面片指定一侧的面自由度，并转换为整个分区域场的整体编号，写入 dofs
{
    const int attribute = interface.sides[side].region_attribute; // 确定这一侧属于哪个区域
    finite_element_space(attribute).mfem_space().GetFaceVDofs(local_face(attribute, interface.face), dofs); // 1. 取得指定区域的 MFEM 有限元空间 2. 原网格面编号 → 该区域子网格中的面编号 3. 取得这个面在区域空间中的全部分量自由度编号
    // 两侧共享同一个原网格面编号 interface.face 但它在两个区域子网格中，各有自己的面编号 两个区域又分别建立了空间，因此两侧的自由度独立
    offset_dofs(attribute, dofs); // 注意：即使两侧节点的实际坐标重合，最终自由度编号仍不同，因此可以保存不同的浓度、温度或位移
}

void RegionFieldData::get_values(mfem::Vector& values) const // 把各区域分别保存的场自由度值，按区域块拼接成一个完整向量 values
{
    values.SetSize(dof_offsets_.Last()); // dof_offsets_ 的最后一项是所有区域的完整自由度总数，因此用它设置输出向量的长度
    for (const auto& [attribute, region] : regions_) // 遍历所有区域编号以及对应管理该区域 RegionObjects 的智能指针
    {
        const auto& field = region->field_data.mfem_grid_function(); // 取得该区域的 GridFunction
        for (int dof = 0; dof < field.Size(); ++dof)
        {
            values[dof_offsets_[region->block] + dof] = field[dof]; // 整体向量[区域起始位置 + 区域内编号] = 区域场值;
        }
    }
}

void RegionFieldData::set_values(const mfem::Vector& values) // 将整体分块向量里的场值，分别写回各区域的场数据
{
    for (auto& [attribute, region] : regions_)
    {
        auto& field = region->field_data.mfem_grid_function();
        for (int dof = 0; dof < field.Size(); ++dof)
        {
            field[dof] = values[dof_offsets_[region->block] + dof];
        }
    }
}

}
