#include "physics/thermal/thermal_assembly.hpp"
#include <algorithm>
#include <stdexcept>

namespace trimuse
{
namespace
{
// 始终使用原网格的单元映射积分, 子网格仅提供有限元和自由度, 因而属性下标仍为原网格编号。
class PointCoefficient final : public mfem::Coefficient
{
public:
    PointCoefficient(const FemSystem& fem, const std::vector<double>& values) : fem_(fem), values_(values) {}
    double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) override
    {
        return values_[fem_.domain_quadrature_offset(transformation.ElementNo) + point.index]
             * fem_.geometric_weight(transformation, point);
    }
private:
    const FemSystem& fem_;
    const std::vector<double>& values_;
};

std::shared_ptr<mfem::SparseMatrix> sum_matrices(const std::map<int, std::shared_ptr<const mfem::SparseMatrix>>& parts, int size)
{
    auto sum = std::make_shared<mfem::SparseMatrix>(size);
    for (const auto& [region, matrix] : parts)
    {
        const auto* rows = matrix->GetI(); const auto* columns = matrix->GetJ(); const auto* data = matrix->GetData();
        for (int row = 0; row < size; ++row)
        for (int j = rows[row]; j < rows[row + 1]; ++j) { sum->Add(row, columns[j], data[j]); }
    }
    sum->Finalize();
    return sum;
}
std::shared_ptr<mfem::Vector> sum_vectors(const std::map<int, std::shared_ptr<const mfem::Vector>>& parts, int size)
{
    auto sum = std::make_shared<mfem::Vector>(size); *sum = 0.0;
    for (const auto& [region, vector] : parts) { *sum += *vector; }
    return sum;
}
}

ThermalAssembly::ThermalAssembly(FemSystem& fem, ThermalField& field,
    const std::vector<ThermalDomainConfig>& sources, const std::vector<ThermalBoundaryConfig>& boundaries,
    const std::vector<ThermalInterfaceConfig>& interfaces)
    : fem_(fem), field_(field), conduction_(fem), capacity_(fem), source_(fem, sources),
      boundary_(boundaries), interface_(fem, field, interfaces)
{
    const auto& mesh = fem.mesh_data().mfem_mesh();
    for (int zone : boundary_.zones())
    for (int b = 0; b < mesh.GetNBE(); ++b)
    {
        int first, second;
        mesh.GetFaceElements(mesh.GetBdrElementFaceIndex(b), &first, &second);
        if (second < 0 && mesh.GetBdrAttribute(b) == zone) { boundary_elements_[zone].push_back(b); }
    }
}
void ThermalAssembly::mark_coupled(PropertiesSystem& properties, bool transient) const
{
    conduction_.mark_coupled(properties, "temperature");
    source_.mark_coupled(properties, "temperature");
    boundary_.mark_coupled(properties, "temperature");
    if (transient) { capacity_.mark_coupled(properties, "temperature"); }
}
bool ThermalAssembly::coupled(const PropertiesSystem& properties, bool transient) const
{
    return conduction_.coupled(properties, "temperature") || source_.coupled(properties, "temperature")
        || boundary_.coupled(properties, "temperature") || (transient && capacity_.coupled(properties, "temperature"));
}

