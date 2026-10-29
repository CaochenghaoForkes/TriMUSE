#include "physics/radioactivity/radioactivity_assembly.hpp"
#include "physics/common/assembly/assembly_utils.hpp"
#include <algorithm>
#include <stdexcept>

namespace trimuse
{
RadioactivityAssembly::RadioactivityAssembly(FemSystem& fem, TrialField& field, double decay_constant, const std::vector<RadioactivityDomainConfig>& sources, const std::vector<RadioactivityBoundaryConfig>& boundaries, const std::vector<RadioactivityInterfaceConfig>& interfaces)
    : decay_constant_(decay_constant), fem_(fem), field_(field), integrator_(fem, field), diffusion_(fem), capacity_(fem), source_(fem, sources),
      boundary_(boundaries), interface_(fem, field, interfaces)
{
    const auto& mesh = fem.mesh_data().mfem_mesh();
    for (int zone : boundary_.zones()) { boundary_elements_[zone] = {}; }
    for (int zone : boundary_.zones())
    for (int b = 0; b < mesh.GetNBE(); ++b)
    {
        int first, second;
        mesh.GetFaceElements(mesh.GetBdrElementFaceIndex(b), &first, &second);
        if (second < 0 && mesh.GetBdrAttribute(b) == zone)
        {
            boundary_elements_[zone].push_back(b);
        }
    }
    // 容量恒为 1，M 同时用于时间导数、衰变和初值投影。
    capacity_.update(PropertiesFieldData{});
    AssemblyCache<mfem::SparseMatrix> mass_cache;
    refresh_matrix(capacity_, mass_cache, false, "capacity");
    mass_ = mass_cache.sum;
}

void RadioactivityAssembly::mark_coupled(PropertiesSystem& properties) const
{
    diffusion_.mark_coupled(properties, "concentration");
    source_.mark_coupled(properties, "concentration");
    boundary_.mark_coupled(properties, "concentration");
}

bool RadioactivityAssembly::coupled(const PropertiesSystem& properties) const
{
    return diffusion_.coupled(properties, "concentration") || source_.coupled(properties, "concentration")
        || boundary_.coupled(properties, "concentration");
}

void RadioactivityAssembly::refresh_matrix(RadioactivityDomainManager& manager, AssemblyCache<mfem::SparseMatrix>& cache, bool diffusion, const std::string& counter)
{
    bool changed = !cache.sum;
    for (int region : manager.regions())
    {
        if (cache.parts.count(region) && manager.required_no_change(region))
        {
            continue;
        }
        cache.parts[region] = integrator_.domain_matrix(region, manager.values(), diffusion);
        manager.assembled(region);
        ++counts_[counter]; changed = true;
    }
    if (changed)
    {
        cache.sum = sum_matrices(cache.parts, field_.size());
    }
}

void RadioactivityAssembly::refresh_source()
{
    bool changed = !source_cache_.sum;
    for (int region : source_.regions())
    {
        if (source_cache_.parts.count(region) && source_.required_no_change(region))
        {
            continue;
        }
        source_cache_.parts[region] = integrator_.domain_load(region, source_.values());
        source_.assembled(region);
        ++counts_["source"]; changed = true;
    }
    if (changed)
    {
        source_cache_.sum = sum_vectors(source_cache_.parts, field_.size());
    }
}

void RadioactivityAssembly::refresh_boundary()
{
    bool matrix_changed = !boundary_matrix_cache_.sum, load_changed = !boundary_load_cache_.sum;
    for (int zone : boundary_.zones())
    {
        if (boundary_.essential(zone))
        {
            continue;
        }
        if (!boundary_matrix_cache_.parts.count(zone) || !boundary_.required_no_change(zone, RadioactivityBoundaryPart::Matrix))
        {
            boundary_matrix_cache_.parts[zone] = integrator_.boundary_matrix(boundary_elements_.at(zone), boundary_.values(zone).matrix);
            boundary_.assembled(zone, RadioactivityBoundaryPart::Matrix);
            ++counts_["boundary_matrix"]; matrix_changed = true;
        }
        if (!boundary_load_cache_.parts.count(zone) || !boundary_.required_no_change(zone, RadioactivityBoundaryPart::Load))
        {
            boundary_load_cache_.parts[zone] = integrator_.boundary_load(boundary_elements_.at(zone), boundary_.values(zone).load);
            boundary_.assembled(zone, RadioactivityBoundaryPart::Load);
            ++counts_["boundary_load"]; load_changed = true;
        }
    }
    if (matrix_changed)
    {
        boundary_matrix_cache_.sum = sum_matrices(boundary_matrix_cache_.parts, field_.size());
    }
    if (load_changed)
    {
        boundary_load_cache_.sum = sum_vectors(boundary_load_cache_.parts, field_.size());
    }
}

RadioactivityAssembly::State RadioactivityAssembly::update(const PropertiesFieldData& properties)
{
    diffusion_.update(properties);
    source_.update(properties);
    boundary_.update(properties);
    refresh_matrix(diffusion_, diffusion_cache_, true, "diffusion");
    refresh_source();
    refresh_boundary();
    if (last_domain_ != diffusion_cache_.sum || last_boundary_ != boundary_matrix_cache_.sum)
    {
        std::unique_ptr<mfem::SparseMatrix> diffusion(mfem::Add(1.0, *diffusion_cache_.sum, 1.0, *boundary_matrix_cache_.sum));
        // 单核素衰变是正的反应项，和扩散、Robin 左端一起组成 K。
        if (decay_constant_ != 0.0)
        {
            state_.diffusion.reset(mfem::Add(1.0, *diffusion, decay_constant_, *mass_));
        }
        else { state_.diffusion = std::move(diffusion); }
        last_domain_ = diffusion_cache_.sum; last_boundary_ = boundary_matrix_cache_.sum;
    }
    if (last_source_ != source_cache_.sum || last_boundary_load_ != boundary_load_cache_.sum)
    {
        auto load = std::make_shared<mfem::Vector>(*source_cache_.sum);
        *load += *boundary_load_cache_.sum;
        state_.load = std::move(load);
        last_source_ = source_cache_.sum; last_boundary_load_ = boundary_load_cache_.sum;
    }
    state_.mass = mass_;
    return state_; // 共享不可变结果，刷新缓存只替换指针，不改写旧时刻的矩阵。
}

// 初始时刻和 theta=0 只需更新指定边界值；不会触发新时刻的域内组装。
void RadioactivityAssembly::update_boundary(const PropertiesFieldData& properties)
{
    boundary_.update(properties);
}

// 收集独立自由度编号 -> 指定浓度，外边界几何仍来自原网格。
void RadioactivityAssembly::essential_values(std::map<int, double>& values) const
{
    const auto& p = interface_.prolongation();
    for (int zone : boundary_.zones())
    {
        if (!boundary_.essential(zone))
        {
            continue;
        }
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
                if (p.GetData()[j] != 0.0)
                {
                    values[p.GetJ()[j]] = boundary_.values(zone).concentration;
                }
            }
        }
    }
}

