#include "properties/material/thermal/conductivity/conductivity_model_manager.hpp"

#include "properties/material/thermal/conductivity/common/constant_conductivity.hpp"
#include "properties/material/thermal/conductivity/kernel/bison_2021_uco_kernel_conductivity.hpp"
#include "properties/material/thermal/conductivity/kernel/ineri_2004_uo2_kernel_conductivity.hpp"
#include "properties/material/thermal/conductivity/buffer/bison_2021_buffer_conductivity.hpp"
#include "properties/material/thermal/conductivity/buffer/uk_2004_buffer_conductivity.hpp"
#include "properties/material/thermal/conductivity/sic/miller_sic_conductivity.hpp"
#include "properties/material/thermal/conductivity/sic/cea_2004_sic_conductivity.hpp"

namespace trimuse
{

void ConductivityModelManager::register_models()
{
    registry_.register_model<ConstantConductivity>("common.constant");
    registry_.register_model<Bison2021UcoKernelConductivity>("kernel.bison_2021_uco", {"temperature"});
    registry_.register_model<Ineri2004Uo2KernelConductivity>("kernel.ineri_2004_uo2", {"temperature", "burnup_percent_fima", "general.porosity"});
    registry_.register_model<Bison2021BufferConductivity>("buffer.bison_2021", {"general.mass_density"});
    registry_.register_model<Uk2004BufferConductivity>("buffer.uk_2004", {"general.porosity"});
    registry_.register_model<MillerSicConductivity>("sic.miller", {"temperature"});
    registry_.register_model<Cea2004SicConductivity>("sic.cea_2004", {"temperature", "general.porosity"});
}

void ConductivityModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double ConductivityModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& ConductivityModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
