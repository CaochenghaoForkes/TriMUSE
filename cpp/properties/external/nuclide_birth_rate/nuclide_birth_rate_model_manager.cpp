#include "properties/external/nuclide_birth_rate/nuclide_birth_rate_model_manager.hpp"
#include "properties/external/nuclide_birth_rate/constant_nuclide_birth_rate.hpp"
#include "properties/external/nuclide_birth_rate/time_series_nuclide_birth_rate.hpp"

#include <stdexcept>

namespace trimuse
{

void NuclideBirthRateModelManager::register_models()
{
    registry_.register_model<ConstantNuclideBirthRate>("common.constant");
    registry_.register_model<TimeSeriesNuclideBirthRate>("common.time_series", {"external.time"});
}

void NuclideBirthRateModelManager::assign(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    model_ = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue NuclideBirthRateModelManager::evaluate(const PropertiesStatePoint& point) const
{
    if (!model_) { throw std::logic_error("尚未设置 nuclide_birth_rate 模型"); }
    return model_->evaluate(point);
}

const std::vector<std::string>& NuclideBirthRateModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
