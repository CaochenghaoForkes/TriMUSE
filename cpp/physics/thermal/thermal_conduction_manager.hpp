#pragma once

#include <vector>

#include "fem/fem_system.hpp"
#include "fem/geometric_weighted_coefficient.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class ThermalConductionManager
{
public:
    explicit ThermalConductionManager(FemSystem& fem_system);

    void update(const PropertiesFieldData& properties_field_data); // 从指定时刻的属性更新导热系数

    void add_integrators(mfem::BilinearForm& bilinear_form); // 向左端矩阵登记导热项积分器

private:
    class ConductivityCoefficient final : public mfem::Coefficient
    {
    public:
        explicit ConductivityCoefficient(int element_count) : values(element_count) {}

        double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) override
        {
            return values[transformation.ElementNo][point.index];
        }

        std::vector<std::vector<double>> values; // 按单元编号和局部积分点编号保存导热系数
    };

    FemSystem& fem_system_;
    ConductivityCoefficient conductivity_coefficient_;
    GeometricWeightedCoefficient weighted_conductivity_coefficient_;
};

}
