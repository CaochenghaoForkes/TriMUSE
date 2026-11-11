#pragma once

#include "physics/common/assembly/assembly_cache.hpp"
#include "physics/common/constraint/dof_constraints.hpp"
#include "physics/mechanics/assembly/mechanics_integrator.hpp"
#include "physics/mechanics/stiffness/mechanics_stiffness_manager.hpp"
#include "physics/mechanics/thermal_expansion/mechanics_thermal_expansion_manager.hpp"
#include "physics/mechanics/body_force/mechanics_body_force_manager.hpp"
#include "physics/mechanics/boundary/mechanics_boundary_manager.hpp"
#include "physics/mechanics/interface/mechanics_interface_manager.hpp"

namespace trimuse
{

class MechanicsState;

class MechanicsAssembly // 汇集完整自由度上的刚度与载荷，统一处理位移约束和求解结果恢复
{
public:
    struct State // K d = f，保存约化和 Dirichlet 消元前的只读组装结果
    {
        std::shared_ptr<const mfem::SparseMatrix> stiffness;
        std::shared_ptr<const mfem::Vector> load; // 热膨胀载荷 + 体积力载荷 + 表面载荷
    };

    using Counts = std::map<std::string, int>;

    MechanicsAssembly(FemSystem& fem, TrialField& trial_field,
        const std::vector<MechanicsDomainConfig>& body_forces = {},
        const std::vector<MechanicsBoundaryConfig>& boundaries = {},
        const std::vector<MechanicsInterfaceConfig>& interfaces = {});

    void mark_coupled(PropertiesSystem& properties) const; // 标记位移场依赖各个管理器依赖的属性

    bool coupled(const PropertiesSystem& properties) const; // 判断: 是否有任何一个模型的依赖属性与指定求解场耦合

    State update(const PropertiesFieldData& properties); // 更新各类系数，按变化情况刷新缓存，最后返回完整自由度上的刚度矩阵与总载荷：

    void form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs, mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& reduced_displacement) const; // 收集指定位移与刚体约束，再约化方程并进行 Dirichlet 消元

    void recover(const mfem::Vector& reduced_displacement); // 完整位移 = P * 独立位移，将结果写入试探场

    void update_state(MechanicsState& mechanics_state) const; // 使用最近一次 update 的 C、beta 及当前试探位移恢复应变和应力，不重新组装

    const Counts& counts() const; // 返回组装次数信息表

private:
    void refresh_stiffness();

    void refresh_thermal_expansion();

    void refresh_body_force();

    void refresh_boundary();
    
    void essential_values(std::map<int, double>& values) const; // 边界各分量的指定位移 -> 独立自由度编号与指定值

    FemSystem& fem_; // 公共有限元空间
    TrialField& trial_field_; // 试探位移场
    MechanicsIntegrator integrator_; // 力学积分器
    MechanicsStiffnessManager stiffness_manager_; // 刚度矩阵模型管理器
    MechanicsThermalExpansionManager thermal_expansion_manager_; // 热膨胀右端载荷管理器
    MechanicsBodyForceManager body_force_manager_; // 体积力向量管理器
    MechanicsBoundaryManager boundary_manager_; // 边界管理器
    DofConstraints constraints_; // 持有 P，供界面连接、刚体约束、方程约化及恢复共用
    MechanicsInterfaceManager interface_manager_; // 界面模型管理器

    std::map<int, std::vector<int>> boundary_elements_; // 外边界物理标识 -> 原网格边界单元编号列表
    AssemblyCache<mfem::SparseMatrix> stiffness_cache_; // 刚度矩阵缓存
    AssemblyCache<mfem::Vector> thermal_expansion_cache_; //热膨胀载荷缓存
    AssemblyCache<mfem::Vector> body_force_cache_; // 体积力载荷缓存
    AssemblyCache<mfem::Vector> boundary_load_cache_; // 表面载荷缓存

    State state_; // 当前完整自由度上的组装结果，包含刚度矩阵 stiffness 和总载荷 load，尚未约化或施加 Dirichlet 消元
    std::shared_ptr<const mfem::Vector> last_thermal_expansion_cache_sum_, last_body_force_cache_sum_, last_boundary_load_cache_sum_; // 只有缓存指针变化时才重新组合总载荷
    Counts counts_; // 组装次数统计，贡献名称 -> 累计组装次数，按每个区域或边界分区的一次组装计数
};

}
