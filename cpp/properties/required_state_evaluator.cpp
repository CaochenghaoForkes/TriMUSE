#include "properties/required_state_evaluator.hpp"

#include "common/value_reduction.hpp"

namespace trimuse
{

RequiredStateEvaluator::RequiredStateEvaluator(const PropertiesFieldData& properties, const FemSystem& fem)
    :
    properties_(properties),
    fem_(fem)
{
}

StateValue RequiredStateEvaluator::read_value(const RequiredStateRequest& request, const RequiredStateLocation& location) const
{
    if (request.range == RequiredStateRange::Global)
    {
        return properties_.global_values.at(request.name).value();
    }
    if (request.range == RequiredStateRange::Region)
    {
        return properties_.region_values.at(request.name).at(location.attribute).value();
    }
    if (request.range == RequiredStateRange::Zone)
    {
        return properties_.zone_values.at(request.name).at(location.attribute).value();
    }
    if (request.range == RequiredStateRange::Interface)
    {
        return properties_.interface_values.at(request.name).at(static_cast<int>(location.index)).value();
    }
    if (request.range == RequiredStateRange::DomainIntegrationPoint)
    {
        return properties_.domain_integration_point_values.at(request.name)[location.index].value();
    }
    if (request.range == RequiredStateRange::FaceIntegrationPoint)
    {
        return properties_.face_integration_point_values.at(request.name)[location.face][location.index].value();
    }
    return fem_.field_view(request.name).evaluate(location.attribute, location.element, location.point); // Field 在传入的参考位置求值
}

StateValue RequiredStateEvaluator::evaluate(const RequiredStateRequest& request, const std::vector<RequiredStateLocation>& locations, const std::vector<double>& weights) const
{
    if (request.operation == RequiredStateOperation::Value)
    {
        return read_value(request, locations.front()); // 直接读取保留原始 StateValue 类型
    }
    std::vector<double> values;
    values.reserve(locations.size());
    for (const auto& location : locations)
    {
        values.push_back(std::get<double>(read_value(request, location))); // 当前汇总操作只处理标量
    }
    if (request.operation == RequiredStateOperation::Integral)
    {
        return weighted_integral(values, weights);
    }
    if (request.operation == RequiredStateOperation::Average)
    {
        return weighted_average(values, weights);
    }
    if (request.operation == RequiredStateOperation::Maximum)
    {
        return maximum(values);
    }
    return any_positive(values) ? 1.0 : 0.0; // AnyPositive 沿用属性系统的标量表示
}

}
