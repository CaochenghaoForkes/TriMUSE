#pragma once

#include "properties/assembly/homogenized_panama_opf/homogenized_panama_opf_model_manager.hpp"
#include "properties/assembly/local_panama_opf/panama.hpp"

namespace trimuse
{

class HomogenizedPanamaOpf final : public HomogenizedPanamaOpfModel
{
public:
    explicit HomogenizedPanamaOpf(const ModelParameterMap& parameters);
    VectorValue evaluate(const PropertiesStatePoint& point) const override;

private:
    PanamaOpf panama_model_; // 复用相同的 PANAMA 历史递推和事故修正
};

}
