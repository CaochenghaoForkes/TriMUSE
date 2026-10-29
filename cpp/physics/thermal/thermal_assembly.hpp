#pragma once

#include "physics/common/assembly/assembly_cache.hpp"
#include "physics/common/assembly/scalar_integrator.hpp"

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

    ThermalAssembly(FemSystem& fem, TrialField& field, const std::vector<ThermalDomainConfig>& sources, const std::vector<ThermalBoundaryConfig>& boundaries, const std::vector<ThermalInterfaceConfig>& interfaces); // 初始化各物理贡献管理器，并建立已配置外边界的单元索引表

    void mark_coupled(PropertiesSystem& properties, bool transient) const; // 登记热方程实际用到的属性与温度场之间的耦合关系

    bool coupled(const PropertiesSystem& properties, bool transient) const; // 判断: 当前热方程中，是否存在已登记的温度耦合关系，用于决定是否需要非线性迭代

    State update(const PropertiesFieldData& properties, bool capacity); // 热方程组装层的总入口: 更新系数，按需刷新各部分缓存，再返回当前的 M、K、F

    void update_boundary(const PropertiesFieldData& properties); // 更新边界值: 单独提供此接口出要适用于三个场景 1.初始时刻(只需要施加初始时刻的表面温度，不需要执行时间推进、组装整套矩阵) 2.新时间步开始(如果 theta 法先用旧属性计算旧时刻贡献，边界管理器也会被更新成旧值 随后需要单独恢复新时刻的边界值，并先施加新表面温度) 3.显式格式theta=0(域内和自然边界贡献使用旧时刻数据，但指定温度仍施加新时刻的值 因此需要更新边界，却不需要组装新时刻的导热矩阵和热源向量)

    void apply_initial_constraints(); // 在正式求解前，让试探温度满足自由度连续约束和当前 Dirichlet 指定温度

    void project_initial(); // 把当前初温投影到满足界面连续约束的空间中

    void form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs, mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& temperature) const;

    void recover(const mfem::Vector& temperature); // 传入独立自由度温度向量，把独立自由度温度恢复成完整温度向量，并写入试探场

    const Counts& counts() const { return counts_; }

private:
    void refresh_matrix(ThermalDomainManager& manager, AssemblyCache<mfem::SparseMatrix>& cache, bool diffusion, const std::string& counter); // 负责按需更新域内矩阵缓存：只重组发生变化的区域，再更新这些区域贡献的总和 导热矩阵和热容矩阵共用这套逻辑 cache: 对应缓存 cache.parts: 区域编号->该区域的矩阵贡献 cache.sum: 所有区域贡献之和

    void refresh_source(); // 按需组装体积热源向量缓存

    void refresh_boundary(); // 分别更新外边界矩阵和右端向量的缓存 两部分独立判断，因此可以只重组发生变化的部分

    void essential_values(std::map<int, double>& values) const; // 收集 Dirichlet 边界的指定温度，并把边界自由度编号转换为约束后独立空间中的编号

    FemSystem& fem_; // 有限元系统

    TrialField& field_; // 试探场

    ScalarIntegrator integrator_; // 公共标量单元和外边界积分器

    ThermalConductionManager conduction_; // 域内导热模型

    ThermalCapacityManager capacity_; // 热容模型

    ThermalSourceManager source_; // 体积热源模型

    ThermalBoundaryManager boundary_; // 外边界指定温度、矩阵系数及右端系数

    ThermalInterfaceManager interface_; // 理想接触约束 P 和热阻界面矩阵 KI

    std::map<int, std::vector<int>> boundary_elements_; // 外边界单元索引表: 边界物理属性编号 → 该分区包含的原网格边界单元编号列表

    AssemblyCache<mfem::SparseMatrix> mass_cache_, conduction_cache_, boundary_matrix_cache_; // 组装结果的缓存 域内热容矩阵 M 域内导热矩阵 Kdomian 外边界 Robin 矩阵 KR

    AssemblyCache<mfem::Vector> source_cache_, boundary_load_cache_; // 右端项的缓存 体积热源向量 FQ Neumann 和 Robin 外边界右端向量 Fgamma

    State state_; // 当前汇总后的组装状态 

    // std::shared_ptr<T> 是 C++ 的共享所有权智能指针：多个指针可以共同拥有同一个对象，最后一个拥有者释放它时，对象会自动销毁
    std::shared_ptr<const mfem::SparseMatrix> last_domain_, last_boundary_; // 上次汇总时使用的缓存指针 用于比较指针是否被替换 last_domain_ -> conduction_cache_.sum last_boundary_ -> boundary_matrix_cache_.sum

    std::shared_ptr<const mfem::Vector> last_source_, last_boundary_load_; // 记录上次形成总右端向量时使用的 用于比较指针是否被替换 last_source_ -> source_cache_.sum last_boundary_load_ -> boundary_load_cache_.sum

    Counts counts_; // 组装次数统计
};
}
