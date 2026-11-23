#include "properties/assembly/local_panama_opf/panama.hpp"

#include <algorithm>
#include <cmath>

namespace trimuse
{

PanamaOpf::PanamaOpf(const ModelParameterMap& parameters)
    :
    accident_time_(std::get<double>(parameters.at("accident_time")))
{
}

VectorValue PanamaOpf::evaluate(const PropertiesStatePoint& point) const
{
    const double temperature = point.scalar("temperature");
    const double time = point.scalar("external.time"); // 当前时间步末时刻
    const double time_step = point.scalar("external.time_step");
    VectorValue state{0.0, 0.0, 0.0, 0.0, temperature};
    if (point.previous_value)
    {
        state = std::get<VectorValue>(*point.previous_value); // 迭代和重试均从已接受历史推进
    }
    double& history = state[LocalPanamaOpfState::history_integral]; // H
    double& integral = state[LocalPanamaOpfState::h_integral]; // J
    double& irradiation_time = state[LocalPanamaOpfState::irradiation_time]; // t_irr
    const double irradiation_step = std::clamp(accident_time_ - (time - time_step), 0.0, time_step); // accident_time_ - (time - time_step) 从当前步起点到事故发生，还剩多少时间 std::clamp(value, 0.0, time_step) 将结果限制在 [0, time_step] 内： 
    if (irradiation_step > 0.0) // 正常工况
    {
        const double previous_temperature = state[LocalPanamaOpfState::previous_temperature]; // Tk
        const double irradiation_end_temperature = previous_temperature + (temperature - previous_temperature) * irradiation_step / time_step; // 插值得到照部分终点的温度
        const double midpoint_temperature = 0.5 * (previous_temperature + irradiation_end_temperature); // 得到辐照部分的中点温度
        const double h = std::pow(10.0, -10.08 - 8500.0 / midpoint_temperature); // 计算 h(Tmid)
        history += 2.0 * integral * irradiation_step + h * irradiation_step * irradiation_step; // Hⁿ⁺¹ = Hⁿ + 2Jⁿ Δt + h(Tmid) Δt²
        integral += h * irradiation_step; // Jⁿ⁺¹ = Jⁿ + h(Tmid) Δt
        irradiation_time += irradiation_step;
    }
    double opf = history; // 正常工况的 OPF
    if (time > accident_time_ && history > 0.0) // 事故工况
    {
        const double irradiation_temperature = -8500.0 / (std::log10(history / (irradiation_time * irradiation_time)) + 10.08); // 用未截断的 H 反推等效辐照温度
        opf *= std::pow(10.0, -4040.0 * (1.0 / temperature - 1.0 / (irradiation_temperature + 75.0))); // PANAMA UO₂ 事故加热修正，H、J 和辐照时长保持冻结
    }
    state[LocalPanamaOpfState::opf] = std::min(0.625, opf);
    state[LocalPanamaOpfState::previous_temperature] = temperature;
    return state;
}

}
