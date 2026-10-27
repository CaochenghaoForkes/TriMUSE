#pragma once

#include "physics/thermal/conduction/thermal_conduction_manager.hpp"
#include "physics/thermal/capacity/thermal_capacity_manager.hpp"
#include "physics/thermal/source/thermal_source_manager.hpp"
#include "physics/thermal/boundary/thermal_boundary_manager.hpp"
#include "physics/thermal/interface/thermal_interface_manager.hpp"

namespace trimuse
{
class ThermalAssembly
{
public:
    struct State
    {
        std::shared_ptr<const mfem::SparseMatrix> mass, conduction;
        std::shared_ptr<const mfem::Vector> load;
    };
    using Counts = std::map<std::string, int>;

    ThermalAssembly(FemSystem& fem, ThermalField& field,
                    const std::vector<ThermalDomainConfig>& sources,
                    const std::vector<ThermalBoundaryConfig>& boundaries,
                    const std::vector<ThermalInterfaceConfig>& interfaces);
    void mark_coupled(PropertiesSystem& properties, bool transient) const;
    bool coupled(const PropertiesSystem& properties, bool transient) const;
    State update(const PropertiesFieldData& properties, bool capacity);
    void update_boundary(const PropertiesFieldData& properties);
    void apply_initial_constraints();
    void project_initial();
    void form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs,
                     mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& temperature) const;
    void recover(const mfem::Vector& temperature);
    const Counts& counts() const { return counts_; }

private:
    template<class Value> struct Cache
    {
        std::map<int, std::shared_ptr<const Value>> parts;
        std::shared_ptr<const Value> sum;
    };
    std::shared_ptr<mfem::SparseMatrix> domain_matrix(int region, const std::vector<double>& values, bool diffusion);
    std::shared_ptr<mfem::Vector> domain_load(int region, const std::vector<double>& values);
    std::shared_ptr<mfem::SparseMatrix> boundary_matrix(int zone);
    std::shared_ptr<mfem::Vector> boundary_load(int zone);
    void refresh_matrix(ThermalDomainManager& manager, Cache<mfem::SparseMatrix>& cache, bool diffusion, const std::string& counter);
    void refresh_source();
    void refresh_boundary();
    void essential_values(std::map<int, double>& values) const;
    mfem::Vector reduced_values() const;

    FemSystem& fem_;
    ThermalField& field_;
    ThermalConductionManager conduction_;
    ThermalCapacityManager capacity_;
    ThermalSourceManager source_;
    ThermalBoundaryManager boundary_;
    ThermalInterfaceManager interface_;
    std::map<int, std::vector<int>> boundary_elements_;
    Cache<mfem::SparseMatrix> mass_cache_, conduction_cache_, boundary_matrix_cache_;
    Cache<mfem::Vector> source_cache_, boundary_load_cache_;
    State state_;
    std::shared_ptr<const mfem::SparseMatrix> last_domain_, last_boundary_;
    std::shared_ptr<const mfem::Vector> last_source_, last_boundary_load_;
    Counts counts_;
};
}