std::shared_ptr<mfem::SparseMatrix> ThermalAssembly::domain_matrix(int region, const std::vector<double>& values, bool diffusion)
{
    auto result = std::make_shared<mfem::SparseMatrix>(field_.size());
    PointCoefficient coefficient(fem_, values);
    std::unique_ptr<mfem::BilinearFormIntegrator> integrator;
    if (diffusion) { integrator = std::make_unique<mfem::DiffusionIntegrator>(coefficient); }
    else { integrator = std::make_unique<mfem::MassIntegrator>(coefficient); }
    mfem::IsoparametricTransformation transformation;
    mfem::DenseMatrix local;
    mfem::Array<int> dofs;
    for (int element : fem_.mesh_data().region_elements(region))
    {
        const auto* fe = field_.space(region).GetFE(field_.local_element(element));
        fem_.mesh_data().mfem_mesh().GetElementTransformation(element, &transformation);
        integrator->SetIntRule(fem_.domain_quadrature_rule(fe->GetGeomType()));
        integrator->AssembleElementMatrix(*fe, transformation, local);
        field_.element_dofs(element, dofs);
        result->AddSubMatrix(dofs, dofs, local, 0);
    }
    result->Finalize();
    return result;
}
std::shared_ptr<mfem::Vector> ThermalAssembly::domain_load(int region, const std::vector<double>& values)
{
    auto result = std::make_shared<mfem::Vector>(field_.size()); *result = 0.0;
    PointCoefficient coefficient(fem_, values);
    mfem::DomainLFIntegrator integrator(coefficient);
    mfem::IsoparametricTransformation transformation;
    mfem::Vector local;
    mfem::Array<int> dofs;
    for (int element : fem_.mesh_data().region_elements(region))
    {
        const auto* fe = field_.space(region).GetFE(field_.local_element(element));
        fem_.mesh_data().mfem_mesh().GetElementTransformation(element, &transformation);
        integrator.SetIntRule(fem_.domain_quadrature_rule(fe->GetGeomType()));
        integrator.AssembleRHSElementVect(*fe, transformation, local);
        field_.element_dofs(element, dofs);
        result->AddElementVector(dofs, local);
    }
    return result;
}

std::shared_ptr<mfem::SparseMatrix> ThermalAssembly::boundary_matrix(int zone)
{
    auto result = std::make_shared<mfem::SparseMatrix>(field_.size());
    const double h = boundary_.values(zone).matrix;
    if (h != 0.0)
    for (int b : boundary_elements_.at(zone))
    {
        auto* t = fem_.mesh_data().mfem_mesh().GetBdrFaceTransformations(b);
        const int element = t->Elem1No;
        const int region = fem_.mesh_data().mfem_mesh().GetAttribute(element);
        const auto* fe = field_.space(region).GetFE(field_.local_element(element));
        const auto* rule = fem_.boundary_quadrature_rule(t->GetGeometryType());
        mfem::Vector shape(fe->GetDof());
        mfem::DenseMatrix local(fe->GetDof()); local = 0.0;
        for (int q = 0; q < rule->GetNPoints(); ++q)
        {
            const auto& ip = rule->IntPoint(q);
            t->SetAllIntPoints(&ip);
            fe->CalcShape(t->GetElement1IntPoint(), shape);
            mfem::AddMult_a_VVt(h * ip.weight * t->Weight() * fem_.geometric_weight(*t, ip), shape, local);
        }
        mfem::Array<int> dofs; field_.element_dofs(element, dofs);
        result->AddSubMatrix(dofs, dofs, local, 0);
    }
    result->Finalize();
    return result;
}
std::shared_ptr<mfem::Vector> ThermalAssembly::boundary_load(int zone)
{
    auto result = std::make_shared<mfem::Vector>(field_.size()); *result = 0.0;
    const double value = boundary_.values(zone).load;
    if (value != 0.0)
    for (int b : boundary_elements_.at(zone))
    {
        auto* t = fem_.mesh_data().mfem_mesh().GetBdrFaceTransformations(b);
        const int element = t->Elem1No;
        const int region = fem_.mesh_data().mfem_mesh().GetAttribute(element);
        const auto* fe = field_.space(region).GetFE(field_.local_element(element));
        const auto* rule = fem_.boundary_quadrature_rule(t->GetGeometryType());
        mfem::Vector shape(fe->GetDof()), local(fe->GetDof()); local = 0.0;
        for (int q = 0; q < rule->GetNPoints(); ++q)
        {
            const auto& ip = rule->IntPoint(q);
            t->SetAllIntPoints(&ip);
            fe->CalcShape(t->GetElement1IntPoint(), shape);
            local.Add(value * ip.weight * t->Weight() * fem_.geometric_weight(*t, ip), shape);
        }
        mfem::Array<int> dofs; field_.element_dofs(element, dofs);
        result->AddElementVector(dofs, local);
    }
    return result;
}

