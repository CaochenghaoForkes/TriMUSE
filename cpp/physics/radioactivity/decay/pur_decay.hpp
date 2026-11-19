#pragma once

#include "physics/radioactivity/common/radioactivity_domain_model.hpp"

namespace trimuse
{

class PurDecay final : public RadioactivityDomainModel // 单核素纯衰变，返回正的左端反应系数 lambda
{
public:
    explicit PurDecay(const ModelParameterMap& parameters);
    double evaluate(const PropertiesFieldData& properties, std::size_t point) const override;
};

}
