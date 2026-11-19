#include "physics/radioactivity/decay/pur_decay.hpp"

namespace trimuse
{

PurDecay::PurDecay(const ModelParameterMap&)
{
}

double PurDecay::evaluate(const PropertiesFieldData& properties, std::size_t) const
{
    return std::get<double>(properties.global_values.at("precomputed.decay_constant").value());
}

}