void ThermalAssembly::refresh_matrix(ThermalDomainManager& manager, Cache<mfem::SparseMatrix>& cache,
    bool diffusion, const std::string& counter)
{
    bool changed = !cache.sum;
    for (int region : manager.regions())
    {
        if (cache.parts.count(region) && manager.required_no_change(region)) { continue; }
        cache.parts[region] = domain_matrix(region, manager.values(), diffusion);
        manager.assembled(region);
        ++counts_[counter]; changed = true;
    }
    if (changed) { cache.sum = sum_matrices(cache.parts, field_.size()); }
}
void ThermalAssembly::refresh_source()
{
    bool changed = !source_cache_.sum;
    for (int region : source_.regions())
    {
        if (source_cache_.parts.count(region) && source_.required_no_change(region)) { continue; }
        source_cache_.parts[region] = domain_load(region, source_.values());
        source_.assembled(region);
        ++counts_["source"]; changed = true;
    }
    if (changed) { source_cache_.sum = sum_vectors(source_cache_.parts, field_.size()); }
}
void ThermalAssembly::refresh_boundary()
{
    bool matrix_changed = !boundary_matrix_cache_.sum, load_changed = !boundary_load_cache_.sum;
    for (int zone : boundary_.zones())
    {
        if (boundary_.essential(zone)) { continue; }
        if (!boundary_matrix_cache_.parts.count(zone) || !boundary_.required_no_change(zone, BoundaryPart::Matrix))
        {
            boundary_matrix_cache_.parts[zone] = boundary_matrix(zone);
            boundary_.assembled(zone, BoundaryPart::Matrix);
            ++counts_["boundary_matrix"]; matrix_changed = true;
        }
        if (!boundary_load_cache_.parts.count(zone) || !boundary_.required_no_change(zone, BoundaryPart::Load))
        {
            boundary_load_cache_.parts[zone] = boundary_load(zone);
            boundary_.assembled(zone, BoundaryPart::Load);
            ++counts_["boundary_load"]; load_changed = true;
        }
    }
    if (matrix_changed) { boundary_matrix_cache_.sum = sum_matrices(boundary_matrix_cache_.parts, field_.size()); }
    if (load_changed) { boundary_load_cache_.sum = sum_vectors(boundary_load_cache_.parts, field_.size()); }
}
ThermalAssembly::State ThermalAssembly::update(const PropertiesFieldData& properties, bool capacity)
{
    if (capacity)
    {
        capacity_.update(properties);
        refresh_matrix(capacity_, mass_cache_, false, "capacity");
    }
    conduction_.update(properties);
    source_.update(properties);
    boundary_.update(properties);
    refresh_matrix(conduction_, conduction_cache_, true, "conduction");
    refresh_source();
    refresh_boundary();
    if (last_domain_ != conduction_cache_.sum || last_boundary_ != boundary_matrix_cache_.sum)
    {
        std::unique_ptr<mfem::SparseMatrix> conduction(
            mfem::Add(1.0, *conduction_cache_.sum, 1.0, *boundary_matrix_cache_.sum));
        if (const auto* interface_matrix = interface_.matrix())
        {
            state_.conduction.reset(mfem::Add(1.0, *conduction, 1.0, *interface_matrix));
        }
        else { state_.conduction = std::move(conduction); }
        last_domain_ = conduction_cache_.sum; last_boundary_ = boundary_matrix_cache_.sum;
    }
    if (last_source_ != source_cache_.sum || last_boundary_load_ != boundary_load_cache_.sum)
    {
        auto load = std::make_shared<mfem::Vector>(*source_cache_.sum);
        *load += *boundary_load_cache_.sum;
        state_.load = std::move(load);
        last_source_ = source_cache_.sum; last_boundary_load_ = boundary_load_cache_.sum;
    }
    state_.mass = mass_cache_.sum;
    return state_; // 只共享不可变的组装结果, 后续更新替换指针, 不改写旧时刻的矩阵。
}
void ThermalAssembly::update_boundary(const PropertiesFieldData& properties) { boundary_.update(properties); }

