#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

// 表面压力，单位 Pa；力学边界模型按 t = -p*n 转换为面力
class SurfacePressureModel
{
public:
    virtual ~SurfacePressureModel();

    virtual double evaluate(const PropertiesStatePoint& point) const = 0;
};

class SurfacePressureModelManager
{
public:
    void register_models();

    void assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<SurfacePressureModel> registry_;
    std::unordered_map<int, std::unique_ptr<SurfacePressureModel>> zone_model_map_;
};

}
