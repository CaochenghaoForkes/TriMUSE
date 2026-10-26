#include "fem/mesh_data.hpp"
#include "common/attributes_to_vector.hpp"

#include <algorithm>

namespace trimuse
{

MeshData::MeshData(const std::string& mesh_path)
    : mfem_mesh_(mesh_path, 1, 1, true) // 网格路径, 生成边, 准备细化数据, 修正反向单元
{
    build_region_elements();
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
