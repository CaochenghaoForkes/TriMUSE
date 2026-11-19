#pragma once

#include <variant>

#include "common/model_parameters.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct MechanicsBoundaryConfig // 外表面分区采用的力学边界模型及其构造参数 包含 边界区域编号 模型名 构造参数
{
    int boundary_attribute;
    std::string model_name;
    ModelParameterMap parameters;
};

enum class MechanicsBoundaryType
{
    Traction,
    Displacement,
    Pressure
}; // 三类力学边界 0-Traction 面力向量 1-Displacement 各向位移 2-Pressure 法向压力

using MechanicsBoundaryValue = std::variant<VectorValue, double>; // 统一保存不同模型的计算结果 面力、位移保存求解坐标系中的向量，压力保存标量；每个分区只有一种边界条件

class MechanicsBoundaryModel // 所有边界模型共同遵守的接口
{
public:
    virtual ~MechanicsBoundaryModel() = default;
    virtual MechanicsBoundaryType type() const = 0;
    virtual MechanicsBoundaryValue evaluate(const PropertiesFieldData& properties, int zone) const = 0;
};

}
