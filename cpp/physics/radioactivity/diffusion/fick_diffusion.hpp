#pragma once
#include "physics/radioactivity/common/radioactivity_domain_model.hpp"

namespace trimuse
{

class FickDiffusion final : public RadioactivityDomainModel // 返回 Fick 扩散项在积分点上的扩散系数 D
{
public:
    explicit FickDiffusion(const ModelParameterMap&) {} // 系数由属性系统提供，本模型无需额外参数

    double evaluate(const PropertiesFieldData& properties, std::size_t point) const override
    {
        return scalar(properties, "material.radioactivity.nuclide_diffusion_coef", point);
    }
};

}
