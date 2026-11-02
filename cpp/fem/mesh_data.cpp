#include "fem/mesh_data.hpp"
#include "common/attributes_to_vector.hpp"

#include <algorithm>
#include <stdexcept>

namespace trimuse
{

MeshData::MeshData(const std::string& mesh_path)
    : mfem_mesh_(mesh_path, 1, 1, true) // 网格路径, 生成边, 准备细化数据, 修正反向单元
{
    build_region_elements(); // 建立体积区域编号与其中包含的域内单元编号列表的关系
    build_interfaces(); // 遍历网格中的所有面，找出两侧属于不同 region 的内部面，并把它们保存到 interfaces_
    calculate_region_volumes();
}

int MeshData::dimension() const // 网格单元拓扑维数 (问题的维数)
{
    return mfem_mesh_.Dimension();
}

int MeshData::space_dimension() const // 网格所在坐标空间的维数，即一个点的位置由几个分量描述，当前网格下该数值与网格单元拓扑维数一致
{
    return mfem_mesh_.SpaceDimension();
}

int MeshData::vertex_count() const // 拓扑顶点数, 不等于高阶场的全部自由度数，不包含曲边中点等高阶几何节点 (高阶几何节点由 mesh.GetNodes() 管理)
{
    return mfem_mesh_.GetNV();
}

int MeshData::element_count() const // 域内单元总数，不包含边界单元
{
    return mfem_mesh_.GetNE();
}

int MeshData::boundary_element_count() const // 边界单元总数，不包含域内单元
{
    return mfem_mesh_.GetNBE();
}

std::vector<int> MeshData::region_attributes() const // 网格中所有域区域属性编号的列表 (同一区域包含很多单元) 
{
    return attributes_to_vector(mfem_mesh_.attributes);
}

std::vector<int> MeshData::boundary_attributes() const // 网格中所有边界属性编号的列表 (内部材料界面也作为边界单元导入了 MFEM，它们的属性编号也会包含在里面；二维的对称轴同样可能包含在内)
{
    return attributes_to_vector(mfem_mesh_.bdr_attributes);
}

const std::unordered_map<int, double>& MeshData::region_volumes() const // 各个体积区域的体积
{
    return region_volumes_;
}

const std::vector<int>& MeshData::region_elements(int region_attribute) const // 指定材料区域的域内单元编号 
{
    return region_elements_.at(region_attribute);
}

RegionMeshData& MeshData::region_mesh(int region_attribute) // 按区域编号取得子网格管理对象：没有就创建，有了就直接返回
{
    auto& mesh = region_meshes_[region_attribute]; 
    if (!mesh) // 判断智能指针是否为空 则需要初始化
    {
        if (mfem_mesh_.Nonconforming()) // 当前实现不支持带悬挂节点等非协调连接的网格，所以明确报错
        {
            throw std::invalid_argument("分区域空间当前仅支持协调网格");
        }
        if (!region_elements_.count(region_attribute))
        {
            throw std::invalid_argument("不存在的区域编号: " + std::to_string(region_attribute));
        }
        mesh = std::make_unique<RegionMeshData>(mfem_mesh_, region_attribute); // 创建并保存区域子网格
    }
    return *mesh; // 返回管理对象的引用
}

const std::vector<MeshData::Interface>& MeshData::interfaces() const // 让外部代码只读访问已经建立的内部界面列表
{
    return interfaces_;
}

mfem::FaceElementTransformations& MeshData::interface_transformations(int interface_index) // 获取某个内部界面片及其两侧单元的几何映射，方便后续在同一个界面积分点上计算两侧场值
{
    // interfaces_[interface_index].face 先根据 interfaces_ 列表中的索引，取得这个界面片的原网格面编号
    // mfem_mesh_.GetFaceElementTransformations() 让 MFEM 获取该面的双侧几何变换对象，其中主要包含：
    // Face：参考面 → 实际空间的映射
    // Elem1、Elem2：两侧参考单元 → 实际空间的映射
    // Loc1、Loc2：同一个参考面积分点 → 两侧参考单元中的对应坐标
    // Elem1No、Elem2No：两侧的原网格单元编号
    return *mfem_mesh_.GetFaceElementTransformations(interfaces_[interface_index].face); 
}

void MeshData::build_interfaces() // 遍历网格中的所有面，找出两侧属于不同 region 的内部面，并把它们保存到 interfaces_
{
    std::vector<int> boundary_attributes(mfem_mesh_.GetNumFaces(), 0); // 创建一个长度等于网格面总数的数组，全部初始化为 0， 注意：即使两个域内单元都属于 kernel，它们之间的公共面仍然是一个网格面，并有自己的 face 索引；公共面只计一次
    for (int boundary = 0; boundary < mfem_mesh_.GetNBE(); ++boundary) // 遍历所有边界单元 这里也可能包含输入网格中显式标记的内部材料界面，不只是外边界 注意：边界单元本身已经是 1d点 2d线 3d片了
    {
        boundary_attributes[mfem_mesh_.GetBdrElementFaceIndex(boundary)] = mfem_mesh_.GetBdrAttribute(boundary); // GetBdrElementFaceIndex(boundary) 这个边界单元对应原网格中全部面中的索引 GetBdrAttribute(boundary) 这个边界单元的边界属性编号(如 xml 中定义的 interfacei 的对应的编号) 例如，边界单元 5 对应面 12，属性编号为 41 相当于 boundary_attributes[12] = 41;
    }
    // boundary_attributes 记录了 原始网格面单元索引 -> 人为定义的交界面 interface 的 id (对于非材料界面的 face 其对应 0)
    for (int face = 0; face < mfem_mesh_.GetNumFaces(); ++face) // 遍历全部网格面 包括外边界面、同区域内部面，以及不同区域之间的界面
    {
        int first, second; // 声明两个变量，用于接收这个面两侧的域单元编号 
        mfem_mesh_.GetFaceElements(face, &first, &second); // 查询当前面连接的两个域单元，将编号写入 first、second
        if (second < 0 || mfem_mesh_.GetAttribute(first) == mfem_mesh_.GetAttribute(second)) // second<0 没有第二侧域单元，是外边界面 两侧单元的区域属性相同：属于同一 region 内部，不是区域界面
        {
            continue; // 跳过不是材料界面的面
        }
        const InterfaceSide first_side{mfem_mesh_.GetAttribute(first), first}; // 创建第一侧记录 包含 {第一侧的区域编号, 第一侧的原网格单元编号}
        const InterfaceSide second_side{mfem_mesh_.GetAttribute(second), second}; // 创建第二侧记录 包含 {第二侧的区域编号, 第二侧的原网格单元编号}
        interfaces_.push_back({face, boundary_attributes[face], {first_side, second_side}}); // 构造一个 Interface 对象，并追加到 interfaces_ {当前面的原网格编号,当前面的边界标识,{第一侧信息, 第二侧信息}}
    }
}

void MeshData::build_region_elements()
{
    region_elements_.clear();
    for (int element = 0; element < mfem_mesh_.GetNE(); ++element)
    {
        region_elements_[mfem_mesh_.GetAttribute(element)].push_back(element); // mfem_mesh_.GetAttribute(element) 根据域内单元索引取得该单元的区域属性编号
    }
}

void MeshData::calculate_region_volumes()
{
    constexpr double pi = 3.14159265358979323846;
    mfem::Mesh& mesh = mfem_mesh_;
    const int dimension = mesh.Dimension();
    mfem::Vector position(mesh.SpaceDimension()); // 创建长度等于网格所在空间维数的向量用来存储实际坐标
    double radial_origin_x = 0.0; // 初始化网格顶点中最小 x 坐标
    if (dimension < 3) // 三维不需要旋转权重，没这么复杂
    {
        radial_origin_x = mesh.GetVertex(0)[0]; // mesh.GetVertex(0)取网格第0个顶点的坐标 [0]取它的 x 分量
        for (int vertex = 1; vertex < mesh.GetNV(); ++vertex) // 遍历其余顶点 GetNV() 是网格顶点总数
        {
            radial_origin_x = std::min(radial_origin_x, mesh.GetVertex(vertex)[0]); // 持续更新最小值 
        }
    }

    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        // 单元积分 \[\int_{\Omega_e} g(\boldsymbol x)\,\mathrm dV=\int_{\hat\Omega}g\!\left(\boldsymbol x(\boldsymbol\zeta)\right)|J(\boldsymbol\zeta)|\,\mathrm d\hat V.\]
        mfem::ElementTransformation* transformation = mesh.GetElementTransformation(element); // transformation 参考单元坐标向网格单元实际位置坐标的映射对象 x=x_e(\zeta)
        const int integration_order = 2 * transformation->OrderW() + 4; // OrderW() 当前单元几何映射的雅可比权重 (当前参考单元维数与实际坐标维数相同: det(J) 或 如三维空间中的二维曲面时: sqrt[det(J^TJ)]) 的多项式阶次估计(雅可比的行列式是关于参考坐标\zeta\eta\psi的多项式，这里估计\zeta\eta\psi构成项的最高次阶),考虑到真正的被积函数还可能有2*\pi*r或4\pi*r**2，所以还加了4以及2倍关系
        const mfem::IntegrationRule& rule = mfem::IntRules.Get(mesh.GetElementBaseGeometry(element), integration_order); // mesh.GetElementBaseGeometry(element) 当前积单元的基本形状, IntRules 返回的 rule 包含该形状该阶次精度积分的参考坐标下的积分点与权重

        for (int point_index = 0; point_index < rule.GetNPoints(); ++point_index)
        {
            const mfem::IntegrationPoint& point = rule.IntPoint(point_index); // 只读引用访问当前求积规则中的第 point_index 个积分点，信息包含其参考单元坐标和求积权
            transformation->SetIntPoint(&point); // 告诉 transformation，接下来要在参考单元的这个积分点计算雅可比等几何量
            double weight = point.weight * transformation->Weight(); // w_q * 雅可比权重 (把参考单元的权重转换到实际单元上) transformation->Weight() 是 det(J) 或 sqrt[det(J^TJ)]

            if (dimension < 3)
            {
                transformation->Transform(point, position); // Transform 利用当前单元的几何映射，把 point 的参考单元坐标换成实际坐标，并写入 position
                const double radius = position[0] - radial_origin_x; // 当前积分点与旋转轴的距离
                weight *= dimension == 1 ? 4.0 * pi * radius * radius : 2.0 * pi * radius; // 1D/2D 需要乘对称权重因子
            }
            region_volumes_[mesh.GetAttribute(element)] += weight; // 该体积区域总体积 += 该单元中积分点的贡献
        }
    }
}

mfem::Mesh& MeshData::mfem_mesh()
{
    return mfem_mesh_;
}

const mfem::Mesh& MeshData::mfem_mesh() const
{
    return mfem_mesh_;
}

}
