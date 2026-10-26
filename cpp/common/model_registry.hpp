#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/model_parameters.hpp"

namespace trimuse
{

template<typename ModelBase, typename... ConstructorArgs>
class ModelRegistry
{
public:
    using Factory = std::function<std::unique_ptr<ModelBase>(const ModelParameterMap&, ConstructorArgs...)>;

    struct Registration
    {
        Factory factory;
        std::vector<std::string> required_states;
    };

    template<typename Model> // 参数字典及可选构造参数原样转交具体模型, 工厂不解析模型参数
    void register_model(const std::string& name, std::vector<std::string> required_states = {})
    {
        Factory factory = [](const ModelParameterMap& parameters, ConstructorArgs... arguments)
        {
            return std::make_unique<Model>(parameters, std::forward<ConstructorArgs>(arguments)...);
        };
        registrations_.emplace(name, Registration{std::move(factory), std::move(required_states)});
    }

    const Registration& registration(const std::string& name) const
    {
        return registrations_.at(name);
    }

private:
    std::unordered_map<std::string, Registration> registrations_;
};

}
