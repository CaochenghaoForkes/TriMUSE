#include "properties/material/mechanics/thermal_expansion_coef/thermal_expansion_coef_model_manager.hpp"
#include "properties/material/mechanics/thermal_expansion_coef/common/isotropy_constant_thermal_expansion_coef.hpp"
#include "properties/material/mechanics/thermal_expansion_coef/common/anisotropy_constant_thermal_expansion_coef.hpp"
#include "properties/material/mechanics/thermal_expansion_coef/buffer/bison_2021_buffer_thermal_expansion_coef.hpp"
#include "properties/material/mechanics/thermal_expansion_coef/pyc/bison_2021_pyc_thermal_expansion_coef.hpp"

namespace trimuse
{

ThermalExpansionCoefModel::~ThermalExpansionCoefModel() = default;

void ThermalExpansionCoefModelManager::register_models()
{
    registry_.register_model<IsotropyConstantThermalExpansionCoef>("common.isotropy_constant");
    registry_.register_model<AnisotropyConstantThermalExpansionCoef>("common.anisotropy_constant");
    registry_.register_model<Bison2021BufferThermalExpansionCoef>("buffer.bison_2021", {"temperature"});
    registry_.register_model<Bison2021PycThermalExpansionCoef>("pyc.bison_2021", {"temperature", "material.general.bacon_anisotropy_factor"});
}

void ThermalExpansionCoefModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

VectorValue ThermalExpansionCoefModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<RequiredState>& ThermalExpansionCoefModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
