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
    struct State // 当前汇总后的组装状态 
    {
        std::shared_ptr<const mfem::SparseMatrix> mass, conduction; // M 以及 K=Kdomain+KR+KI
        std::shared_ptr<const mfem::Vector> load; // F=FQ+Fgamma
    };
    using Counts = std::map<std::string, int>; // 组装次数统计 如 "conduction" -> 域内导热贡献组装次数

    ThermalAssembly(FemSystem& fem, ThermalField& field, const std::vector<ThermalDomainConfig>& sources, const std::vector<ThermalBoundaryConfig>& boundaries, const std::vector<ThermalInterfaceConfig>& interfaces);
    void mark_coupled(PropertiesSystem& properties, bool transient) const;
    bool coupled(const PropertiesSystem& properties, bool transient) const;
    State update(const PropertiesFieldData& properties, bool capacity);
    void update_boundary(const PropertiesFieldData& properties);
    void apply_initial_constraints();
    void project_initial();
    void form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs, mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& temperature) const;
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

    FemSystem& fem_; // 有限元系统
    ThermalField& field_; // 试探场
    ThermalConductionManager conduction_; // 域内导热模型
    ThermalCapacityManager capacity_; // 热容模型
    ThermalSourceManager source_; // 体积热源模型
    ThermalBoundaryManager boundary_; // 外边界指定温度、矩阵系数及右端系数
    ThermalInterfaceManager interface_; // 理想接触约束 P 和热阻界面矩阵 KI
    std::map<int, std::vector<int>> boundary_elements_; // 外边界单元索引表: 边界物理属性编号 → 该分区包含的原网格边界单元编号列表
    Cache<mfem::SparseMatrix> mass_cache_, conduction_cache_, boundary_matrix_cache_; // 组装结果的缓存 域内热容矩阵 M 域内导热矩阵 Kdomian 外边界 Robin 矩阵 KR
    Cache<mfem::Vector> source_cache_, boundary_load_cache_; // 右端项的缓存 体积热源向量 FQ Neumann 和 Robin 外边界右端向量 Fgamma
    State state_; // 当前汇总后的组装状态 
    // std::shared_ptr<T> 是 C++ 的共享所有权智能指针：多个指针可以共同拥有同一个对象，最后一个拥有者释放它时，对象会自动销毁
    std::shared_ptr<const mfem::SparseMatrix> last_domain_, last_boundary_; // 上次汇总时使用的缓存指针 用于比较指针是否被替换 last_domain_ -> conduction_cache_.sum last_boundary_ -> boundary_matrix_cache_.sum
    std::shared_ptr<const mfem::Vector> last_source_, last_boundary_load_; // 记录上次形成总右端向量时使用的 用于比较指针是否被替换 last_source_ -> source_cache_.sum last_boundary_load_ -> boundary_load_cache_.sum
    Counts counts_; // 组装次数统计
};
}
