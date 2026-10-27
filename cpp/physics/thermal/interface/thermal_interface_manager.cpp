#include "physics/thermal/interface/thermal_interface_manager.hpp"
#include <numeric>
#include <cmath>
#include <unordered_map>
#include <stdexcept>

namespace trimuse
{
ThermalInterfaceManager::ThermalInterfaceManager(FemSystem& fem, ThermalField& field,
    const std::vector<ThermalInterfaceConfig>& configs)
{
    std::unordered_map<int, ThermalInterfaceConfig> selected;
    for (const auto& config : configs)
    {
        if (config.model_name != "perfect_contact" && config.model_name != "conductance" && config.model_name != "insulated")
        { throw std::invalid_argument("未知热界面模型: " + config.model_name); }
        if (!field.regional() && config.model_name != "perfect_contact")
        { throw std::invalid_argument("有限界面传热或绝热界面需要分域温度场"); }
        if (config.model_name == "conductance")
        {
            const double h = std::get<double>(config.parameters.at("heat_transfer_coef"));
            if (!std::isfinite(h) || h < 0) { throw std::invalid_argument("界面传热系数必须有限且非负"); }
        }
        selected.emplace(config.boundary_attribute, config);
    }
    matrix_ = std::make_unique<mfem::SparseMatrix>(field.size());
    if (!field.regional())
    {
        const auto* p = field.space(0).GetConformingProlongation();
        if (p) { prolongation_ = std::make_unique<mfem::SparseMatrix>(*p); }
        else
        {
            prolongation_ = std::make_unique<mfem::SparseMatrix>(field.size());
            for (int i = 0; i < field.size(); ++i) { prolongation_->Add(i, i, 1.0); }
            prolongation_->Finalize();
        }
        matrix_->Finalize();
        return;
    }

    auto& mesh = fem.mesh_data().mfem_mesh();
    const int order = field.space(fem.mesh_data().region_attributes().front()).GetMaxElementOrder();
    mfem::H1_FECollection collection(order, mesh.Dimension());
    mfem::FiniteElementSpace parent_space(&mesh, &collection);
    std::vector<int> representative(field.size());
    std::iota(representative.begin(), representative.end(), 0);
    auto root = [&](int dof)
    {
        while (representative[dof] != dof)
        {
            representative[dof] = representative[representative[dof]];
            dof = representative[dof];
        }
        return dof;
    };
    for (std::size_t i = 0; i < fem.mesh_data().interfaces().size(); ++i)
    {
        const auto& face = fem.mesh_data().interfaces()[i];
        const auto config = selected.find(face.boundary_attribute);
        const std::string model = config == selected.end() ? "perfect_contact" : config->second.model_name;
        if (model == "insulated") { continue; }
        mfem::Array<int> dofs[2];
        for (int side = 0; side < 2; ++side) { field.element_dofs(face.sides[side].element, dofs[side]); }
        if (model == "perfect_contact")
        {
            // 通过父空间的自由度标识配对, 不比较浮点坐标或假定两侧面的排列方向相同。
            std::unordered_map<int, int> first;
            mfem::Array<int> parent_dofs, face_dofs;
            parent_space.GetElementDofs(face.sides[0].element, parent_dofs);
            for (int j = 0; j < parent_dofs.Size(); ++j) { first[parent_dofs[j]] = dofs[0][j]; }
            parent_space.GetFaceDofs(face.face, face_dofs);
            parent_space.GetElementDofs(face.sides[1].element, parent_dofs);
            for (int j = 0; j < parent_dofs.Size(); ++j)
            {
                if (face_dofs.Find(parent_dofs[j]) != -1)
                { representative[root(dofs[1][j])] = root(first.at(parent_dofs[j])); }
            }
            continue;
        }

        const double h = std::get<double>(config->second.parameters.at("heat_transfer_coef"));
        auto& transformation = fem.mesh_data().interface_transformations(i);
        const auto* rule = fem.boundary_quadrature_rule(transformation.GetGeometryType());
        mfem::Array<int> joined(dofs[0].Size() + dofs[1].Size());
        for (int j = 0; j < dofs[0].Size(); ++j) { joined[j] = dofs[0][j]; }
        for (int j = 0; j < dofs[1].Size(); ++j) { joined[dofs[0].Size() + j] = dofs[1][j]; }
        mfem::Vector jump(joined.Size()), shapes[2];
        mfem::DenseMatrix local(joined.Size()); local = 0.0;
        for (int q = 0; q < rule->GetNPoints(); ++q)
        {
            const auto& point = rule->IntPoint(q);
            transformation.SetAllIntPoints(&point);
            for (int side = 0; side < 2; ++side)
            {
                shapes[side].SetSize(dofs[side].Size());
                const auto* fe = field.space(face.sides[side].region_attribute).GetFE(field.local_element(face.sides[side].element));
                fe->CalcShape(side == 0 ? transformation.GetElement1IntPoint() : transformation.GetElement2IntPoint(), shapes[side]);
            }
            for (int j = 0; j < shapes[0].Size(); ++j) { jump[j] = shapes[0][j]; }
            for (int j = 0; j < shapes[1].Size(); ++j) { jump[shapes[0].Size() + j] = -shapes[1][j]; }
            const double weight = h * point.weight * transformation.Weight() * fem.geometric_weight(transformation, point);
            mfem::AddMult_a_VVt(weight, jump, local);
        }
        matrix_->AddSubMatrix(joined, joined, local, 0);
    }
    std::unordered_map<int, int> columns;
    for (int i = 0; i < field.size(); ++i)
    {
        const int r = root(i);
        if (!columns.count(r)) { columns.emplace(r, columns.size()); }
    }
    prolongation_ = std::make_unique<mfem::SparseMatrix>(field.size(), columns.size());
    for (int i = 0; i < field.size(); ++i) { prolongation_->Add(i, columns.at(root(i)), 1.0); }
    prolongation_->Finalize();
    matrix_->Finalize();
}
}
