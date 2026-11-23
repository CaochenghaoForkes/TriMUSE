#include "properties/assembly/internal_gas_pressure/ideal_gas_from_co_fission_gas_release.hpp"

namespace trimuse
{

IdealGasFromCoFissionGasRelease::IdealGasFromCoFissionGasRelease(const ModelParameterMap& parameters)
    :
    source_region_(static_cast<int>(std::get<double>(parameters.at("source_region"))))
{
}

double IdealGasFromCoFissionGasRelease::evaluate(const PropertiesStatePoint& point) const
{
    const double fission_gas = std::get<double>(point.attribute_values("assembly.cumulative_fission_gas_release").at(source_region_).value()); // mol，来源区域释放到自由空间的裂变气体
    const double co = std::get<double>(point.attribute_values("assembly.cumulative_co_release").at(source_region_).value()); // mol，来源区域的 CO
    const auto& porosities = point.domain_values("material.general.porosity");
    const auto& weights = point.domain_values("precomputed.initial_domain_quadrature_weight");
    const auto& temperatures = point.vector("temperature");
    const auto& indices = point.region_point_indices(); // 当前求值区域就是 gas_region
    double free_volume = 0.0, temperature_integral = 0.0;
    for (std::size_t q = 0; q < indices.size(); ++q)
    {
        const std::size_t index = indices[q];
        const double pore_volume = std::get<double>(weights[index].value()) * std::get<double>(porosities[index].value());
        free_volume += pore_volume;
        temperature_integral += pore_volume * temperatures[q];
    }
    const double gas_temperature = temperature_integral / free_volume; // K，按孔隙体积加权的代表温度
    constexpr double gas_constant = 8.31446261815324; // Pa·m³/(mol·K)
    return (fission_gas + co) * gas_constant * gas_temperature / free_volume; // Pa，p = nRT/Vfree
}

}
