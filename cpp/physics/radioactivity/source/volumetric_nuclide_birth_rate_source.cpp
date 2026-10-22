#include "physics/radioactivity/source/volumetric_nuclide_birth_rate_source.hpp"

#include <variant>

namespace trimuse
{

double VolumetricNuclideBirthRateSource::evaluate(const PropertiesFieldData& properties_field_data, std::size_t point_index)
{
    return std::get<double>(properties_field_data.integration_point_values.at("volumetric_nuclide_birth_rate")[point_index].value()); // atom/(m³·s)
}

}
