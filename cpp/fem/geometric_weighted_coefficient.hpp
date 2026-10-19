#pragma once

#include "fem/fem_system.hpp"

namespace trimuse
{

// 只在积分时乘几何权重, 原始系数和 FemSystem 必须比包装对象存活更久
class GeometricWeightedCoefficient final : public mfem::Coefficient
{
public:
    GeometricWeightedCoefficient(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : coefficient_(coefficient), fem_system_(fem_system) {}

    double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) override
    {
        return coefficient_.Eval(transformation, point) * fem_system_.geometric_weight(transformation, point);
    }

private:
    mfem::Coefficient& coefficient_;
    const FemSystem& fem_system_;
};

}
