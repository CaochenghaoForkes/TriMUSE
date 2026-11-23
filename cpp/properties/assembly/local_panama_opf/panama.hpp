#pragma once

#include <cstddef>
#include "properties/assembly/local_panama_opf/local_panama_opf_model_manager.hpp"

namespace trimuse
{

struct LocalPanamaOpfState
{
    inline static constexpr std::size_t opf = 0; // 每次裂变释放的氧原子数，已限制上限
    inline static constexpr std::size_t history_integral = 1; // H，未截断的历史积分
    inline static constexpr std::size_t h_integral = 2; // J = ∫ h(T) dt
    inline static constexpr std::size_t irradiation_time = 3; // s，停堆前累计辐照时长
    inline static constexpr std::size_t previous_temperature = 4; // K，上一个已接受时刻的温度
};

class PanamaOpf final : public LocalPanamaOpfModel
{
public:
    explicit PanamaOpf(const ModelParameterMap& parameters);
    VectorValue evaluate(const PropertiesStatePoint& point) const override;

private:
    double accident_time_; // s，停止辐照并进入事故阶段的时刻
};

}
