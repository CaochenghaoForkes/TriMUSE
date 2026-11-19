#pragma once

#include "physics/mechanics/thermal_expansion/mechanics_thermal_expansion_model.hpp"

namespace trimuse
{

class LinearThermalExpansion final : public MechanicsThermalExpansionModel
{
public:
    explicit LinearThermalExpansion(const ModelParameterMap&);
    VectorValue evaluate(const PropertiesFieldData& properties, std::size_t point, double temperature) const override; // 取得求积点 point 上向量 \sigma_{th,global} = C:\varepsilon_{th,global}
};

}
