#pragma once

#include <array>

#include "physics/common/field/trial_field.hpp"
#include "physics/common/interface/interface_context.hpp"

namespace trimuse
{

struct ThermalInterfacePoint // 保存同一个界面积分点的实际坐标，以及该点两侧各自的试探温度
{
    double x = 0.0, y = 0.0, z = 0.0;
    std::array<double, 2> temperature{}; // 顺序与 Interface::sides 一致
};

class ThermalInterfaceContext // 一个网格界面片的数据访问入口 把界面信息、有限元系统和试探温度场联系起来，让具体模型可以方便地取得 (当前界面片及两侧区域、单元信息；两侧有限元空间与自由度编号；两侧几何变换和界面求积规则；同一个界面积分点的实际坐标与两侧试探温度)
{
public:
    ThermalInterfaceContext(const MeshData::Interface& interface, FemSystem& fem, const TrialField& field); // 构造函数

    const MeshData::Interface& interface() const { return geometry_.interface(); }

    const TrialField& field() const { return field_; }

    mfem::FaceElementTransformations& transformations() const { return geometry_.transformations(); } // 取得当前界面片及两侧域单元的几何变换对象返回引用 有了它就可以把同一个界面积分点映射到两侧单元中的对应位置，分别计算两侧温度或形函数

    const mfem::IntegrationRule& quadrature_rule() const { return geometry_.quadrature_rule(); } // 返回适用于当前界面片的求积规则，用于遍历界面积分点及其权重

    ThermalInterfacePoint point(const mfem::IntegrationPoint& face_point) const; // 输入参考面上的一个积分点，返回它的实际空间坐标，以及该位置两侧的试探温度

private:
    InterfaceContext geometry_; // 共用的双侧界面几何访问；原界面与有限元系统由它引用

    const TrialField& field_; // 热求解使用的独立试探场; 全部矩阵索引采用这里的完整自由度编号
};

}
