#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

// 所有模型统一返回 [nu_rt, nu_tt]，径向受力的切向泊松比、切向之间的泊松比。
class PoissonRatioModel
{
public:
    virtual ~PoissonRatioModel() = default;

    virtual VectorValue evaluate(const PropertiesStatePoint& state_point) const = 0;
};

class PoissonRatioModelManager
{
public:
    void register_models();

    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& state_point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<PoissonRatioModel> registry_;
    std::unordered_map<int, std::unique_ptr<PoissonRatioModel>> region_model_map_;
};

}
