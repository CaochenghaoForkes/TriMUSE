#include "physics/thermal/interface/thermal_interface_assembly.hpp"

#include <numeric>
#include <unordered_map>

namespace trimuse
{

ThermalInterfaceAssembly::ThermalInterfaceAssembly(FemSystem& fem, const ThermalField& field)
    : field_(field)
{
    if (!field.regional()) { return; }
    const int order = field.space(fem.mesh_data().region_attributes().front()).GetMaxElementOrder();
    collection_ = std::make_unique<mfem::H1_FECollection>(order, fem.mesh_data().dimension());
    parent_space_ = std::make_unique<mfem::FiniteElementSpace>(&fem.mesh_data().mfem_mesh(), collection_.get());
    representative_.resize(field.size());
    std::iota(representative_.begin(), representative_.end(), 0);
}

int ThermalInterfaceAssembly::root(int dof)
{
    while (representative_[dof] != dof)
    {
        representative_[dof] = representative_[representative_[dof]];
        dof = representative_[dof];
    }
    return dof;
}

void ThermalInterfaceAssembly::connect(const ThermalInterfaceContext& context)
{
    if (!field_.regional()) { return; } // 连续场已共享界面自由度
    const auto& interface = context.interface();
    mfem::Array<int> dofs[2];
    for (int side = 0; side < 2; ++side) { field_.element_dofs(interface.sides[side].element, dofs[side]); }

    // 以父连续空间的自由度编号配对, 不比较坐标或假定两侧面的局部排列相同。
    std::unordered_map<int, int> first;
    mfem::Array<int> parent_dofs, face_dofs;
    parent_space_->GetElementDofs(interface.sides[0].element, parent_dofs);
    for (int j = 0; j < parent_dofs.Size(); ++j) { first[parent_dofs[j]] = dofs[0][j]; }
    parent_space_->GetFaceDofs(interface.face, face_dofs);
    parent_space_->GetElementDofs(interface.sides[1].element, parent_dofs);
    for (int j = 0; j < parent_dofs.Size(); ++j)
    {
        if (face_dofs.Find(parent_dofs[j]) != -1)
        {
            representative_[root(dofs[1][j])] = root(first.at(parent_dofs[j]));
        }
    }
}

std::unique_ptr<mfem::SparseMatrix> ThermalInterfaceAssembly::finalize()
{
    if (!field_.regional())
    {
        const auto* conforming = field_.space(0).GetConformingProlongation();
        if (conforming) { return std::make_unique<mfem::SparseMatrix>(*conforming); }
        auto identity = std::make_unique<mfem::SparseMatrix>(field_.size());
        for (int i = 0; i < field_.size(); ++i) { identity->Add(i, i, 1.0); }
        identity->Finalize();
        return identity;
    }

    std::unordered_map<int, int> columns;
    for (int i = 0; i < field_.size(); ++i)
    {
        const int representative = root(i);
        if (!columns.count(representative)) { columns.emplace(representative, columns.size()); }
    }
    auto prolongation = std::make_unique<mfem::SparseMatrix>(field_.size(), columns.size());
    for (int i = 0; i < field_.size(); ++i) { prolongation->Add(i, columns.at(root(i)), 1.0); }
    prolongation->Finalize();
    return prolongation;
}

}
