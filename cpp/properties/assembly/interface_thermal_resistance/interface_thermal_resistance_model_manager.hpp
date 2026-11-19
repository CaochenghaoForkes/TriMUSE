#pragma once

#include <optional>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class InterfaceThermalResistance
{
public:
    virtual ~InterfaceThermalResistance();

    virtual std::optional<double> evaluate(const PropertiesStatePoint& point) const = 0; // 空值表示该面片采用理想接触
};

class InterfaceThermalResistanceModelManager
{
public:
    void register_models();

    void assign(int attribute, const std::string& model, const ModelParameterMap& parameters);

    const std::vector<std::string>& required_states(const std::string& model) const;

    std::optional<double> evaluate(const PropertiesStatePoint& point) const;

private:
    ModelRegistry<InterfaceThermalResistance> registry_;
    std::unordered_map<int, std::unique_ptr<InterfaceThermalResistance>> interface_models_;
};

}
