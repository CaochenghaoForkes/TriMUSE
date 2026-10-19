#pragma once

#include <vector>

#include "fem/fem_system.hpp"
#include "fem/geometric_weighted_coefficient.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class ThermalCapacityManager
{
public:
    explicit ThermalCapacityManager(FemSystem& fem_system);

    void update(const PropertiesFieldData& properties_field_data); // 从指定时刻的属性读取 rho cp

    void add_integrators(mfem::BilinearForm& capacity_form); // 向独立的热容量矩阵登记质量积分器

private:
    class CapacityCoefficient final : public mfem::Coefficient
    {
    public:
        explicit CapacityCoefficient(int element_count) : values(element_count) {}

        double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) override
        {
            return values[transformation.ElementNo][point.index];
        }

        std::vector<std::vector<double>> values; // 按单元编号和局部积分点编号保存 rho cp
    };

    FemSystem& fem_system_;
    CapacityCoefficient capacity_coefficient_;
    GeometricWeightedCoefficient weighted_capacity_coefficient_;
};

}
