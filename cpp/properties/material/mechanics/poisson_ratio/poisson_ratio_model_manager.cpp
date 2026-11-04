#include "properties/material/mechanics/poisson_ratio/poisson_ratio_model_manager.hpp"
#include "properties/material/mechanics/poisson_ratio/common/isotropy_constant_poisson_ratio.hpp"
#include "properties/material/mechanics/poisson_ratio/common/anisotropy_constant_poisson_ratio.hpp"
#include "properties/material/mechanics/poisson_ratio/common/isotropy_time_series_poisson_ratio.hpp"
#include "properties/material/mechanics/poisson_ratio/common/anisotropy_time_series_poisson_ratio.hpp"

namespace trimuse
{

void PoissonRatioModelManager::register_models()
{
    registry_.register_model<IsotropyConstantPoissonRatio>("common.isotropy_constant");
    registry_.register_model<AnisotropyConstantPoissonRatio>("common.anisotropy_constant");
    registry_.register_model<IsotropyTimeSeriesPoissonRatio>("common.isotropy_time_series", {"external.time"});
    registry_.register_model<AnisotropyTimeSeriesPoissonRatio>("common.anisotropy_time_series", {"external.time"});
}

void PoissonRatioModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

VectorValue PoissonRatioModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& PoissonRatioModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
