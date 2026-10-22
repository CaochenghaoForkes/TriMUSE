#pragma once

#include <vector>

#include "fem/fem_system.hpp"
#include "fem/geometric_weighted_coefficient.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class RadioactivityDiffusionManager
{
public:
    explicit RadioactivityDiffusionManager(FemSystem& fem_system);

    void update(const PropertiesFieldData& properties_field_data);

    void add_integrators(mfem::BilinearForm& bilinear_form);

private:
    class DiffusionCoefficient final : public mfem::Coefficient
    {
    public:
        explicit DiffusionCoefficient(int element_count) : values(element_count) {}

        double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) override
        {
            return values[transformation.ElementNo][point.index];
        }

        std::vector<std::vector<double>> values; // 单元编号、局部积分点编号 -> D
    };

    FemSystem& fem_system_;
    DiffusionCoefficient diffusion_coefficient_;
    GeometricWeightedCoefficient weighted_diffusion_coefficient_;
};

}
