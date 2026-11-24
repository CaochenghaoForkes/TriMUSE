#include "physics/common/field/trial_field.hpp"

namespace trimuse
{

TrialField::TrialField(FemSystem& fem, RegionFieldData& accepted_field)
    :
    accepted_region_field_(&accepted_field)
{
    const auto attributes = fem.mesh_data().region_attributes();
    const auto& first_space = accepted_field.finite_element_space(attributes.front());
    trial_region_field_ = std::make_unique<RegionFieldData>(fem.mesh_data(), first_space.order(), first_space.vector_dimension());
    for (std::size_t i = 0; i < attributes.size(); ++i)
    {
        offsets_[attributes[i]] = accepted_field.dof_offsets()[i]; // 区域编号 -> 整体拼接向量中的自由度偏移
    }
    load();
}

int TrialField::size() const
{
    return trial_region_field_->dof_offsets().Last(); // 各区域完整自由度总数，尚未施加界面及 Dirichlet 约束
}

mfem::FiniteElementSpace& TrialField::space(int region) const
{
    return trial_region_field_->finite_element_space(region).mfem_space();
}

int TrialField::local_element(int element) const
{
    return trial_region_field_->local_element(element);
}

void TrialField::element_dofs(int element, mfem::Array<int>& dofs) const
{
    trial_region_field_->element_dofs(element, dofs); // 原网格单元编号 -> 分域场完整自由度编号
}

void TrialField::face_dofs(int region, int face, mfem::Array<int>& dofs) const
{
    space(region).GetFaceVDofs(trial_region_field_->local_face(region, face), dofs);
    for (int& dof : dofs)
    {
        dof = dof >= 0 ? dof + offsets_.at(region) : dof - offsets_.at(region); // 加区域偏移，保留 MFEM 的有符号自由度编码
    }
}

mfem::Vector TrialField::values() const
{
    mfem::Vector result;
    trial_region_field_->get_values(result);
    return result;
}

void TrialField::set_values(const mfem::Vector& values)
{
    trial_region_field_->set_values(values);
}

void TrialField::load()
{
    mfem::Vector values;
    accepted_region_field_->get_values(values);
    trial_region_field_->set_values(values); // 正式场 -> 试探场
}

void TrialField::accept()
{
    accepted_region_field_->set_values(values()); // 试探场 -> 正式场
}

FieldView TrialField::view() const
{
    return trial_region_field_.get();
}

}
