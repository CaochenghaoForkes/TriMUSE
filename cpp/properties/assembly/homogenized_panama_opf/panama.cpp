#include "properties/assembly/homogenized_panama_opf/panama.hpp"

namespace trimuse
{

HomogenizedPanamaOpf::HomogenizedPanamaOpf(const ModelParameterMap& parameters)
    :
    panama_model_(parameters)
{
}

VectorValue HomogenizedPanamaOpf::evaluate(const PropertiesStatePoint& point) const
{
    const auto& temperatures = point.domain_values("temperature"); // 全域温度采样，通过当前区域积分点编号读取
    const auto& weights = point.domain_values("precomputed.initial_domain_quadrature_weight");
    double integral = 0.0, volume = 0.0;
    const auto& indices = point.region_point_indices();
    for (std::size_t q = 0; q < indices.size(); ++q)
    {
        const double weight = std::get<double>(weights[indices[q]].value());
        integral += weight * std::get<double>(temperatures[indices[q]].value());
        volume += weight;
    }
    PropertiesStatePoint averaged_point = point;
    averaged_point.values["temperature"] = integral / volume; // 先计算体积平均温度，再按前后两步平均温度计算 h(Tmid)
    return panama_model_.evaluate(averaged_point);
}

}
