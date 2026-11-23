#include "properties/assembly/internal_gas_pressure/rk_from_co_fission_gas_release.hpp"

#include <cmath>

namespace trimuse
{

RkFromCoFissionGasRelease::RkFromCoFissionGasRelease(const ModelParameterMap& parameters)
    :
    source_region_(static_cast<int>(std::get<double>(parameters.at("source_region"))))
{
}

double RkFromCoFissionGasRelease::evaluate(const PropertiesStatePoint& point) const
{
    const double fission_gas = std::get<double>(point.attribute_values("assembly.cumulative_fission_gas_release").at(source_region_).value()); // mol，Kr 与 Xe 的累计释放量
    const double co = std::get<double>(point.attribute_values("assembly.cumulative_co_release").at(source_region_).value()); // mol，CO 的累计释放量
    const auto& porosities = point.domain_values("material.general.porosity");
    const auto& weights = point.domain_values("precomputed.initial_domain_quadrature_weight");
    const auto& temperatures = point.vector("temperature");
    const auto& indices = point.region_point_indices();
    double free_volume = 0.0, temperature_integral = 0.0;
    for (std::size_t q = 0; q < indices.size(); ++q)
    {
        const std::size_t index = indices[q];
        const double pore_volume = std::get<double>(weights[index].value()) * std::get<double>(porosities[index].value());
        free_volume += pore_volume;
        temperature_integral += pore_volume * temperatures[q];
    }
    const double gas_temperature = temperature_integral / free_volume; // K，与理想气体模型采用相同的孔隙体积加权温度
    constexpr double gas_constant = 8.31446261815324; // Pa·m³/(mol·K)
    constexpr double kr_fraction = 0.185, xe_fraction = 0.815; // PARFUME 的裂变气体组成，Kr 与 Xe 采用相同释放份额
    constexpr double a_co = 1.72, a_kr = 3.411, a_xe = 7.158; // Pa·m⁶·K⁰·⁵/mol²，源项分析论文表 3.2
    constexpr double b_co = 2.735e-5, b_kr = 2.743e-5, b_xe = 3.538e-5; // m³/mol，同上
    const double kr = kr_fraction * fission_gas, xe = xe_fraction * fission_gas;
    const double excluded_volume = co * b_co + kr * b_kr + xe * b_xe; // B = n b_mix，m³
    const double attraction = co * std::sqrt(a_co) + kr * std::sqrt(a_kr) + xe * std::sqrt(a_xe); // A = Σ nᵢ√aᵢ
    return (co + fission_gas) * gas_constant * gas_temperature / (free_volume - excluded_volume) - attraction * attraction / (std::sqrt(gas_temperature) * free_volume * (free_volume + excluded_volume)); // RK 的总量形式，等价于摩尔体积形式，零气量时自然得到零压力
}

}
