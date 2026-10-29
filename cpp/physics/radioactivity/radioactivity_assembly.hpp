#pragma once

#include "physics/common/assembly/assembly_cache.hpp"
#include "physics/common/assembly/scalar_integrator.hpp"

#include "physics/radioactivity/diffusion/radioactivity_diffusion_manager.hpp"
#include "physics/radioactivity/capacity/radioactivity_capacity_manager.hpp"
#include "physics/radioactivity/source/radioactivity_source_manager.hpp"
#include "physics/radioactivity/boundary/radioactivity_boundary_manager.hpp"
#include "physics/radioactivity/interface/radioactivity_interface_manager.hpp"

namespace trimuse
{
class RadioactivityAssembly
{
public:
    struct State
    {
        std::shared_ptr<const mfem::SparseMatrix> mass, diffusion; // M 与 K=Kdiffusion+KRobin+lambda M
        std::shared_ptr<const mfem::Vector> load;
    };
    using Counts = std::map<std::string, int>;

    RadioactivityAssembly(FemSystem& fem, TrialField& field, double decay_constant, const std::vector<RadioactivityDomainConfig>& sources, const std::vector<RadioactivityBoundaryConfig>& boundaries, const std::vector<RadioactivityInterfaceConfig>& interfaces);
    void mark_coupled(PropertiesSystem& properties) const;
    bool coupled(const PropertiesSystem& properties) const;
    State update(const PropertiesFieldData& properties);
    void update_boundary(const PropertiesFieldData& properties);
    void apply_initial_constraints();
    void project_initial();
    void form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs, mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& concentration) const;
    void recover(const mfem::Vector& concentration);
    const Counts& counts() const { return counts_; }

private:
    void refresh_matrix(RadioactivityDomainManager& manager, AssemblyCache<mfem::SparseMatrix>& cache, bool diffusion, const std::string& counter);
    void refresh_source();
    void refresh_boundary();
    void essential_values(std::map<int, double>& values) const;

    double decay_constant_; // s^-1；衰变矩阵为 lambda M，不重复积分。
    FemSystem& fem_;
    TrialField& field_;
    ScalarIntegrator integrator_;
    RadioactivityDiffusionManager diffusion_;
    RadioactivityCapacityManager capacity_;
    RadioactivitySourceManager source_;
    RadioactivityBoundaryManager boundary_;
    RadioactivityInterfaceManager interface_;
    std::map<int, std::vector<int>> boundary_elements_;
    std::shared_ptr<const mfem::SparseMatrix> mass_; // 固定网格、空间下只组装一次的单位容量矩阵
    AssemblyCache<mfem::SparseMatrix> diffusion_cache_, boundary_matrix_cache_;
    AssemblyCache<mfem::Vector> source_cache_, boundary_load_cache_;
    State state_;

    std::shared_ptr<const mfem::SparseMatrix> last_domain_, last_boundary_;
    std::shared_ptr<const mfem::Vector> last_source_, last_boundary_load_;
    Counts counts_;
};
}
