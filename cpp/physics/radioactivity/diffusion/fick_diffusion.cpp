#include "physics/radioactivity/diffusion/fick_diffusion.hpp"

namespace trimuse
{

FickDiffusion::FickDiffusion(const ModelParameterMap&)
{
}

double FickDiffusion::evaluate(const PropertiesFieldData& properties, std::size_t point) const
{
    return scalar(properties, "material.radioactivity.nuclide_diffusion_coef", point);
}

}
