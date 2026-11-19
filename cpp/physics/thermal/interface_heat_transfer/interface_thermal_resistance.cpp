#include "physics/thermal/interface_heat_transfer/interface_thermal_resistance.hpp"
#include "physics/common/interface/interface_context.hpp"

namespace trimuse
{

ThermalResistanceFromProperty::ThermalResistanceFromProperty(const ModelParameterMap& parameters)
    :
    angles_(parameters) // 把 Python 传来的角域参数整理成 AngularRanges 角域列表
{
}

bool ThermalResistanceFromProperty::is_perfect_contact(const InterfaceContext& context) const // 判断当前面片是否按完美接触处理
{
    if (angles_.empty()) // 进入这个函数时，当前界面已经选了热阻模型。如果没有指定角域，热阻就覆盖整条界面(对于脱粘热阻，这条也成立，因为初始未脱粘区的会基于很小的阻值替代完美接触条件)，因此当前面片返回 false
    {
        return false;
    }
    const auto& reference_center = mfem::Geometries.GetCenter(context.transformations().GetGeometryType()); // 得到参考中心
    const auto point = context.point(reference_center); // 映射到实际面片中心位置
    const auto& center = context.geometry_center(); // 得到几何中心
    return !angles_.match({point.x - center[0], point.y - center[1], point.z - center[2]}); // 用这个向量查热阻角域。命中时，`match()` 返回有值的 optional，取反后得到 false，表示该面片走热阻模型。未命中时返回 std::nullopt，取反后得到 true，表示完美接触
}

}
