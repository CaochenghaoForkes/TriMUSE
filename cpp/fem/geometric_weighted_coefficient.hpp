#pragma once

#include "fem/fem_system.hpp"

namespace trimuse
{


class GeometricWeightedCoefficient final : public mfem::Coefficient // 系数包装器: 把原来的物理系数 c 包装成含几何权重等新系数
{
public:
    GeometricWeightedCoefficient(mfem::Coefficient& coefficient, const FemSystem& fem_system) // mfem::Coefficient 对象能作为 MFEM 系数传给积分器
        : coefficient_(coefficient), fem_system_(fem_system) {} // coefficient：原始物理系数

    double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) override // 重写 MFEM 的系数求值接口 积分器需要系数时，就会调用它
    {
        return coefficient_.Eval(transformation, point) * fem_system_.geometric_weight(transformation, point); // fem_system_.geometric_weight(transformation, point) 根据问题维数返回 point 位置对应的几何权重 coefficient_.Eval(...) 当前积分点的物理系数 
    }

private:
    mfem::Coefficient& coefficient_;
    const FemSystem& fem_system_;
};

}
