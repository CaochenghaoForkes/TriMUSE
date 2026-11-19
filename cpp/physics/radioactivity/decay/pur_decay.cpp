#include "physics/radioactivity/decay/pur_decay.hpp"

#include <cmath>

namespace trimuse
{

PurDecay::PurDecay(const ModelParameterMap&)
{
}

double PurDecay::evaluate(const PropertiesFieldData& properties, std::size_t) const
{
    const double half_life = std::get<double>(properties.global_values.at("precomputed.half_life").value());
    return std::log(2.0) / half_life;
}

}
