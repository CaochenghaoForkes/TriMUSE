#pragma once

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

#include "common/model_parameters.hpp"
#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class ConductivityModel
{
public:
    virtual ~ConductivityModel() = default;

    virtual double evaluate(const PropertiesStatePoint& state_point) const = 0;

protected:
    static double checked_conductivity(double conductivity)
    {
        if (!std::isfinite(conductivity) || conductivity <= 0.0)
        {
            throw std::domain_error("导热模型计算结果必须为有限正数");
        }
        return conductivity;
    }
};

class ConductivityModelManager
{
public:
    void register_models();

    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<ConductivityModel> registry_;

    std::unordered_map<int, std::unique_ptr<ConductivityModel>> region_model_map_; // 区域编号对应一个独立导热模型, 重复安排时替换该区域的模型
};

}
