#include "properties/derived/heat_transfer_coef/heat_transfer_coef_model_manager.hpp"
#include "properties/derived/heat_transfer_coef/constant_heat_transfer_coef.hpp"
#include "properties/derived/heat_transfer_coef/time_series_heat_transfer_coef.hpp"

namespace trimuse
{

HeatTransferCoefModel::~HeatTransferCoefModel() = default; // = default 表示：自动执行成员、基类等正常的销毁过程 virtual 通过基类指针销毁对象时，也能正确调用具体派生类的析构函数

void HeatTransferCoefModelManager::register_models()
{
    registry_.register_model<ConstantHeatTransferCoef>("common.constant");
    registry_.register_model<TimeSeriesHeatTransferCoef>("common.time_series", {"external.time"});
}

void HeatTransferCoefModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue HeatTransferCoefModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<RequiredState>& HeatTransferCoefModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
