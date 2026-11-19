#include "properties/material/general/bacon_anisotropy_factor/pyc/bison_2021_bacon_anisotropy_factor.hpp"

#include "common/linear_interpolation.hpp"

namespace trimuse
{

namespace
{

// Jiang et al., Journal of Nuclear Materials 548 (2021) 152795, Table 5
const std::vector<double> fluences{0.0, 1.0, 2.0, 3.0, 3.5, 4.0, 4.5, 5.0};
const std::vector<double> ratios{1.0, 1.0019, 1.0114, 1.0219, 1.0286, 1.0324, 1.0362, 1.0381};
}

Bison2021BaconAnisotropyFactor::Bison2021BaconAnisotropyFactor(const ModelParameterMap&)
{
}

double Bison2021BaconAnisotropyFactor::evaluate(const PropertiesStatePoint& point) const
{
    const double initial_baf = point.scalar("material.general.initial_bacon_anisotropy_factor");
    const double fluence = point.scalar("derived.fast_neutron_fluence") / 1e25; // 表中注量以 10^25 n/m² 为单位，超过表上限时保持最后一个比值
    return initial_baf * LinearInterpolator::evaluate(fluences, ratios, fluence);
}

}
