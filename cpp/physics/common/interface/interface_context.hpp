#pragma once

#include <array>

#include "fem/fem_system.hpp"

namespace trimuse
{

struct InterfacePoint // 保存同一个界面积分点的实际坐标，以及该点在两侧参考单元中的坐标
{
    double x = 0.0, y = 0.0, z = 0.0;
    std::array<mfem::IntegrationPoint, 2> local_points; // 顺序与 Interface::sides 一致
};

class InterfaceContext // 一个网格界面片的数据访问入口 把界面信息与有限元系统联系起来，提供当前界面片及两侧区域、单元信息、几何变换、求积规则，以及同一个积分点的实际坐标和两侧参考坐标
{
public:
    InterfaceContext(const MeshData::Interface& interface, FemSystem& fem); // 构造函数

    const MeshData::Interface& interface() const { return interface_; }

    mfem::FaceElementTransformations& transformations() const; // 取得当前界面片及两侧域单元的几何变换对象返回引用 有了它就可以把同一个界面积分点映射到两侧单元中的对应位置，分别计算两侧场值或形函数

    const mfem::IntegrationRule& quadrature_rule() const; // 返回适用于当前界面片的求积规则，用于遍历界面积分点及其权重

    InterfacePoint point(const mfem::IntegrationPoint& face_point) const; // 输入参考面上的一个积分点，返回它的实际空间坐标，以及该位置两侧的参考单元坐标

private:
    const MeshData::Interface& interface_; // face(该界面在原网格中的面索引) boundary_attribute(输入给这个界面指定的编号,未标记时为0) sides[0]sides[1](两侧区域编号和单元编号)

    FemSystem& fem_; // 有限元对象
};

}
