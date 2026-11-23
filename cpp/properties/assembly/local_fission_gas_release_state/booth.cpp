#include "properties/assembly/local_fission_gas_release_state/booth.hpp"

#include <cmath>

namespace trimuse
{

BoothLocalFissionGasReleaseState::BoothLocalFissionGasReleaseState(const ModelParameterMap& parameters)
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

VectorValue BoothLocalFissionGasReleaseState::evaluate(const PropertiesStatePoint& point) const
{
    VectorValue state(LocalFissionGasReleaseState::modes_begin + weights_.size(), 0.0);
    if (point.previous_value)
    {
        state = std::get<VectorValue>(*point.previous_value); // 每次从已接受历史重新推进，试算与耦合迭代不重复累积
    }
    const double diffusion = point.scalar("material.radioactivity.fission_gas_diffusion_coef"); // m²/s，当前步内取常数
    const double time_step = point.scalar("external.time_step"); // 实际时间步长，停堆后仍推进扩散
    const double produced_increment = point.scalar("derived.fission_gas_production_rate") / point.scalar("derived.region_volume") * time_step; // g Δt，mol/m³
    state[LocalFissionGasReleaseState::produced_density] += produced_increment;
    state[LocalFissionGasReleaseState::released_density] = 0.0;
    for (std::size_t mode = 0; mode < weights_.size(); ++mode)
    {
        const double h = diffusion_factors_[mode] * diffusion * time_step;
        double source_factor = 1.0; // h = 0 时 (1 − exp(−h))/h 的极限
        if (h > 0.0)
        {
            source_factor = -std::expm1(-h) / h; // expm1 避免小 h 时 1 − exp(−h) 相减损失精度
        }
        double& z = state[LocalFissionGasReleaseState::modes_begin + mode];
        z = std::exp(-h) * z + produced_increment * source_factor;
        state[LocalFissionGasReleaseState::released_density] += weights_[mode] * (state[LocalFissionGasReleaseState::produced_density] - z); // nrel = Σ Aₙ(P − zₙ)，保留前 M 项
    }
    state[LocalFissionGasReleaseState::release_fraction] = 0.0;
    if (state[LocalFissionGasReleaseState::produced_density] > 0.0)
    {
        state[LocalFissionGasReleaseState::release_fraction] = state[LocalFissionGasReleaseState::released_density] / state[LocalFissionGasReleaseState::produced_density];
    }
    return state;
}

}
