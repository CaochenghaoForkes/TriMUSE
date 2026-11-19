#include "physics/mechanics/mechanics_assembly.hpp"
#include "physics/mechanics/mechanics_state.hpp"
#include "physics/common/assembly/assembly_utils.hpp"
#include "physics/mechanics/constraint/rigid_body_constraints.hpp"

namespace trimuse
{

MechanicsAssembly::MechanicsAssembly(FemSystem& fem, TrialField& trial_field, const MechanicsRegionConfigs& regions, const MechanicsBoundaryConfigs& boundaries, const MechanicsInterfaceConfigs& interfaces) // 初始化对象，登记连接生成界面连续映射 P，建立外边界单元索引表
    :
    fem_(fem),
    trial_field_(trial_field),
    integrator_(fem, trial_field),
    constraints_(fem, trial_field),
    stiffness_manager_(fem, regions.at("stiffness")),
    thermal_expansion_manager_(fem, regions.at("thermal_expansion")),
    source_manager_(fem, regions.at("source")),
    boundary_manager_(boundaries.at("mechanics_boundary")),
    interface_manager_(fem, constraints_, interfaces.at("interface_contact"))
{
    constraints_.finalize(); // 界面管理器已登记全部连接，统一生成 P （生成约束映射，界面连续约束的映射）
    for (int zone : boundary_manager_.zones()) // 为已配置边界建立空列表
    {
        boundary_elements_[zone] = {};
    }
    const auto& mesh = fem_.mesh_data().mfem_mesh();
    for (int boundary = 0; boundary < mesh.GetNBE(); ++boundary) // 遍历原网格中的边界单元，包含显式标记的内部材料界面 未显式配置模型的外边界 已配置模型的外边界
    {
        const auto zone = boundary_elements_.find(mesh.GetBdrAttribute(boundary)); // 找到对应边界区域编号
        if (zone == boundary_elements_.end()) // 当前边界分区未配置力学边界模型，跳过
        {
            continue;
        }
        int first, second;
        mesh.GetFaceElements(mesh.GetBdrElementFaceIndex(boundary), &first, &second); // 取得该面两侧的域单元编号
        if (second < 0) // 内部材料界面不作为外部载荷或位移边界
        {
            zone->second.push_back(boundary); // 将外表面单元编号加入对应外边界区域列表
        }
    }
}

void MechanicsAssembly::mark_coupled(PropertiesSystem& properties) const // 标记位移场依赖各个管理器依赖的属性
{
    stiffness_manager_.mark_coupled(properties, "displacement");
    thermal_expansion_manager_.mark_coupled(properties, "displacement");
    source_manager_.mark_coupled(properties, "displacement");
    boundary_manager_.mark_coupled(properties, "displacement");
}

bool MechanicsAssembly::coupled(const PropertiesSystem& properties) const // 判断: 是否有任何一个模型的依赖属性与指定求解场耦合
{
    return stiffness_manager_.coupled(properties, "displacement")
        || thermal_expansion_manager_.coupled(properties, "displacement")
        || source_manager_.coupled(properties, "displacement")
        || boundary_manager_.coupled(properties, "displacement");
}

void MechanicsAssembly::refresh_stiffness() // 只重组需要更新的区域刚度矩阵，如果有更新，再把各区域贡献重新相加
{
    bool changed = !stiffness_cache_.sum; // 判断当前有没有总刚度矩阵 指针为空：changed = true，本次需要首次建立总矩阵
    for (int region : stiffness_manager_.regions()) // 遍历刚度管理器中配置了模型的各个区域
    {
        if (stiffness_cache_.parts.count(region) && stiffness_manager_.required_no_change(region)) // 若该区域已经有组装缓存，且该区域的系数的 dirty == false
        {
            continue; // 则跳过
        }
        stiffness_cache_.parts[region] = integrator_.domain_matrix(region, stiffness_manager_.values()); // 传入全域积分点系数数值，但只遍历并积分当前 region 的单元
        stiffness_manager_.assembled(region); // 通知管理器：当前区域已经按最新系数组装完成，将该区域的 dirty 清为 false
        ++counts_["stiffness"]; // 刚度组装次数加一，每重新组装一个区域计一次
        changed = true; // 记录本次至少有一个区域的刚度缓存发生了更新，需要重新计算总矩阵
    }
    if (changed) // 如果有任何一个区域的刚度矩阵发生了变化，或还没有组装过总刚度矩阵
    {
        stiffness_cache_.sum = sum_matrices(stiffness_cache_.parts, trial_field_.size()); // 则组装总刚度矩阵
    }
}

void MechanicsAssembly::refresh_thermal_expansion()
{
    bool changed = !thermal_expansion_cache_.sum;
    for (int region : thermal_expansion_manager_.regions())
    {
        if (thermal_expansion_cache_.parts.count(region) && thermal_expansion_manager_.required_no_change(region))
        {
            continue;
        }
        thermal_expansion_cache_.parts[region] = integrator_.eigenstrain_load(region, thermal_expansion_manager_.values());
        thermal_expansion_manager_.assembled(region);
        ++counts_["thermal_expansion"];
        changed = true;
    }
    if (changed)
    {
        thermal_expansion_cache_.sum = sum_vectors(thermal_expansion_cache_.parts, trial_field_.size());
    }
}

void MechanicsAssembly::refresh_source()
{
    bool changed = !source_cache_.sum;
    for (int region : source_manager_.regions())
    {
        if (source_cache_.parts.count(region) && source_manager_.required_no_change(region))
        {
            continue;
        }
        source_cache_.parts[region] = integrator_.domain_load(region, source_manager_.values());
        source_manager_.assembled(region);
        ++counts_["source"];
        changed = true;
    }
    if (changed)
    {
        source_cache_.sum = sum_vectors(source_cache_.parts, trial_field_.size());
    }
}

void MechanicsAssembly::refresh_boundary()
{
    bool changed = !boundary_load_cache_.sum;
    for (int zone : boundary_manager_.zones())
    {
        if (boundary_manager_.essential(zone))
        {
            continue;
        } // 指定位移在形成系统时处理，不进行表面积分
        if (boundary_load_cache_.parts.count(zone) && boundary_manager_.required_no_change(zone))
        {
            continue;
        }
        boundary_load_cache_.parts[zone] = integrator_.boundary_load(boundary_elements_.at(zone), boundary_manager_.values(zone));
        boundary_manager_.assembled(zone);
        ++counts_["boundary_load"];
        changed = true;
    }
    if (changed)
    {
        boundary_load_cache_.sum = sum_vectors(boundary_load_cache_.parts, trial_field_.size());
    }
}

MechanicsAssembly::State MechanicsAssembly::update(const PropertiesFieldData& properties) // 更新各类系数，按变化情况刷新缓存，最后返回完整自由度上的刚度矩阵与总载荷：
{
    stiffness_manager_.update(properties); // 读取各求积点的弹性矩阵 C，更新管理器中的系数数组，并检查更新 dirty
    thermal_expansion_manager_.update(properties); // 读取当前正式温度场，以及属性中的弹性矩阵、热膨胀系数和参考温度，计算热膨胀系数，并检查更新 dirty
    source_manager_.update(properties); // 更新各求积点点体积力向量，并检查更新 dirty
    boundary_manager_.update(properties); // 更新各边界分区的指定面力向量、压力标量或指定位移向量，并检查更新 dirty
    refresh_stiffness(); // 根据 dirty 只重新组装发生变化或尚无缓存的区域刚度矩阵，必要时重新计算总刚度矩阵 并更换 cache_.sum 指针为新对象
    refresh_thermal_expansion(); // 根据 dirty 只重新组装发生变化或尚无缓存的区域热膨胀载荷，必要时重新计算总热膨胀载荷 并更换 cache_.sum 指针为新对象
    refresh_source(); // 根据 dirty 只重新组装发生变化或尚无缓存的区域体积力载荷，必要时重新计算总体积力载荷 并更换 cache_.sum 指针为新对象
    refresh_boundary(); // 根据 dirty 只重新组装发生变化或尚无缓存的表面边界表面力载荷，必要时重新计算表面边界表面力载荷 并更换 cache_.sum 指针为新对象

    state_.matrix = stiffness_cache_.sum; // 让 state_.matrix 与 stiffness_cache_.sum 共同持有同一张矩阵，不复制矩阵数值
    if (last_thermal_expansion_cache_sum_ != thermal_expansion_cache_.sum // 若总热膨胀载荷指针发生更换
        || last_source_cache_sum_ != source_cache_.sum // 或总体积力载荷指针发生更换
        || last_boundary_load_cache_sum_ != boundary_load_cache_.sum) // 或总表面力载荷指针发生更换
    {
        auto load = std::make_shared<mfem::Vector>(*thermal_expansion_cache_.sum); // 重新创建新总载荷向量 f = fth + fbody + fboundary
        *load += *source_cache_.sum;
        *load += *boundary_load_cache_.sum;
        state_.load = std::move(load); // 把这个新向量的共享指针移交给 state_.load
        last_thermal_expansion_cache_sum_ = thermal_expansion_cache_.sum; // 记录本次使用的新缓存指针
        last_source_cache_sum_ = source_cache_.sum; // 记录本次使用的新缓存指针
        last_boundary_load_cache_sum_ = boundary_load_cache_.sum; // 记录本次使用的新缓存指针
    }
    return state_; // 更新时替换指针，调用方保留的旧状态不会被改写
}

void MechanicsAssembly::essential_values(std::map<int, double>& values) const // 找到指定位移边界上的各个位移自由度，将它们转换为独立自由度编号，再填入指定值表
{
    const auto& mesh = fem_.mesh_data().mfem_mesh(); // 取得网格
    const auto& prolongation = constraints_.prolongation(); // 取得约束矩阵 P
    const int dimension = mesh.Dimension(); // 取得维数
    mfem::Array<int> dofs; // 接收当前边界面的全部分量自由度编号
    for (int zone : boundary_manager_.zones()) // 遍历已经配置的边界分区
    {
        if (!boundary_manager_.essential(zone)) // 跳过指定面力和压力边界，只保留指定位移边界
        {
            continue;
        }
        const auto& displacement = std::get<VectorValue>(boundary_manager_.values(zone)); // 取得该分区的指定位移向量
        for (int boundary : boundary_elements_.at(zone)) // 找到该分区包含的原网格外边界单元
        {
            int first, second;
            const int face = mesh.GetBdrElementFaceIndex(boundary);
            mesh.GetFaceElements(face, &first, &second);
            trial_field_.face_dofs(mesh.GetAttribute(first), face, dofs);
            const int count = dofs.Size() / dimension; // 每个分量的自由度数（插值节点数） 面上自由度按 [分量 0 的全部节点，分量 1 的全部节点，...] 排列
            for (int component = 0; component < dimension; ++component) // 遍历维度分量数
            for (int node = 0; node < count; ++node) // 遍历插值节点
            {
                const int full_dof = dofs[component * count + node]; // 按分量块找到对应的完整自由度编号
                const int reduced_dof = prolongation.GetJ()[prolongation.GetI()[full_dof]]; // 得到对应的独立自由度编号
                values[reduced_dof] = displacement[component]; // 保存的是被指定位移约束的独立自由度及其指定值
            }
        }
    }
}

void MechanicsAssembly::form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& load, mfem::SparseMatrix& system, mfem::Vector& system_load, mfem::Vector& solution) const
{
    std::map<int, double> essential; // 保存的是被指定位移约束的独立自由度及其指定值
    essential_values(essential);
    add_rigid_body_constraints(fem_, trial_field_, constraints_, essential); // 读取已有位移约束，再补上对称条件和必要的定位约束，最终更新 essential 表
    constraints_.form_system(matrix, load, essential, system, system_load, solution); // 把完整自由度上的方程，转换成独立自由度系统，并施加 Dirichlet 场值约束
}

void MechanicsAssembly::recover(const mfem::Vector& solution) // 将独立自由度位移恢复完整自由度位移
{
    trial_field_.set_values(constraints_.recover(solution));
}

void MechanicsAssembly::update_state(MechanicsState& mechanics_state) const // 更新各求积点的试探应变、应力
{
    mechanics_state.update(integrator_, stiffness_manager_.values(), thermal_expansion_manager_.values());
}

const MechanicsAssembly::Counts& MechanicsAssembly::counts() const // 返回组装次数信息表
{
    return counts_;
}

}
