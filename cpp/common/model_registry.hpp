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

template<typename ModelBase>
class ModelRegistry
{
public:
    using Factory = std::function<std::unique_ptr<ModelBase>(const ModelParameterMap&)>;

    struct Registration
    {
        Factory factory;
        std::vector<std::string> required_states;
    };

    template<typename Model> // 每个模型均接收参数字典, 工厂不逐项解析构造参数
    void register_model(const std::string& name, std::vector<std::string> required_states = {})
    {
        Factory factory = [](const ModelParameterMap& parameters)
        {
            return std::make_unique<Model>(parameters);
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
