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
class RadioactivityAssembly // 汇总扩散、衰变、源与外边界贡献，并处理完美接触和 Dirichlet 约束
{
public:
    struct State // 一次 update() 返回的只读组装结果，可由旧、新时刻同时持有
    {
        std::shared_ptr<const mfem::SparseMatrix> mass, diffusion; // M 与 K=Kdiffusion+KRobin+lambda M
        std::shared_ptr<const mfem::Vector> load; // F=F_birth+F_boundary
    };
    using Counts = std::map<std::string, int>; // 贡献名称 -> 已组装区域或边界的次数

    RadioactivityAssembly(FemSystem& fem, TrialField& trial_field, double decay_constant, const std::vector<RadioactivityDomainConfig>& sources, const std::vector<RadioactivityBoundaryConfig>& boundaries, const std::vector<RadioactivityInterfaceConfig>& interfaces); // 配置贡献管理器、界面约束和外边界索引，预组装固定的 M
    void mark_coupled(PropertiesSystem& properties) const; // 登记本方程所需属性与浓度场的耦合关系
    bool coupled(const PropertiesSystem& properties) const; // 判断是否需要浓度 Picard 迭代
    State update(const PropertiesFieldData& properties); // 按需更新系数与缓存，返回 M、K、F
    void update_boundary(const PropertiesFieldData& properties); // 只更新当前时刻边界值，不组装整套方程
    void apply_initial_constraints(); // 将完美接触和 Dirichlet 条件施加到试探浓度值
    void project_initial(); // 将阶跃初值投影到满足完美接触的空间
    void form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs, mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& reduced_concentration) const; // 将完整方程约化到独立自由度并消去 Dirichlet 值
    void recover(const mfem::Vector& reduced_concentration); // 经 P 恢复完整浓度并写入试探场
    const Counts& counts() const { return counts_; }

private:
    void refresh_matrix(RadioactivityDomainManager& manager, AssemblyCache<mfem::SparseMatrix>& cache, bool diffusion, const std::string& counter); // 仅重组系数变化的区域，再汇总各区域矩阵
    void refresh_source(); // 按区域变化标记更新体积源向量
    void refresh_boundary(); // 分别更新自然边界的矩阵和右端贡献
    void essential_values(std::map<int, double>& values) const; // 收集独立自由度编号 -> 指定表面浓度

    double decay_constant_; // s^-1；衰变矩阵为 lambda M，不重复积分。
    FemSystem& fem_; // 原网格、区域单元表和求积规则
    TrialField& trial_field_; // 本次求解的试探浓度场
    ScalarIntegrator integrator_; // 公共标量域内及外边界积分器
    RadioactivityDiffusionManager diffusion_; // 积分点扩散系数及区域变化标记
    RadioactivityCapacityManager capacity_; // 恒为 1 的时间导数系数
    RadioactivitySourceManager source_; // 体积核素产生率
    RadioactivityBoundaryManager boundary_; // 外边界矩阵、载荷及指定浓度
    RadioactivityInterfaceManager interface_; // 完美接触约束与映射 P
    std::map<int, std::vector<int>> boundary_elements_; // 边界属性编号 -> 原网格外边界单元编号列表
    std::shared_ptr<const mfem::SparseMatrix> mass_; // 固定网格、空间下只组装一次的单位容量矩阵
    AssemblyCache<mfem::SparseMatrix> diffusion_cache_, boundary_matrix_cache_; // 按区域或分区缓存扩散矩阵与 Robin 矩阵
    AssemblyCache<mfem::Vector> source_cache_, boundary_load_cache_; // 按区域或分区缓存体积源和边界右端
    State state_; // 最近一次汇总的 M、K、F

    std::shared_ptr<const mfem::SparseMatrix> last_domain_, last_boundary_; // 上次组成 K 时使用的 diffusion_cache_.sum 与 boundary_matrix_cache_.sum
    std::shared_ptr<const mfem::Vector> last_source_, last_boundary_load_; // 上次组成 F 时使用的 source_cache_.sum 与 boundary_load_cache_.sum
    Counts counts_; // 各类贡献的累计组装次数
};
}
