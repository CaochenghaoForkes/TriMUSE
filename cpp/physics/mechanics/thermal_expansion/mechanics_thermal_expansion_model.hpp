#pragma once

#include "common/model_parameters.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class MechanicsThermalExpansionModel
{
public:
    virtual ~MechanicsThermalExpansionModel() = default;

    virtual VectorValue evaluate(const PropertiesFieldData& properties, std::size_t point, double temperature) const = 0; // 返回计算坐标系中的 beta = C : epsilon_th，顺序为 [11,22,33,23,13,12] beta 是热膨胀右端的系数，最终应力为 sigma = C : epsilon(u) - beta
};

}
