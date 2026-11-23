#include "properties/assembly/homogenized_booth_fission_gas_release_state/booth.hpp"

#include <cmath>

namespace trimuse
{

BoothHomogenizedFissionGasReleaseState::BoothHomogenizedFissionGasReleaseState(const ModelParameterMap& parameters)
{
    const int mode_count = static_cast<int>(std::get<double>(parameters.at("mode_count")));
    const double grain_radius = std::get<double>(parameters.at("grain_radius")); // m，固定等效晶粒半径
    constexpr double pi = 3.14159265358979323846;
    for (int n = 1; n <= mode_count; ++n)
    {
        const double eigenvalue = static_cast<double>(n) * n * pi * pi;
        weights_.push_back(6.0 / eigenvalue); // An
        diffusion_factors_.push_back(eigenvalue / (grain_radius * grain_radius)); // n2pi2/a2
    }
}

VectorValue BoothHomogenizedFissionGasReleaseState::evaluate(const PropertiesStatePoint& point) const
{
    VectorValue state(HomogenizedBoothFissionGasReleaseState::modes_begin + weights_.size(), 0.0);
    if (point.previous_value)
    {
        state = std::get<VectorValue>(*point.previous_value); // 每次从已接受历史重新推进，试算与耦合迭代不重复累积
    }
    const auto& coefficients = point.domain_values("material.radioactivity.fission_gas_diffusion_coef");
    const auto& quadrature_weights = point.domain_values("precomputed.initial_domain_quadrature_weight");
    double integral = 0.0, volume = 0.0;
    for (std::size_t index : point.region_point_indices())
    {
        const double weight = std::get<double>(quadrature_weights[index].value());
        integral += weight * std::get<double>(coefficients[index].value());
        volume += weight;
    }
    const double diffusion = integral / volume; // Deff = Σ wq Dq / Σ wq，每步取区域体积平均扩散系数
    const double time_step = point.scalar("external.time_step"); // 实际时间步长，停堆后仍推进扩散
    const double produced_increment = point.scalar("derived.fission_gas_production_rate") / point.scalar("derived.region_volume") * time_step; // g Δt，mol/m³
    state[HomogenizedBoothFissionGasReleaseState::produced_density] += produced_increment;
    state[HomogenizedBoothFissionGasReleaseState::released_density] = 0.0;
    for (std::size_t mode = 0; mode < weights_.size(); ++mode)
    {
        const double h = diffusion_factors_[mode] * diffusion * time_step;
        double source_factor = 1.0; // h = 0 时 (1 − exp(−h))/h 的极限
        if (h > 0.0)
        {
            source_factor = -std::expm1(-h) / h; // expm1 避免小 h 时 1 − exp(−h) 相减损失精度
        }
        double& z = state[HomogenizedBoothFissionGasReleaseState::modes_begin + mode];
        z = std::exp(-h) * z + produced_increment * source_factor;
        state[HomogenizedBoothFissionGasReleaseState::released_density] += weights_[mode] * (state[HomogenizedBoothFissionGasReleaseState::produced_density] - z); // nrel = Σ Aₙ(P − zₙ)，保留前 M 项
    }
    state[HomogenizedBoothFissionGasReleaseState::release_fraction] = 0.0;
    if (state[HomogenizedBoothFissionGasReleaseState::produced_density] > 0.0)
    {
        state[HomogenizedBoothFissionGasReleaseState::release_fraction] = state[HomogenizedBoothFissionGasReleaseState::released_density] / state[HomogenizedBoothFissionGasReleaseState::produced_density];
    }
    return state;
}

}
