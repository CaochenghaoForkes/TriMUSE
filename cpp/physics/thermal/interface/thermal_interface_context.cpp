#include "physics/thermal/interface/thermal_interface_context.hpp"

namespace trimuse
{

ThermalInterfaceContext::ThermalInterfaceContext(const MeshData::Interface& interface, FemSystem& fem, const ThermalField& field) // 一个网格界面片的数据访问入口 把界面信息、有限元系统和试探温度场联系起来，让具体模型可以方便地取得 (当前界面片及两侧区域、单元信息；两侧有限元空间与自由度编号；两侧几何变换和界面求积规则；同一个界面积分点的实际坐标与两侧试探温度)
    : interface_(interface), fem_(fem), field_(field) 
{
}

mfem::FaceElementTransformations& ThermalInterfaceContext::transformations() const // 取得当前界面片及两侧域单元的几何变换对象返回引用 有了它就可以把同一个界面积分点映射到两侧单元中的对应位置，分别计算两侧温度或形函数
{
    /*
                    ┌── Face ─────────────────→ 实际点 x
        参考面点 ŝ ──┼── Loc1 → 第一侧参考点 → Elem1 → 实际点 x
                    └── Loc2 → 第二侧参考点 → Elem2 → 实际点 x
    */
    return *fem_.mesh_data().mfem_mesh().GetFaceElementTransformations(interface_.face); // interface_.face 当前界面在原网格中的面编号 .GetFaceElementTransformations(interface_.face) 返回 Face：参考面坐标 -> 实际空间坐标 Elem1、Elem2 两侧参考单元坐标 -> 实际空间坐标 Loc1、Loc2 参考面坐标->两侧参考单元坐标 Elem1No、Elem2No 两侧的原网格域单元编号
}

const mfem::IntegrationRule& ThermalInterfaceContext::quadrature_rule() const // 返回适用于当前界面片的求积规则，用于遍历界面积分点及其权重
{
    return *fem_.boundary_quadrature_rule(transformations().GetGeometryType()); // transformations().GetGeometryType() 取得当前界面片的参考几何类型 (如 1D 点 2D 线 3D 三角形/四边形) fem_.boundary_quadrature_rule(几何类型) 从 FemSystem 预先建立的边界求积规则表中 找到几何类型对应的规则
}

ThermalInterfacePoint ThermalInterfaceContext::point(const mfem::IntegrationPoint& face_point) const // 输入参考面上的一个积分点，返回它的实际空间坐标，以及该位置两侧的试探温度
{
    auto& transformation = transformations(); // 取得当前界面片的双侧几何变换
    transformation.SetAllIntPoints(&face_point); // 设置当前参考面积分点，同时通过 Loc1、Loc2 得到它在两侧参考单元中的对应坐标，并设置两侧变换的当前积分点
    mfem::Vector position(fem_.mesh_data().space_dimension()); // 创建用于接收实际坐标的向量，长度等于空间维数
    transformation.Transform(face_point, position); // 通过界面的几何映射，将参考面点转换为实际空间坐标 x = Face(ŝ)
    const std::array<mfem::IntegrationPoint, 2> local_points{transformation.GetElement1IntPoint(), transformation.GetElement2IntPoint()}; // 保存该点在两侧参考单元中的坐标 local_points[0](Loc1(face_point)) local_points[1](Loc2(face_point))

    ThermalInterfacePoint result; // 将实际坐标写入返回结构 低维问题中不存在的坐标分量填零
    result.x = position[0];
    result.y = position.Size() > 1 ? position[1] : 0.0;
    result.z = position.Size() > 2 ? position[2] : 0.0;
    const auto view = field_.view(); // 取得试探温度场的统一访问接口，连续场和分域场都通过它读取
    for (int side = 0; side < 2; ++side) // 分别处理两侧
    {
        const auto& location = interface_.sides[side]; // 取得当前侧的区域属性编号与原网格域单元编号
        result.temperature[side] = std::get<double>(view.evaluate(location.region_attribute, location.element, local_points[side])); // 根据当前侧所属区域、单元及参考坐标，插值得到该侧的试探温度
    }
    return result;
}

}
