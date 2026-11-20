#pragma once

#include "properties/derived/interface_area/interface_area_model_manager.hpp"

namespace trimuse
{

class UndeformedInterfaceArea final : public InterfaceAreaModel
{
public:
    explicit UndeformedInterfaceArea(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