mfem::Vector ThermalAssembly::reduced_values() const
{
    const auto& p = interface_.prolongation();
    mfem::Vector full = field_.values(), reduced(p.Width()), ones(p.Height()), weights(p.Width());
    if (!field_.regional())
    {
        const auto* restriction = field_.space(0).GetConformingRestriction();
        if (restriction) { restriction->Mult(full, reduced); return reduced; }
        return full;
    }
    if (p.Height() == p.Width()) { return full; }
    ones = 1.0;
    p.MultTranspose(full, reduced);
    p.MultTranspose(ones, weights);
    for (int i = 0; i < reduced.Size(); ++i) { reduced[i] /= weights[i]; }
    return reduced;
}
void ThermalAssembly::essential_values(std::map<int, double>& values) const
{
    const auto& p = interface_.prolongation();
    for (int zone : boundary_.zones())
    {
        if (!boundary_.essential(zone)) { continue; }
        for (int b : boundary_elements_.at(zone))
        {
            const auto& mesh = fem_.mesh_data().mfem_mesh();
            int first, second;
            const int face = mesh.GetBdrElementFaceIndex(b);
            mesh.GetFaceElements(face, &first, &second);
            mfem::Array<int> dofs; field_.face_dofs(mesh.GetAttribute(first), face, dofs);
            for (int dof : dofs)
            for (int j = p.GetI()[dof]; j < p.GetI()[dof + 1]; ++j)
            {
                if (p.GetData()[j] != 0.0) { values[p.GetJ()[j]] = boundary_.values(zone).temperature; }
            }
        }
    }
}
void ThermalAssembly::apply_initial_constraints()
{
    auto temperature = reduced_values();
    std::map<int, double> essential; essential_values(essential);
    for (const auto& [dof, value] : essential) { temperature[dof] = value; }
    recover(temperature);
    for (int zone : boundary_.zones()) { boundary_.assembled(zone, BoundaryPart::Essential); }
}
void ThermalAssembly::form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs,
    mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& temperature) const
{
    const auto& p = interface_.prolongation();
    if (p.Height() == p.Width())
    {
        system = matrix;
        load = rhs;
    }
    else
    {
        std::unique_ptr<mfem::SparseMatrix> reduced(mfem::RAP(p, matrix, p));
        system = *reduced;
        load.SetSize(p.Width()); p.MultTranspose(rhs, load);
    }
    temperature = reduced_values();
    std::map<int, double> essential; essential_values(essential);
    for (const auto& [dof, value] : essential)
    {
        temperature[dof] = value;
        system.EliminateRowCol(dof, value, load);
    }
}
void ThermalAssembly::recover(const mfem::Vector& temperature)
{
    mfem::Vector full(field_.size());
    interface_.prolongation().Mult(temperature, full);
    field_.set_values(full);
}
void ThermalAssembly::project_initial()
{
    const auto& p = interface_.prolongation();
    auto guess = reduced_values();
    mfem::Vector original = field_.values(), projected(field_.size());
    p.Mult(guess, projected); projected -= original;
    if (projected.Normlinf() <= 1e-12 * std::max(1.0, original.Normlinf())) { return; }
    std::map<int, std::shared_ptr<const mfem::SparseMatrix>> parts;
    std::vector<double> unit(fem_.domain_quadrature_point_count(), 1.0);
    for (int region : fem_.mesh_data().region_attributes()) { parts[region] = domain_matrix(region, unit, false); }
    auto mass = sum_matrices(parts, field_.size());
    mfem::Vector full_rhs(field_.size()), rhs(p.Width());
    mass->Mult(original, full_rhs); p.MultTranspose(full_rhs, rhs);
    std::unique_ptr<mfem::SparseMatrix> system(mfem::RAP(p, *mass, p));
    mfem::GSSmoother preconditioner(*system);
    mfem::CGSolver solver; solver.SetOperator(*system); solver.SetPreconditioner(preconditioner);
    solver.SetRelTol(1e-12); solver.SetAbsTol(0.0); solver.SetMaxIter(1000); solver.SetPrintLevel(0);
    solver.Mult(rhs, guess);
    if (!solver.GetConverged()) { throw std::runtime_error("界面连续约束下的初温投影未收敛"); }
    recover(guess);
}
}
