#pragma once

#include <unordered_map>
#include "physics/common/assembly/assembly_cache.hpp"
#include "physics/common/assembly/scalar_integrator.hpp"
#include "physics/common/constraint/dof_constraints.hpp"

#include "physics/thermal/conduction/thermal_conduction_manager.hpp"
#include "physics/thermal/capacity/thermal_capacity_manager.hpp"
#include "physics/thermal/source/thermal_source_manager.hpp"
#include "physics/thermal/boundary/thermal_boundary_manager.hpp"
#include "physics/thermal/interface_heat_transfer/thermal_interface_heat_transfer_manager.hpp"

namespace trimuse
{

using ThermalRegionConfigs = std::unordered_map<std::string, std::vector<ThermalDomainConfig>>;
using ThermalBoundaryConfigs = std::unordered_map<std::string, std::vector<ThermalBoundaryConfig>>;
using ThermalInterfaceConfigs = std::unordered_map<std::string, std::vector<ThermalInterfaceConfig>>;

class ThermalAssembly
{
public:
    struct State // 完整自由度上的方程贡献，尚未进行时间离散和约束处理
    {
        std::shared_ptr<const mfem::SparseMatrix> mass, matrix; // 时间导数项矩阵 M，以及域内、边界和界面合并的总矩阵 K
        std::shared_ptr<const mfem::Vector> load; // F=FQ+Fgamma
    };

    using Counts = std::map<std::string, int>; // 组装次数统计 如 "conduction" -> 域内导热贡献组装次数

    ThermalAssembly(FemSystem& fem, TrialField& trial_field, const ThermalRegionConfigs& regions, const ThermalBoundaryConfigs& boundaries, const ThermalInterfaceConfigs& interfaces); // 初始化各物理贡献管理器，并建立已配置外边界的单元索引表
    void mark_coupled(PropertiesSystem& properties, bool transient) const; // 登记热方程实际用到的属性与温度场之间的耦合关系
    bool coupled(const PropertiesSystem& properties, bool transient) const; // 判断: 当前热方程中，是否存在已登记的温度耦合关系，用于决定是否需要非线性迭代
    State update(const PropertiesFieldData& properties, bool capacity); // 热方程组装层的总入口: 更新系数，按需刷新各部分缓存，再返回当前的 M、K、F
    void update_boundary(const PropertiesFieldData& properties); // 更新边界值: 单独提供此接口出要适用于三个场景 1.初始时刻(只需要施加初始时刻的表面温度，不需要执行时间推进、组装整套矩阵) 2.新时间步开始(如果 theta 法先用旧属性计算旧时刻贡献，边界管理器也会被更新成旧值 随后需要单独恢复新时刻的边界值，并先施加新表面温度) 3.显式格式theta=0(域内和自然边界贡献使用旧时刻数据，但指定温度仍施加新时刻的值 因此需要更新边界，却不需要组装新时刻的导热矩阵和热源向量)
    void apply_initial_constraints(); // 在正式求解前，让试探温度满足自由度连续约束和当前 Dirichlet 指定温度
    void project_initial(const PropertiesFieldData::IntegrationPointValues& weights); // 把当前初温投影到满足界面连续约束的空间中
    void form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& load, mfem::SparseMatrix& system, mfem::Vector& system_load, mfem::Vector& solution) const;
    void recover(const mfem::Vector& solution); // 传入独立自由度温度向量，把独立自由度温度恢复成完整温度向量，并写入试探场
    const Counts& counts() const { return counts_; } // 返回 组装次数统计

private:
    void refresh_conduction(const PropertiesFieldData::IntegrationPointValues& weights); // 按区域变化标记刷新conduction矩阵
    void refresh_capacity(const PropertiesFieldData::IntegrationPointValues& weights); // 按区域变化标记刷新capacity矩阵
    void refresh_source(const PropertiesFieldData::IntegrationPointValues& weights); // 按需组装体积热源向量缓存
    void refresh_boundary(); // 分别更新外边界矩阵和右端向量的缓存 两部分独立判断，因此可以只重组发生变化的部分
    void refresh_interface_heat_transfer(); // 按材料界面的 dirty 标记更新界面传热矩阵缓存
    void essential_values(std::map<int, double>& values) const; // 收集 Dirichlet 边界的指定温度，并把边界自由度编号转换为约束后独立空间中的编号

    FemSystem& fem_; // 有限元系统
    TrialField& trial_field_; // 试探场
    ScalarIntegrator integrator_; // 公共标量单元和外边界积分器
    ThermalConductionManager conduction_manager_; // 域内导热模型
    ThermalCapacityManager capacity_manager_; // 热容模型
    ThermalSourceManager source_manager_; // 体积热源模型
    ThermalBoundaryManager boundary_manager_; // 外边界指定温度、矩阵系数及右端系数
    DofConstraints constraints_; // 持有约束映射 P，统一完成自由度约化与恢复；界面组装只登记相等关系
    ThermalInterfaceManager interface_heat_transfer_manager_; // 界面约束及各界面的面片热阻
    std::map<int, std::vector<int>> boundary_elements_; // 外边界单元索引表: 边界物理属性编号 → 该分区包含的原网格边界单元编号列表
    AssemblyCache<mfem::SparseMatrix> capacity_matrix_cache_, conduction_matrix_cache_, boundary_matrix_cache_; // 组装结果的缓存 域内热容矩阵 M 域内导热矩阵 Kdomian 外边界 Robin 矩阵 KR
    AssemblyCache<mfem::SparseMatrix> interface_heat_transfer_matrix_cache_; // 界面传热矩阵 KI
    AssemblyCache<mfem::Vector> source_load_cache_, boundary_load_cache_; // 右端项的缓存 体积热源向量 FQ Neumann 和 Robin 外边界右端向量 Fgamma
    State state_; // 当前汇总后的组装状态

    // std::shared_ptr<T> 是 C++ 的共享所有权智能指针：多个指针可以共同拥有同一个对象，最后一个拥有者释放它时，对象会自动销毁
    std::shared_ptr<const mfem::SparseMatrix> last_conduction_matrix_cache_sum_, last_boundary_matrix_cache_sum_, last_interface_heat_transfer_matrix_cache_sum_; // 上次汇总时使用的矩阵缓存指针
    std::shared_ptr<const mfem::Vector> last_source_load_cache_sum_, last_boundary_load_cache_sum_; // 记录上次形成总右端向量时使用的 用于比较指针是否被替换 last_source_load_cache_sum_ -> source_load_cache_.sum last_boundary_load_cache_sum_ -> boundary_load_cache_.sum
    Counts counts_; // 组装次数统计 某矩阵/右端名称 -> 该项组装次数
};
}
