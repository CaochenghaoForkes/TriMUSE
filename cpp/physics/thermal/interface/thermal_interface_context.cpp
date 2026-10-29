#include "physics/thermal/interface/thermal_interface_context.hpp"

namespace trimuse
{

ThermalInterfaceContext::ThermalInterfaceContext(const MeshData::Interface& interface, FemSystem& fem, const TrialField& field) // 一个网格界面片的数据访问入口 把界面信息、有限元系统和试探温度场联系起来，让具体模型可以方便地取得 (当前界面片及两侧区域、单元信息；两侧有限元空间与自由度编号；两侧几何变换和界面求积规则；同一个界面积分点的实际坐标与两侧试探温度)
    : geometry_(interface, fem), field_(field)
{
}

ThermalInterfacePoint ThermalInterfaceContext::point(const mfem::IntegrationPoint& face_point) const // 输入参考面上的一个积分点，返回它的实际空间坐标，以及该位置两侧的试探温度
{
    const auto geometry_point = geometry_.point(face_point); // 公共接口完成面到实际坐标及两侧参考坐标的映射
    ThermalInterfacePoint result;
    result.x = geometry_point.x;
    result.y = geometry_point.y;
    result.z = geometry_point.z;
    const auto view = field_.view(); // 取得试探温度场的统一访问接口，连续场和分域场都通过它读取
    for (int side = 0; side < 2; ++side) // 分别处理两侧
    {
        const auto& location = interface().sides[side]; // 取得当前侧的区域属性编号与原网格域单元编号
        result.temperature[side] = std::get<double>(view.evaluate(location.region_attribute, location.element, geometry_point.local_points[side])); // 根据当前侧所属区域、单元及参考坐标，插值得到该侧的试探温度
    }
    return result;
}

}
