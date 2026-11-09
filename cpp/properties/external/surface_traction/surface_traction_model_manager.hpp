#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

// 表面面力，单位 Pa；分量顺序为 1D [r]、2D [r,z]、3D [x,y,z]
class SurfaceTractionModel
{
public:
    virtual ~SurfaceTractionModel() = default;

    virtual VectorValue evaluate(const PropertiesStatePoint& point) const = 0;
};

class SurfaceTractionModelManager
{
public:
    void register_models();

    void assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<SurfaceTractionModel> registry_;
    std::unordered_map<int, std::unique_ptr<SurfaceTractionModel>> zone_model_map_;
};

}
