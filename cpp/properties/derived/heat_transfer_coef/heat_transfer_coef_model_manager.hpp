#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class HeatTransferCoefModel
{
public:
    virtual ~HeatTransferCoefModel() = default; // = default 表示：自动执行成员、基类等正常的销毁过程 virtual 通过基类指针销毁对象时，也能正确调用具体派生类的析构函数 

    virtual StateValue evaluate(const PropertiesStatePoint& point) const = 0; // =0 表示 这是一个纯虚函数，要求具体派生类提供实现
};

class HeatTransferCoefModelManager
{
public:
    void register_models();

    void assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<HeatTransferCoefModel> registry_; // 使用类模板创建成员对象(ModelRegistry 类模版 <HeatTransferCoefModel> 指定模板参数 ModelBase 为换热系数模型接口) 这个对象内部的 registrations_ 将保存模型名(如 commonn.time_series) -> 注册记录 初始为空
    
    std::unordered_map<int, std::unique_ptr<HeatTransferCoefModel>> zone_model_map_; // 按边界分区编号管理换热系数模型对象的哈希表 如 边界分区 41 -> ConstantHeatTransferCoef 对象
};

}
