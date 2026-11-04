#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

// 所有模型统一返回 [Er, Et]，径向、切向杨氏模量，单位 Pa。
class YoungModulusModel
{
public:
    virtual ~YoungModulusModel() = default;

    virtual VectorValue evaluate(const PropertiesStatePoint& state_point) const = 0;
};

class YoungModulusModelManager
{
public:
    void register_models();

    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& state_point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<YoungModulusModel> registry_;
    std::unordered_map<int, std::unique_ptr<YoungModulusModel>> region_model_map_;
};

}
