#pragma once

#include <array>
#include "physics/thermal/interface/thermal_interface_model.hpp"

namespace trimuse
{

class ThermalLocalFromConstantResistance final : public ThermalInterfaceModel // 用参考面中心映射后的实际坐标分类；整片采用恒定热阻或完美接触
{
public:
    explicit ThermalLocalFromConstantResistance(const ModelParameterMap& parameters);

    void assemble(const ThermalInterfaceContext& context, ThermalInterfaceAssembly& assembly) const override;

private:
    struct Zone // 当前界面上，各个局部热阻区域的判定规则和传热系数
    {
        double conductance; // 该区域的界面传热系数 h=1/R W/(m^2K)
        double minimum, maximum; // 区域的角度下限、上限，内部使用弧度 2D 中是从上极点起算的极角范围；3D 中是相对 direction 的角距离范围
        std::array<double, 3> direction; // 长度为 3 的数组，保存 3D 区域中心方向的单位向量 (sintheta*cosphi, sintheta*sinphi,costheta)
    };

    int dimension_; // 模型维数

    std::array<double, 3> center_{}; // 保存颗粒中心的 {x, y, z} 坐标 计算角度前，先用面片中心坐标减去它，得到相对颗粒中心的位置

    std::vector<Zone> zones_; // 保存这个界面上的全部局部热阻区域 每个元素是一个 Zone
};

}
