#include "fem/field_data.hpp"

namespace trimuse
{

FieldData::FieldData(FiniteElementSpaceData& finite_element_space, double initial_value)
    : finite_element_space_(finite_element_space),
      grid_function_(&finite_element_space_.mfem_space())
{
    assign(initial_value);
}

int FieldData::size() const
{
    return grid_function_.Size();
}

void FieldData::assign(double value)
{
    grid_function_ = value;
}

double FieldData::value(int index) const
{
    return grid_function_[index];
}

void FieldData::set_value(int index, double value)
{
    grid_function_[index] = value;
}

mfem::GridFunction& FieldData::mfem_grid_function()
{
    return grid_function_;
}

const mfem::GridFunction& FieldData::mfem_grid_function() const
{
    return grid_function_;
}

}
