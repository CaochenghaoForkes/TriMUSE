#include "fem/region_mesh_data.hpp"

#include <tuple>
#include <vector>

namespace trimuse
{

RegionMeshData::RegionMeshData(const mfem::Mesh& parent, int region_attribute) // 从原网格提取指定 region 的子网格，并建立原网格与子网格之间的编号映射
{
    mfem::Array<int> attributes(1); // MFEM 的区域提取接口接收一个属性编号数组 我们每次只提取一个 region，所以数组长度为 1
    attributes[0] = region_attribute; // 例如提取区域 3 : attributes = [3]
    local_faces_.SetSize(parent.GetNumFaces()); // 初始化“原网格面 → 子网格面”的映射 数组长度等于原网格的全部面数
    local_faces_ = -1; // 所有项先设为 -1 含义是：local_faces_[原网格面编号] = 子网格面编号; -1 表示该原网格面不属于这个区域子网格
    if (parent.Dimension() == 1)
    {
        // MFEM 4.8 的 SubMesh 点边界提取不支持 1D, 单独构造线网格。
        build_interval(parent, attributes);
    }
    else
    {
        // mfem::SubMesh::CreateFromDomain(parent, attributes) 从原网格提取区域子网格，该方法返回的是一个 mfem::SubMesh 对象，不是指针； parent: 原始网格 attributes：需要提取的域属性编号列表 (如 attributes=[3]; 就是提取原网格中属性编号为 3 的全部域单元，生成一个区域子网格)
        // std::make_unique<mfem::SubMesh>(上一步返回的对象) 创建由智能指针管理的子网格对象
        // 注意：使用 CreateFromDomain 这张表只映射域单元，也就是网格最高拓扑维度的单元
        auto submesh = std::make_unique<mfem::SubMesh>(mfem::SubMesh::CreateFromDomain(parent, attributes));
        parent_elements_ = submesh->GetParentElementIDMap(); // 把“子网格单元编号 → 原网格单元编号”的映射表，复制到成员变量 parent_elements_ 中；MFEM 提取子网格时会建立对应关系 如 子网格单元编号 0 1 2 对应原网格单元编号 5 8 12 赋值后 parent_elements_[0] == 5;...
        // 根据网格维数，取得“子网格面编号 → 原网格面编号”的映射表，并用只读引用 faces 访问它
        // 二维：域单元的边界是边，所以取得边映射
        // 三维：域单元的边界是面，所以取得面映射
        const auto& faces = parent.Dimension() == 2 ? submesh->GetParentEdgeIDMap() : submesh->GetParentFaceIDMap(); 
        for (int face = 0; face < faces.Size(); ++face) { local_faces_[faces[face]] = face; } // 把面编号映射反过来 faces[子网格面编号] = 原网格面编号; -> local_faces_[原网格面编号] = 子网格面编号; 不属于该子网格的面仍保持初始化值 -1
        mesh_ = std::move(submesh); // 把 submesh 智能指针拥有的子网格，转交给成员变量 mesh_ 管理
    }
    // 建立“原网格单元编号 → 当前区域子网格单元编号”的反向映射
    local_elements_.SetSize(parent.GetNE()); // 按原网格的域单元总数分配数组，因此可以用原网格单元编号作下标
    local_elements_ = -1; // 将全部元素设为 -1，表示暂时都标记为“不属于当前区域子网格”
    for (int element = 0; element < parent_elements_.Size(); ++element)
    {
        local_elements_[parent_elements_[element]] = element; // 先查出这个子单元对应的原单元编号，再登记反向关系
    }
}

void RegionMeshData::build_interval(const mfem::Mesh& parent, const mfem::Array<int>& attributes)
{
    mesh_ = std::make_unique<mfem::Mesh>(1, 0, 0, 0, parent.SpaceDimension()); // 创建一个空的一维网格对象，并由 mesh_ 智能指针管理。后面再往里面添加当前区域的顶点、线单元和边界点 Mesh(拓扑维数, 预分配顶点数, 预分配域单元数, 预分配边界单元数, 空间维数);
    mfem::Array<int> vertices; // 存储整数的 MFEM 动态数组 后续 vertices[子网格顶点编号] = 原网格顶点编号;
    // mfem::SubMeshUtils::AddElementsToMesh 把指定区域的单元及其顶点加入子网格，并接收返回的两张编号映射表
    // parent：原网格
    // *mesh_：正在构建的子网格对象
    // attributes：需要提取的区域编号列表
    // 这个函数会修改 *mesh_，加入选中的域单元和顶点，并返回一个包含两个数组的 std::tuple 第一个数组：子网格顶点编号 → 原网格顶点编号 第二个数组：子网格单元编号 → 原网格单元编号
    // std::tie(vertices, parent_elements_) = ...; 将这两个已有变量绑定起来，按顺序接收返回值 vertices ← 第一个数组 parent_elements_ ← 第二个数组
    std::tie(vertices, parent_elements_) = mfem::SubMeshUtils::AddElementsToMesh(parent, *mesh_, attributes);
    // 统计子网格中每个顶点被多少条线单元使用，以便后续找出区域边界点
    std::vector<int> counts(mesh_->GetNV(), 0); // 创建长度等于子网格顶点数的数组，全部初始化为 0
    for (int element = 0; element < mesh_->GetNE(); ++element)
    {
        const int* ends = mesh_->GetElement(element)->GetVertices(); // 获取当前线单元两个端点的子网格顶点编号 ends[0] 与 ends[1]
        ++counts[ends[0]];
        ++counts[ends[1]];
    }
    std::vector<int> boundary_attributes(parent.GetNV(), parent.bdr_attributes.Max() + 1); // 创建一个按原网格顶点编号索引的数组 默认值取原网格最大边界属性编号加 1 这是为子网格提取后新增、但原网格没有显式标记的边界点准备的编号，避免与原有编号冲突
    for (int boundary = 0; boundary < parent.GetNBE(); ++boundary) // 遍历原网格的全部边界单元 一维中，每个边界单元就是一个点
    {
        boundary_attributes[parent.GetBdrElement(boundary)->GetVertices()[0]] = parent.GetBdrAttribute(boundary); // GetBdrElement(boundary)：取得这个边界点单元 GetVertices()[0]：取得它唯一的原网格顶点编号 GetBdrAttribute(boundary)：取得该边界点的属性编号
    }
    for (int vertex = 0; vertex < vertices.Size(); ++vertex) // 遍历子网格顶点，这里的 vertex 是子网格顶点编号
    {
        local_faces_[vertices[vertex]] = vertex; // local_faces_[原顶点编号] = 子顶点编号;
        if (counts[vertex] == 1) { mesh_->AddBdrPoint(vertex, boundary_attributes[vertices[vertex]]); } // 如果某个子顶点只连接一个线单元，它就是这个区域的端点 AddBdrPoint(子网格顶点编号, 边界属性编号); 这里不是再创建一个新顶点，而是在已有顶点上添加一个“边界点单元” 边界属性优先继承原标识；原来没有标识的，则使用前面准备的默认编号
    }
    mesh_->FinalizeMesh(); // 根据已经加入的顶点、线单元和边界点，建立所需的连接关系并完成网格整理

    if (const auto* nodes = parent.GetNodes()) // 检查原网格是否有用有限元形式存储的几何坐标场,如果有的话需要把原网格的高阶几何映射保留到一维子网格中，取的取得原网格的几何节点 GridFunction
    {
        const auto* parent_space = nodes->FESpace(); // 取得这个几何坐标场使用的有限元空间，以便读取其阶次和分量排列方式
        // 为子网格建立高阶几何节点场
        // parent_space->GetMaxElementOrder(): 使用原几何空间的最大单元阶次
        // false: 采用连续的几何节点空间
        // parent.SpaceDimension(): 坐标分量数等于原网格的空间维数
        // parent_space->GetOrdering(): 使用与原几何空间相同的分量排列方式
        mesh_->SetCurvature(parent_space->GetMaxElementOrder(), false, parent.SpaceDimension(), parent_space->GetOrdering());
        auto* local_nodes = mesh_->GetNodes(); // 取得刚建立的子网格几何坐标场，接下来向里面写入准确的节点坐标
        mfem::Array<int> target_dofs; // 存储当前子单元的几何坐标场自由度编号，包括全部坐标分量
        mfem::Vector coordinates; // 临时存储当前单元待写入的坐标系数
        mfem::IsoparametricTransformation transformation; // 用于获取对应原单元的几何映射
        mfem::DenseMatrix positions; // 存储映射得到的实际坐标 positions(坐标分量, 节点编号)
        for (int element = 0; element < mesh_->GetNE(); ++element) // 遍历子单元所有域单元
        {
            const auto& points = local_nodes->FESpace()->GetFE(element)->GetNodes(); // 取得当前单元几何有限元的参考插值节点
            parent.GetElementTransformation(parent_elements_[element], &transformation); // 先通过 parent_elements_ 找到对应的原网格单元，再取得它的几何映射
            transformation.Transform(points, positions); // 一次性把全部参考插值节点映射到实际空间
            local_nodes->FESpace()->GetElementVDofs(element, target_dofs); // 取得当前单元所有坐标分量对应的自由度编号
            coordinates.SetSize(target_dofs.Size()); // 为待写入的数据分配相同长度的临时向量
            for (int component = 0; component < parent.SpaceDimension(); ++component)
            for (int point = 0; point < points.GetNPoints(); ++point)
            {
                coordinates[component * points.GetNPoints() + point] = positions(component, point); // 把坐标矩阵展开为当前单元自由度所需的分量分组形式
            }
            local_nodes->SetSubVector(target_dofs, coordinates); // 按 GetElementVDofs() 给出的局部编号顺序组织数值；整体存储位置由 target_dofs 决定
        }
    }
}

mfem::Mesh& RegionMeshData::mfem_mesh() // 返回当前区域的子网格对象的引用
{
    return *mesh_;
}

const mfem::Mesh& RegionMeshData::mfem_mesh() const // 返回当前区域的子网格对象的引用
{
    return *mesh_;
}

int RegionMeshData::parent_element(int local_element) const // 根据子网格域单元编号，返回对应的原网格域单元编号
{
    return parent_elements_[local_element];
}

int RegionMeshData::local_element(int parent_element) const // 反向查询：根据原网格域单元编号，返回它在当前子网格中的编号
{
    return local_elements_[parent_element]; 
}

int RegionMeshData::local_face(int parent_face) const // 根据原网格面编号，返回它在当前子网格中的面编号
{
    return local_faces_[parent_face];
}

}
