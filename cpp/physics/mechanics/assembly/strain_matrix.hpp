#pragma once

#include <algorithm>

#include "physics/common/field/trial_field.hpp"

namespace trimuse
{

inline std::vector<mfem::DenseMatrix> make_strain_matrices(const FemSystem& fem, const TrialField& trial_field) // 六分量张量应变 [11,22,33,23,13,12]，列按各位移分量依次排列
{
    const int dimension = fem.mesh_data().dimension();
    const auto& mesh = fem.mesh_data().mfem_mesh();
    double radial_origin = 0.0; // 与 FemSystem 的降维几何权重使用同一个径向原点
    if (dimension < 3) // 如果为 1D/2D 寻找 x 最小的那个几何节点作为 radial_origin
    {
        radial_origin = mesh.GetVertex(0)[0];
        for (int vertex = 1; vertex < mesh.GetNV(); ++vertex)
        {
            radial_origin = std::min(radial_origin, mesh.GetVertex(vertex)[0]);
        }
    }
    std::vector<mfem::DenseMatrix> matrices(fem.domain_quadrature_point_count()); // 全局积分点的 B 矩阵 \epsilon_C = Bd
    mfem::IsoparametricTransformation transformation;
    for (int index = 0; index < fem.mesh_data().element_count(); ++index) // 遍历原网格单元索引
    {
        const int region = mesh.GetAttribute(index); // 取得所属区域编号
        const auto* element = trial_field.space(region).GetFE(trial_field.local_element(index)); // 取得所在单元
        const int count = element->GetDof(); // 取得当前单元的标量自由度数，不包含分量数，也就是该单元的插值节点数
        mfem::DenseMatrix gradient(count, dimension); // 各标量形函数对实际空间坐标的导数，只在建立 B 缓存时使用
        mesh.GetElementTransformation(index, &transformation); // 取得该单元的 transformation
        const auto* rule = fem.domain_quadrature_rule(element->GetGeomType()); // 取得该单元求积规则
        const auto offset = fem.domain_quadrature_offset(index); // 取得该单元起始积分点在全局积分点的偏移量
        for (int q = 0; q < rule->GetNPoints(); ++q) // 遍历积分点
        {
            transformation.SetIntPoint(&rule->IntPoint(q)); // 取得该积分点位置的雅可比映射
            element->CalcPhysDShape(transformation, gradient); // 在当前求积点计算各个形函数对实际空间坐标的导数，结果写入 gradient
            auto& matrix = matrices[offset + q]; // 准备当前求积点的 B
            matrix.SetSize(6, count * dimension); // 6 行对应六个应变分量 count * dimension 列：对应当前单元全部位移自由度
            matrix = 0.0;
            if (dimension < 3)
            {
                mfem::Vector shape(count), position(dimension);
                element->CalcShape(transformation.GetIntPoint(), shape); // 计算当前点的形函数值
                transformation.Transform(transformation.GetIntPoint(), position); // 当前点位置
                const double radius = position[0] - radial_origin; // 当前点与几何中心间的距离
                for (int i = 0; i < count; ++i) // 遍历插值节点
                {
                    matrix(0, i) = gradient(i, 0);
                    if (dimension == 1) // 球对称：(u_r,r，u_r/r，u_r/r)，其余分量为零
                    {
                        matrix(1, i) = matrix(2, i) = shape[i] / radius;
                    }
                    else // 轴对称：(u_r,r，u_z,z，u_r/r，0，0，(u_r,z+u_z,r)/2)
                    {
                        matrix(1, count + i) = gradient(i, 1);
                        matrix(2, i) = shape[i] / radius;
                        matrix(5, i) = 0.5 * gradient(i, 1);
                        matrix(5, count + i) = 0.5 * gradient(i, 0);
                    }
                }
                continue;
            }
            for (int i = 0; i < count; ++i) // 遍历插值节点
            {
                matrix(0, i) = gradient(i, 0);
                matrix(1, count + i) = gradient(i, 1);
                matrix(2, 2 * count + i) = gradient(i, 2);
                matrix(3, count + i) = 0.5 * gradient(i, 2);
                matrix(3, 2 * count + i) = 0.5 * gradient(i, 1);
                matrix(4, i) = 0.5 * gradient(i, 2);
                matrix(4, 2 * count + i) = 0.5 * gradient(i, 0);
                matrix(5, i) = 0.5 * gradient(i, 1);
                matrix(5, count + i) = 0.5 * gradient(i, 0);
            }
        }
    }
    return matrices;
}

}
