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

int MeshData::dimension() const
{
    return mfem_mesh_.Dimension();
}

int MeshData::space_dimension() const
{
    return mfem_mesh_.SpaceDimension();
}

int MeshData::vertex_count() const
{
    return mfem_mesh_.GetNV();
}

int MeshData::element_count() const
{
    return mfem_mesh_.GetNE();
}

int MeshData::boundary_element_count() const
{
    return mfem_mesh_.GetNBE();
}

std::vector<int> MeshData::region_attributes() const
{
    return attributes_to_vector(mfem_mesh_.attributes);
}

std::vector<int> MeshData::boundary_attributes() const
{
    return attributes_to_vector(mfem_mesh_.bdr_attributes);
}

const std::unordered_map<int, double>& MeshData::region_volumes() const
{
    return region_volumes_;
}

const std::vector<int>& MeshData::region_elements(int region_attribute) const
{
    return region_elements_.at(region_attribute);
}

void MeshData::build_region_elements()
{
    region_elements_.clear();
    for (int element = 0; element < mfem_mesh_.GetNE(); ++element)
    {
        region_elements_[mfem_mesh_.GetAttribute(element)].push_back(element);
    }
}

void MeshData::calculate_region_volumes()
{
    constexpr double pi = 3.14159265358979323846;
    mfem::Mesh& mesh = mfem_mesh_;
    const int dimension = mesh.Dimension();
    mfem::Vector position(mesh.SpaceDimension());
    double radial_origin_x = 0.0;
    if (dimension < 3)
    {
        radial_origin_x = mesh.GetVertex(0)[0];
        for (int vertex = 1; vertex < mesh.GetNV(); ++vertex)
        {
            radial_origin_x = std::min(radial_origin_x, mesh.GetVertex(vertex)[0]);
        }
    }

    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        // 单元积分 \[\int_{\Omega_e} g(\boldsymbol x)\,\mathrm dV=\int_{\hat\Omega}g\!\left(\boldsymbol x(\boldsymbol\zeta)\right)|J(\boldsymbol\zeta)|\,\mathrm d\hat V.\]
        mfem::ElementTransformation* transformation = mesh.GetElementTransformation(element); // transformation 参考单元坐标向网格单元实际位置坐标的映射对象 x=x_e(\zeta)
        const int integration_order = 2 * transformation->OrderW() + 4; // OrderW() 当前单元几何映射的雅可比权重 |J| 的多项式阶次估计(雅可比行列式关于参考坐标\zeta\eta\psi的多项式的最高次阶),考虑真正的被积函数还可能有2\pir或4\pir**2，所以还加了4
        const mfem::IntegrationRule& rule = mfem::IntRules.Get(mesh.GetElementBaseGeometry(element), integration_order); // mesh.GetElementBaseGeometry(element)当前积单元的基本形状, IntRules反回的rule包含该形状该积分精度阶次的参考坐标下的积分点与权重

        for (int point_index = 0; point_index < rule.GetNPoints(); ++point_index)
        {
            const mfem::IntegrationPoint& point = rule.IntPoint(point_index);
            transformation->SetIntPoint(&point); // 告诉 transformation，接下来要在参考单元的这个积分点计算雅可比等几何量
            double weight = point.weight * transformation->Weight(); // w_q |J(\zeta_q)| 把参考单元的权重转换到实际单元上

            if (dimension < 3)
            {
                transformation->Transform(point, position); // Transform 利用当前单元的几何映射，把point的参考单元坐标换成实际坐标，并写入 position
                const double radius = position[0] - radial_origin_x;
                weight *= dimension == 1 ? 4.0 * pi * radius * radius : 2.0 * pi * radius;
            }
            region_volumes_[mesh.GetAttribute(element)] += weight;
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
