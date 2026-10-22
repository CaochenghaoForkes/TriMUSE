#pragma once

#include <string>
#include <vector>

#include "common/model_parameters.hpp"
#include "fem/fem_system.hpp"
#include "fem/geometric_weighted_coefficient.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct RadioactivitySourceConfig
{
    int region_attribute;
    std::string model_name;
    ModelParameterMap parameters;
};

class RadioactivitySourceManager
{
public:
    RadioactivitySourceManager(FemSystem& fem_system, const std::vector<RadioactivitySourceConfig>& source_configs);

    void update(const PropertiesFieldData& properties_field_data);

    void add_integrators(mfem::LinearForm& linear_form);

private:
    class SourceCoefficient final : public mfem::Coefficient
    {
    public:
        explicit SourceCoefficient(int element_count) : values(element_count) {}

        double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) override
        {
            return values[transformation.ElementNo][point.index];
        }

        std::vector<std::vector<double>> values; // 单元编号、局部积分点编号 -> 体积产生率
    };

    FemSystem& fem_system_;
    std::vector<RadioactivitySourceConfig> source_configs_;
    std::vector<mfem::Array<int>> region_markers_;
    SourceCoefficient source_coefficient_;
    GeometricWeightedCoefficient weighted_source_coefficient_;
};

}
