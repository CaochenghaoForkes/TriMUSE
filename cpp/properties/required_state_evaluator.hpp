#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "fem/fem_system.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

enum class RequiredStateRange // 数据存储范围或有限元场
{
    Global,
    Region,
    Zone,
    Interface,
    DomainIntegrationPoint,
    FaceIntegrationPoint,
    Field
};
enum class RequiredStateOperation // 直接读取或标量汇总
{
    Value,
    Integral,
    Average,
    Maximum,
    AnyPositive
};

struct RequiredStateRequest
{
    std::string name;
    RequiredStateRange range;
    RequiredStateOperation operation = RequiredStateOperation::Value;
};

struct RequiredStateLocation
{
    int attribute = 0; // Region/Zone 的物理组编号，Field 的所属区域编号
    std::size_t index = 0; // DomainIntegrationPoint 的全域求积点编号、FaceIntegrationPoint 的面内求积点编号或 Interface 的网格面编号
    int element = 0; // Field 的原网格单元编号
    mfem::IntegrationPoint point; // Field 在该单元内的参考坐标
    int face = 0; // FaceIntegrationPoint 的原网格面编号
};

class RequiredStateEvaluator
{
public:
    RequiredStateEvaluator(const PropertiesFieldData& properties, const FemSystem& fem);
    StateValue evaluate(const RequiredStateRequest& request, const std::vector<RequiredStateLocation>& locations, const std::vector<double>& weights = {}) const; // 位置列表由调用方选定，积分与平均的权重必须与位置一一对应，AnyPositive 返回标量 1 或 0

private:
    StateValue read_value(const RequiredStateRequest& request, const RequiredStateLocation& location) const;

    const PropertiesFieldData& properties_;
    const FemSystem& fem_;
};

}
