#include "properties/assembly/cumulative_co_release/from_local_panama_opf.hpp"
#include "properties/assembly/local_panama_opf/panama.hpp"

namespace trimuse
{

FromLocalPanamaOpf::FromLocalPanamaOpf(const ModelParameterMap&)
{
}

double FromLocalPanamaOpf::evaluate(const PropertiesStatePoint& point) const
{
    const auto& states = point.domain_values("assembly.local_panama_opf");
    const auto& weights = point.domain_values("precomputed.initial_domain_quadrature_weight");
    double integral = 0.0, volume = 0.0;
    for (std::size_t index : point.region_point_indices())
    {
        const double weight = std::get<double>(weights[index].value());
        integral += weight * std::get<VectorValue>(states[index].value())[LocalPanamaOpfState::opf];
        volume += weight;
    }
    const double opf = integral / volume; // 初始重金属在区域内均匀分布，使用 OPF 的体积平均值
    constexpr double avogadro_constant = 6.02214076e23; // mol⁻¹
    return point.scalar("external.initial_heavy_metal_inventory") / avogadro_constant * point.scalar("derived.burnup_fima_fraction") * opf; // mol，进入颗粒内部自由空间的 CO 累计量
}

}
