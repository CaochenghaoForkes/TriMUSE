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

template<typename ModelBase, typename... ConstructorArgs> // 注册表的两个模板参数: [ModelBase 这一类模型公共继承的接口 如 ConductivityModel] [ConstructorArgs 除参数字典外，构造模型时还需要传入的其他参数类型，可以没有]
class ModelRegistry // 模型名称 -> 创建这个模型的方法，以及模型依赖哪些状态
{
public:
    // Factory 创建模型的函数类型 它表示一种可调用对象: 接收参数字典及额外构造参数 创建一个具体模型 返回指向模型基类的独占智能指针
    // 对于导热系数, 相当于 输入 parameters 输出 std::unique_ptr<ConductivityModel>
    // std::function<返回类型(参数类型...)> 是一种可以保存可调用对象的包装类型, 它可以保存普通函数、lambda 等, 只要它们能按指定的参数方式调用，并产生可转换为指定返回类型的结果
    // std::unique_ptr<ModelBase> 是函数的返回类型 类型是一个管理模型对象的智能指针 如 ConstantConductivity 对象,后续它可以 model->evaluate(point);
    // const ModelParameterMap& 是该函数需要传入的第一个参数: 一个模型参数字典
    // ConstructorArgs... 后续还要传入的参数由其展开决定, 可以没有
    using Factory = std::function<std::unique_ptr<ModelBase>(const ModelParameterMap&, ConstructorArgs...)>; // 一种“接收模型参数、返回模型智能指针”的可调用对象类型

    struct Registration
    {
        Factory factory; // Registration 保存创建这个模型的方法 调用它才会创建模型对象 auto model = registration.factory(parameters);
        std::vector<std::string> required_states; // 保存模型求值时依赖的属性或场状态名称 如 required_states → ["external.time"]
    };

    template<typename Model> // 用于注册一个具体模型：保存它的名称、创建方法和依赖列表 注册时还不会创建模型对象； Model 是调用时通过尖括号指定的具体模型类型(和上面的模型公共接口不同) 如 Model = TimeSeriesConductivity
    void register_model(const std::string& name, std::vector<std::string> required_states = {}) // name 注册名称 如 common.time_series; required_states 求值需要哪些状态 默认为空，例如 {"external.time"} 
    {
        // 创建一个 lambda 并保存在局部变量 factory 中; [] 表示不捕获外部变量 parameters：以后创建模型时传入的参数字典 arguments...：以后传入的额外构造参数
        Factory factory = [](const ModelParameterMap& parameters, ConstructorArgs... arguments) 
        {
            return std::make_unique<Model>(parameters, std::forward<ConstructorArgs>(arguments)...); // std::make_unique<Model>(...) 调用具体模型的构造函数, 并返回拥有它的智能指针 但此刻没有调用这个 lambda 因此还没有创建模型 std::forward<ConstructorArgs>(arguments) 将额外参数逐个转发给构造函数，保留对应的引用和值类别
        };
        registrations_.emplace(name, Registration{std::move(factory), std::move(required_states)}); // 保存注册记录 模型名->Registration: factory->创建模型的方法(工厂函数,工厂函数真的接收 parametersters 以后才会生产真正的模型对象, 而我们保存在 registrations_ 中的正是工厂函数 quired_states->依赖名称列表
    }

    const Registration& registration(const std::string& name) const // 返回这个模型名称对应的整条注册记录的只读引用 注册记录包含 创建模型的方法 依赖列表
    {
        return registrations_.at(name); 
    }

private:
    std::unordered_map<std::string, Registration> registrations_; // 模型名(如 commonn.time_series) -> 注册记录
};

}
