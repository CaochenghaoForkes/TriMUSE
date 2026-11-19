#include "physics/radioactivity/capacity/unit_capacity.hpp"

namespace trimuse
{

UnitCapacity::UnitCapacity(const ModelParameterMap&)
{
}

double UnitCapacity::evaluate(const PropertiesFieldData&, std::size_t) const // 不依赖材料属性和积分点位置
{
    return 1.0;
}

}
