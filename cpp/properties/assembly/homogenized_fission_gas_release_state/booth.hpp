#pragma once

#include <cstddef>
#include "properties/assembly/homogenized_fission_gas_release_state/homogenized_fission_gas_release_state_model_manager.hpp"

namespace trimuse
{

struct HomogenizedFissionGasReleaseState // 每个区域保存一个长度为 mode_count + 3 的均匀化状态向量
{
    inline static constexpr std::size_t released_density = 0; // mol/m³，累计释放量密度
    inline static constexpr std::size_t produced_density = 1; // mol/m³，累计产生量密度
    inline static constexpr std::size_t release_fraction = 2; // 无量纲，累计释放份额
    inline static constexpr std::size_t modes_begin = 3; // 从此处依次保存 z₁、z₂、…、zM，单位 mol/m³
};

class BoothHomogenizedFissionGasReleaseState final : public HomogenizedFissionGasReleaseStateModel
{
public:
    explicit BoothHomogenizedFissionGasReleaseState(const ModelParameterMap& parameters);
    VectorValue evaluate(const PropertiesStatePoint& point) const override;

private:
    std::vector<double> weights_; // Aₙ = 6/(n²π²)
    std::vector<double> diffusion_factors_; // n²π²/a²，单位 m⁻²
};

}
