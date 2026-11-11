#include "properties/material/mechanics/young_modulus/young_modulus_model_manager.hpp"
#include "properties/material/mechanics/young_modulus/common/isotropy_constant_young_modulus.hpp"
#include "properties/material/mechanics/young_modulus/common/anisotropy_constant_young_modulus.hpp"
#include "properties/material/mechanics/young_modulus/common/isotropy_time_series_young_modulus.hpp"
#include "properties/material/mechanics/young_modulus/common/anisotropy_time_series_young_modulus.hpp"
#include "properties/material/mechanics/young_modulus/kernel/bison_2021_uo2_young_modulus.hpp"
#include "properties/material/mechanics/young_modulus/buffer/bison_2021_buffer_young_modulus.hpp"
#include "properties/material/mechanics/young_modulus/pyc/bison_2021_pyc_young_modulus.hpp"

namespace trimuse
{

YoungModulusModel::~YoungModulusModel() = default;

void YoungModulusModelManager::register_models()
{
    registry_.register_model<IsotropyConstantYoungModulus>("common.isotropy_constant");
    registry_.register_model<AnisotropyConstantYoungModulus>("common.anisotropy_constant");
    registry_.register_model<IsotropyTimeSeriesYoungModulus>("common.isotropy_time_series", {"external.time"});
    registry_.register_model<AnisotropyTimeSeriesYoungModulus>("common.anisotropy_time_series", {"external.time"});
    registry_.register_model<Bison2021Uo2YoungModulus>("kernel.bison_2021_uo2", {"temperature", "material.general.mass_density"});
    registry_.register_model<Bison2021BufferYoungModulus>("buffer.bison_2021", {"temperature", "material.general.mass_density", "derived.fast_neutron_fluence"});
    registry_.register_model<Bison2021PycYoungModulus>("pyc.bison_2021", {"temperature", "material.general.mass_density", "material.general.initial_bacon_anisotropy_factor", "material.general.crystallite_diameter", "derived.fast_neutron_fluence"});
}

void YoungModulusModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

VectorValue YoungModulusModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& YoungModulusModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
