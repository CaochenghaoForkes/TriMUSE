#pragma once
#include "physics/radioactivity/common/radioactivity_domain_model.hpp"

namespace trimuse
{

class UnitCapacity final : public RadioactivityDomainModel // 浓度时间导数前的系数恒为 1
{
public:
    explicit UnitCapacity(const ModelParameterMap&) {}

    double evaluate(const PropertiesFieldData&, std::size_t) const override // 不依赖材料属性和积分点位置
    {
        return 1.0;
    }
};

}
