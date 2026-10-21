#pragma once

#include <type_traits>

#include "common/model_registry.hpp"
#include "properties/common/constant.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

// 公共算法不继承具体物性接口; 由这个薄适配器对接已有 double/StateValue 接口。
template<typename ModelBase, typename Model, typename ValueType = double>
class CommonModelAdapter final : public ModelBase
{
public:
    explicit CommonModelAdapter(const ModelParameterMap& parameters) : model_(parameters) {}

    ValueType evaluate(const PropertiesStatePoint& point) const override
    {
        if constexpr (std::is_same_v<ValueType, StateValue>)
        {
            return model_.evaluate(point);
        }
        else
        {
            return std::get<ValueType>(model_.evaluate(point));
        }
    }

private:
    Model model_;
};

// 各属性 manager 保留自己的注册表和模型名, 仅复用公共模型的实现。
template<typename ModelBase, typename ValueType = double>
void register_common_models(ModelRegistry<ModelBase>& registry, const std::string& constant_name = "common.constant", const std::string& time_series_name = "common.time_series")
{
    registry.template register_model<CommonModelAdapter<ModelBase, ConstantModel, ValueType>>(constant_name);
    registry.template register_model<CommonModelAdapter<ModelBase, TimeSeriesModel, ValueType>>(time_series_name, {"time"});
}

}
