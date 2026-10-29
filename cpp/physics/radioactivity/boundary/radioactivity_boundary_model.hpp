#pragma once

#include "common/model_parameters.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct RadioactivityBoundaryConfig
{
    int boundary_attribute;
    std::string model_name;
    ModelParameterMap parameters;
};

struct RadioactivityBoundaryValues
{
    double matrix = 0.0; // Robin 的 h_m
    double load = 0.0; // Robin 的 h_m*c_infinity 或 Neumann 的 -j_n
    double concentration = 0.0; // Dirichlet 指定浓度，atom/m³
};

enum class RadioactivityBoundaryPart { Matrix, Load, Essential };

class RadioactivityBoundaryModel
{
public:
    virtual ~RadioactivityBoundaryModel() = default;

    virtual bool essential() const { return false; }

    virtual RadioactivityBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const = 0;

protected:
    static double scalar(const PropertiesFieldData& properties, const std::string& name, int zone)
    {
        return std::get<double>(properties.zone_values.at(name).at(zone).value());
    }
};

}
