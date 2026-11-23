#pragma once

#include <cstddef>
#include "properties/assembly/local_fission_gas_release_state/local_fission_gas_release_state_model_manager.hpp"

namespace trimuse
{

struct LocalFissionGasReleaseState // 每个域求积点保存一个长度为 mode_count + 3 的向量
{
    inline static constexpr std::size_t released_density = 0; // mol/m³，累计释放量密度
    inline static constexpr std::size_t produced_density = 1; // mol/m³，累计产生量密度
    inline static constexpr std::size_t release_fraction = 2; // 无量纲，累计释放份额
    inline static constexpr std::size_t modes_begin = 3; // 从此处依次保存 z₁、z₂、…、zM，单位 mol/m³
};

class BoothLocalFissionGasReleaseState final : public LocalFissionGasReleaseStateModel
{
public:
    explicit BoothLocalFissionGasReleaseState(const ModelParameterMap& parameters);
    VectorValue evaluate(const PropertiesStatePoint& point) const override;

private:
    std::vector<double> weights_; // Aₙ = 6/(n²π²)
    std::vector<double> diffusion_factors_; // n²π²/a²，单位 m⁻²
};

}