void RadioactivityAssembly::apply_initial_constraints()
{
    auto concentration = interface_.constraints().reduced_values();
    std::map<int, double> essential;
    essential_values(essential);
    for (const auto& [dof, value] : essential)
    {
        concentration[dof] = value;
    }
    recover(concentration);
    for (int zone : boundary_.zones())
    {
        boundary_.assembled(zone, RadioactivityBoundaryPart::Essential);
    }
}

void RadioactivityAssembly::form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs,
    mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& concentration) const
{
    std::map<int, double> essential;
    essential_values(essential);
    interface_.constraints().form_system(matrix, rhs, essential, system, load, concentration);
}

void RadioactivityAssembly::recover(const mfem::Vector& concentration)
{
    const auto full = interface_.constraints().recover(concentration);
    field_.set_values(full);
}

void RadioactivityAssembly::project_initial()
{
    // 带几何权重的 L2 投影保持总盘存；逐自由度平均不具有这个性质。
    const auto& p = interface_.prolongation();
    auto guess = interface_.constraints().reduced_values();
    mfem::Vector original = field_.values(), projected(field_.size());
    p.Mult(guess, projected); projected -= original;
    if (projected.Normlinf() <= 1e-12 * original.Normlinf()) { return; }
    mfem::Vector full_rhs(field_.size()), rhs(p.Width());
    mass_->Mult(original, full_rhs); p.MultTranspose(full_rhs, rhs);
    std::unique_ptr<mfem::SparseMatrix> system(mfem::RAP(p, *mass_, p));
    mfem::GSSmoother preconditioner(*system);
    mfem::CGSolver solver; solver.SetOperator(*system); solver.SetPreconditioner(preconditioner);
    solver.SetRelTol(1e-12); solver.SetAbsTol(0.0); solver.SetMaxIter(1000); solver.SetPrintLevel(0);
    solver.Mult(rhs, guess);
    if (!solver.GetConverged()) { throw std::runtime_error("界面连续约束下的初始浓度投影未收敛"); }
    recover(guess);
}

}
