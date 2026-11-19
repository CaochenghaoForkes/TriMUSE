#pragma once

#include "properties/derived/region_volume/region_volume_model_manager.hpp"

namespace trimuse
{

class Undeformed final : public RegionVolumeModel
{
public:
    explicit Undeformed(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
