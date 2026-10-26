#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class SurfaceNuclideConcentrationModel
{
public:
    virtual ~SurfaceNuclideConcentrationModel() = default;

    virtual StateValue evaluate(const PropertiesStatePoint& point) const = 0;
};

class SurfaceNuclideConcentrationModelManager
{
public:
    void register_models();

    void assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<SurfaceNuclideConcentrationModel, const std::string&> registry_;
    std::unordered_map<int, std::unique_ptr<SurfaceNuclideConcentrationModel>> zone_model_map_;
};

}
