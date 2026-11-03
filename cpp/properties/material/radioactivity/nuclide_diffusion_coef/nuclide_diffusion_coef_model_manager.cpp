#include "properties/material/radioactivity/nuclide_diffusion_coef/nuclide_diffusion_coef_model_manager.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/common/arrhenius_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/common/constant_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/common/time_series_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/kernel/tecdoc978_frg_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/kernel/tecdoc978_japan_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/pyc/tecdoc978_frg_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/pyc/tecdoc978_usa_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/pyc/tecdoc978_japan_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/pyc/tecdoc978_russian_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/sic/tecdoc978_frg_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/sic/tecdoc978_usa_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/sic/tecdoc978_japan_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/sic/tecdoc978_russian_nuclide_diffusion_coef.hpp"

namespace trimuse
{

void NuclideDiffusionCoefModelManager::register_models()
{
    registry_.register_model<ConstantNuclideDiffusionCoef>("common.constant");
    registry_.register_model<TimeSeriesNuclideDiffusionCoef>("common.time_series", {"external.time"});
    registry_.register_model<ArrheniusNuclideDiffusionCoef>("common.arrhenius", {"temperature"});
    registry_.register_model<TecDoc978FRGUO2NuclideDiffusionCoef>("kernel.tecdoc978_frg", {"temperature"});
    registry_.register_model<TecDoc978JapanUO2NuclideDiffusionCoef>("kernel.tecdoc978_japan", {"temperature"});
    registry_.register_model<TecDoc978FRGPyCNuclideDiffusionCoef>("pyc.tecdoc978_frg", {"temperature"});
    registry_.register_model<TecDoc978USAPyCNuclideDiffusionCoef>("pyc.tecdoc978_usa", {"temperature"});
    registry_.register_model<TecDoc978JapanPyCNuclideDiffusionCoef>("pyc.tecdoc978_japan", {"temperature"});
    registry_.register_model<TecDoc978RussianPyCNuclideDiffusionCoef>("pyc.tecdoc978_russian", {"temperature"});
    registry_.register_model<TecDoc978FRGSiCNuclideDiffusionCoef>("sic.tecdoc978_frg", {"temperature"});
    registry_.register_model<TecDoc978USASiCNuclideDiffusionCoef>("sic.tecdoc978_usa", {"temperature"});
    registry_.register_model<TecDoc978USALowerSiCNuclideDiffusionCoef>("sic.tecdoc978_usa_lower", {"temperature"});
    registry_.register_model<TecDoc978USAUpperSiCNuclideDiffusionCoef>("sic.tecdoc978_usa_upper", {"temperature"});
    registry_.register_model<TecDoc978JapanSiCNuclideDiffusionCoef>("sic.tecdoc978_japan", {"temperature"});
    registry_.register_model<TecDoc978RussianSiCNuclideDiffusionCoef>("sic.tecdoc978_russian", {"temperature"});
}

void NuclideDiffusionCoefModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double NuclideDiffusionCoefModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& NuclideDiffusionCoefModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
