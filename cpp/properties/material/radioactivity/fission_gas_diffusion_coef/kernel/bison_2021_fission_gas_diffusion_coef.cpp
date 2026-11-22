#include "properties/material/radioactivity/fission_gas_diffusion_coef/kernel/bison_2021_fission_gas_diffusion_coef.hpp"

#include <cmath>

namespace trimuse
{

Bison2021FissionGasDiffusionCoef::Bison2021FissionGasDiffusionCoef(const ModelParameterMap& parameters)
{
}

// BISON/PARFUME 三项扩散模型，常数按物性知识库取值，结果单位为 m²/s
// https://mooseframework.inl.gov/bison/source/materials/UCOFGR.html
// 此处采用 D₁ + D₂ + D₃，不包含可选的气泡俘获与再溶解修正
double Bison2021FissionGasDiffusionCoef::evaluate(const PropertiesStatePoint& point) const
{
    constexpr double jump_distance = 3.0e-10; // m，原子跃迁距离
    constexpr double recombination_sites = 2.0; // 点缺陷周围必然发生复合的位点数 Z
    constexpr double damage_per_fission = 1.0e4; // 每次裂变产生的缺陷数 K
    const double temperature = point.scalar("temperature"); // K
    const double burnup = point.scalar("derived.burnup_fima_fraction"); // FIMA 份额，0～1
    const double fission_rate = point.scalar("derived.average_volumetric_fission_rate"); // fission/(m³·s)
    const double irradiation_time = point.scalar("derived.irradiation_time"); // s
    const double thermal_diffusion = 7.6e-10 * std::exp(-35225.0 / temperature);
    double irradiation_diffusion = 0.0;
    if (irradiation_time > 0.0) // 初始辐照时间为零时，辐照空位项取零
    {
        const double vacancy_jump_rate = 1.0e13 * std::exp(-27778.0 / temperature);
        irradiation_diffusion = jump_distance * jump_distance * std::sqrt(vacancy_jump_rate * damage_per_fission * burnup / (recombination_sites * irradiation_time)); // s²√(jᵥ K Bᵤ / (Z t))，与知识库公式等价
    }
    const double low_temperature_diffusion = 2.0e-40 * fission_rate;
    return thermal_diffusion + irradiation_diffusion + low_temperature_diffusion;
}

}
