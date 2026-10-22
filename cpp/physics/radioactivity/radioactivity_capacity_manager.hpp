#pragma once

#include "fem/fem_system.hpp"
#include "fem/geometric_weighted_coefficient.hpp"

namespace trimuse
{

class RadioactivityCapacityManager
{
public:
    explicit RadioactivityCapacityManager(const FemSystem& fem_system);

    void add_integrators(mfem::BilinearForm& capacity_form);

private:
    const FemSystem& fem_system_;
    mfem::ConstantCoefficient unit_coefficient_{1.0};
    GeometricWeightedCoefficient weighted_unit_coefficient_;
};

}
