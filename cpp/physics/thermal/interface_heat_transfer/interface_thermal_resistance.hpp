#pragma once

#include "common/angular_range.hpp"
#include "physics/thermal/interface_heat_transfer/thermal_interface_heat_transfer_model.hpp"

namespace trimuse
{

class ThermalResistanceFromProperty final : public ThermalInterfaceModel
{
public:
    explicit ThermalResistanceFromProperty(const ModelParameterMap& parameters); // 把 Python 传来的角域参数整理成 AngularRanges 角域列表

    bool is_perfect_contact(const InterfaceContext& context) const override; // 判断当前面片是否按完美接触处理

private:
    AngularRanges angles_;
};

}
