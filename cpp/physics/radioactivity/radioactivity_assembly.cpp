#include "physics/radioactivity/radioactivity_assembly.hpp"
#include "physics/common/assembly/assembly_utils.hpp"
#include <algorithm>
#include <stdexcept>

namespace trimuse
{
RadioactivityAssembly::RadioactivityAssembly(FemSystem& fem, TrialField& trial_field, const RadioactivityRegionConfigs& regions, const RadioactivityBoundaryConfigs& boundaries, const RadioactivityInterfaceConfigs& interfaces)
    :
    fem_(fem),
    trial_field_(trial_field),
    integrator_(fem, trial_field),
    diffusion_manager_(fem, regions.at("diffusion")),
    capacity_manager_(fem, regions.at("capacity")),
    decay_manager_(fem, regions.at("decay")),
    source_manager_(fem, regions.at("source")),
    boundary_manager_(boundaries.at("nuclide_boundary")),
    interface_nuclide_transfer_manager_(fem, trial_field, interfaces.at("interface_nuclide_transfer"))
{
    const auto& mesh = fem.mesh_data().mfem_mesh(); // 原网格用于筛选外边界单元
    for (int zone : boundary_manager_.zones()) { boundary_elements_[zone] = {}; }
    for (int zone : boundary_manager_.zones())
    for (int b = 0; b < mesh.GetNBE(); ++b)
    {
        int first, second;
        mesh.GetFaceElements(mesh.GetBdrElementFaceIndex(b), &first, &second); // second < 0 表示该面没有第二侧域单元
        if (second < 0 && mesh.GetBdrAttribute(b) == zone)
        {
            boundary_elements_[zone].push_back(b);
        }
    }

}

void RadioactivityAssembly::mark_coupled(PropertiesSystem& properties) const // 登记扩散、源和边界模型的浓度依赖
{
    diffusion_manager_.mark_coupled(properties, "concentration");
    decay_manager_.mark_coupled(properties, "concentration");
    source_manager_.mark_coupled(properties, "concentration");
    boundary_manager_.mark_coupled(properties, "concentration");
}

bool RadioactivityAssembly::coupled(const PropertiesSystem& properties) const // 任一方程系数依赖当前浓度时需要 Picard 迭代
{
    return diffusion_manager_.coupled(properties, "concentration") || decay_manager_.coupled(properties, "concentration") || source_manager_.coupled(properties, "concentration")
        || boundary_manager_.coupled(properties, "concentration");
}

void RadioactivityAssembly::refresh_diffusion() // 按区域变化标记刷新diffusion矩阵
{
    bool changed = !diffusion_matrix_cache_.sum;
    for (int region : diffusion_manager_.regions())
    {
        if (diffusion_matrix_cache_.parts.count(region) && diffusion_manager_.required_no_change(region))
        {
            continue;
        }
        diffusion_matrix_cache_.parts[region] = integrator_.domain_matrix(region, diffusion_manager_.values(), ScalarIntegrator::MatrixForm::Gradient);
        diffusion_manager_.assembled(region);
        ++counts_["diffusion"];
        changed = true;
    }
    if (changed)
    {
        diffusion_matrix_cache_.sum = sum_matrices(diffusion_matrix_cache_.parts, trial_field_.size());
    }
}

void RadioactivityAssembly::refresh_capacity() // 按区域变化标记刷新capacity矩阵
{
    bool changed = !capacity_matrix_cache_.sum;
    for (int region : capacity_manager_.regions())
    {
        if (capacity_matrix_cache_.parts.count(region) && capacity_manager_.required_no_change(region))
        {
            continue;
        }
        capacity_matrix_cache_.parts[region] = integrator_.domain_matrix(region, capacity_manager_.values(), ScalarIntegrator::MatrixForm::Mass);
        capacity_manager_.assembled(region);
        ++counts_["capacity"];
        changed = true;
    }
    if (changed)
    {
        capacity_matrix_cache_.sum = sum_matrices(capacity_matrix_cache_.parts, trial_field_.size());
    }
}

void RadioactivityAssembly::refresh_decay() // 按区域变化标记刷新衰变矩阵
{
    bool changed = !decay_matrix_cache_.sum;
    for (int region : decay_manager_.regions())
    {
        if (decay_matrix_cache_.parts.count(region) && decay_manager_.required_no_change(region))
        {
            continue;
        }
        decay_matrix_cache_.parts[region] = integrator_.domain_matrix(region, decay_manager_.values(), ScalarIntegrator::MatrixForm::Mass);
        decay_manager_.assembled(region);
        ++counts_["decay"];
        changed = true;
    }
    if (changed)
    {
        decay_matrix_cache_.sum = sum_matrices(decay_matrix_cache_.parts, trial_field_.size());
    }
}

void RadioactivityAssembly::refresh_source() // 按区域缓存体积核素产生率的右端贡献
{
    bool changed = !source_load_cache_.sum;
    for (int region : source_manager_.regions())
    {
        if (source_load_cache_.parts.count(region) && source_manager_.required_no_change(region))
        {
            continue;
        }
        source_load_cache_.parts[region] = integrator_.domain_load(region, source_manager_.values()); // F_birth,i=int_Omega_region q_birth N_i dOmega
        source_manager_.assembled(region);
        ++counts_["source"]; changed = true;
    }
    if (changed)
    {
        source_load_cache_.sum = sum_vectors(source_load_cache_.parts, trial_field_.size());
    }
}

void RadioactivityAssembly::refresh_boundary() // 自然边界的矩阵与右端独立判断是否需要重组
{
    bool matrix_changed = !boundary_matrix_cache_.sum, load_changed = !boundary_load_cache_.sum;
    for (int zone : boundary_manager_.zones())
    {
        if (boundary_manager_.essential(zone)) // Dirichlet 在形成求解系统时施加，不参与自然边界积分
        {
            continue;
        }
        if (!boundary_matrix_cache_.parts.count(zone) || !boundary_manager_.required_no_change(zone, RadioactivityBoundaryPart::Matrix))
        {
            boundary_matrix_cache_.parts[zone] = integrator_.boundary_matrix(boundary_elements_.at(zone), boundary_manager_.values(zone).matrix); // Robin 左端 h_m N_i N_j
            boundary_manager_.assembled(zone, RadioactivityBoundaryPart::Matrix);
            ++counts_["boundary_matrix"]; matrix_changed = true;
        }
        if (!boundary_load_cache_.parts.count(zone) || !boundary_manager_.required_no_change(zone, RadioactivityBoundaryPart::Load))
        {
            boundary_load_cache_.parts[zone] = integrator_.boundary_load(boundary_elements_.at(zone), boundary_manager_.values(zone).load); // Neumann -j_n 或 Robin h_m c_infinity
            boundary_manager_.assembled(zone, RadioactivityBoundaryPart::Load);
            ++counts_["boundary_load"]; load_changed = true;
        }
    }
    if (matrix_changed)
    {
        boundary_matrix_cache_.sum = sum_matrices(boundary_matrix_cache_.parts, trial_field_.size());
    }
    if (load_changed)
    {
        boundary_load_cache_.sum = sum_vectors(boundary_load_cache_.parts, trial_field_.size());
    }
}

RadioactivityAssembly::State RadioactivityAssembly::update(const PropertiesFieldData& properties) // 根据指定时刻属性按需更新并汇总 M、K、F
{
    diffusion_manager_.update(properties);
    capacity_manager_.update(properties);
    decay_manager_.update(properties);
    source_manager_.update(properties);
    boundary_manager_.update(properties);
    refresh_diffusion();
    refresh_capacity();
    refresh_decay();
    refresh_source();
    refresh_boundary();
    if (last_diffusion_matrix_cache_sum_ != diffusion_matrix_cache_.sum || last_decay_matrix_cache_sum_ != decay_matrix_cache_.sum || last_boundary_matrix_cache_sum_ != boundary_matrix_cache_.sum) // 用缓存指针是否替换判断总 K 是否需要更新
    {
        std::unique_ptr<mfem::SparseMatrix> diffusion(mfem::Add(1.0, *diffusion_matrix_cache_.sum, 1.0, *boundary_matrix_cache_.sum)); // K_temp=K_diffusion+K_Robin
        state_.matrix.reset(mfem::Add(1.0, *diffusion, 1.0, *decay_matrix_cache_.sum)); // K = K_diffusion + K_boundary + K_decay
        last_decay_matrix_cache_sum_ = decay_matrix_cache_.sum;
        last_diffusion_matrix_cache_sum_ = diffusion_matrix_cache_.sum; last_boundary_matrix_cache_sum_ = boundary_matrix_cache_.sum;
    }
    if (last_source_load_cache_sum_ != source_load_cache_.sum || last_boundary_load_cache_sum_ != boundary_load_cache_.sum) // 源或边界右端变化时才重新汇总 F
    {
        auto load = std::make_shared<mfem::Vector>(*source_load_cache_.sum);
        *load += *boundary_load_cache_.sum; // F=F_birth+F_boundary
        state_.load = std::move(load);
        last_source_load_cache_sum_ = source_load_cache_.sum; last_boundary_load_cache_sum_ = boundary_load_cache_.sum;
    }
    state_.mass = capacity_matrix_cache_.sum; // 容量项独立缓存，不参与衰变组装
    return state_; // 共享不可变结果，刷新缓存只替换指针，不改写旧时刻的矩阵。
}

// 初始时刻和 theta=0 只需更新指定边界值；不会触发新时刻的域内组装。
void RadioactivityAssembly::update_boundary(const PropertiesFieldData& properties)
{
    boundary_manager_.update(properties);
}

// 收集独立自由度编号 -> 指定浓度，外边界几何仍来自原网格。
void RadioactivityAssembly::essential_values(std::map<int, double>& values) const
{
    const auto& p = interface_nuclide_transfer_manager_.prolongation(); // P 的行是完整自由度，列是完美接触约束后的独立自由度
    for (int zone : boundary_manager_.zones())
    {
        if (!boundary_manager_.essential(zone))
        {
            continue;
        }
        for (int b : boundary_elements_.at(zone))
        {
            const auto& mesh = fem_.mesh_data().mfem_mesh();
            int first, second;
            const int face = mesh.GetBdrElementFaceIndex(b); // 边界单元编号 -> 原网格面编号
            mesh.GetFaceElements(face, &first, &second);
            mfem::Array<int> dofs; trial_field_.face_dofs(mesh.GetAttribute(first), face, dofs); // 取得外表面一侧的完整自由度编号
            for (int dof : dofs)
            for (int j = p.GetI()[dof]; j < p.GetI()[dof + 1]; ++j) // 遍历 P 中该完整自由度对应的非零条目
            {
                if (p.GetData()[j] != 0.0)
                {
                    values[p.GetJ()[j]] = boundary_manager_.values(zone).concentration; // 转为独立自由度编号并记录指定浓度
                }
            }
        }
    }
}

void RadioactivityAssembly::apply_initial_constraints() // 只调整试探浓度值，不修改待求解的矩阵与右端
{
    auto solution = interface_nuclide_transfer_manager_.constraints().reduced_values(); // 从完整试探场提取独立自由度浓度
    std::map<int, double> essential;
    essential_values(essential);
    for (const auto& [dof, value] : essential)
    {
        solution[dof] = value;
    }
    recover(solution); // 经 P 恢复完整试探场，同时满足完美接触和指定表面浓度
    for (int zone : boundary_manager_.zones())
    {
        boundary_manager_.assembled(zone, RadioactivityBoundaryPart::Essential);
    }
}

void RadioactivityAssembly::form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& load,
    mfem::SparseMatrix& system, mfem::Vector& system_load, mfem::Vector& solution) const
{
    std::map<int, double> essential;
    essential_values(essential);
    interface_nuclide_transfer_manager_.constraints().form_system(matrix, load, essential, system, system_load, solution); // P^T A P、P^T b，再施加 Dirichlet 消元
}

void RadioactivityAssembly::recover(const mfem::Vector& solution)
{
    const auto full = interface_nuclide_transfer_manager_.constraints().recover(solution);
    trial_field_.set_values(full); // 仅修改本次求解的试探场，正式场由 accept() 更新
}

void RadioactivityAssembly::project_initial()
{
    // 带几何权重的 L2 投影保持总盘存；逐自由度平均不具有这个性质。
    const auto& p = interface_nuclide_transfer_manager_.prolongation();
    auto solution = interface_nuclide_transfer_manager_.constraints().reduced_values(); // 独立自由度上的初始猜测
    mfem::Vector original = trial_field_.values(), projected(trial_field_.size());
    p.Mult(solution, projected); projected -= original; // 已满足连续约束时无须再次投影
    if (projected.Normlinf() <= 1e-12 * original.Normlinf()) { return; }
    // 投影使用全域单位质量矩阵，与容量、衰变是否启用无关
    std::map<int, std::shared_ptr<const mfem::SparseMatrix>> parts;
    std::vector<double> unit(fem_.domain_quadrature_point_count(), 1.0);
    for (int region : fem_.mesh_data().region_attributes())
    {
        parts[region] = integrator_.domain_matrix(region, unit, ScalarIntegrator::MatrixForm::Mass);
    }
    const auto mass = sum_matrices(parts, trial_field_.size());
    mfem::Vector load(trial_field_.size()), system_load(p.Width());
    mass->Mult(original, load); p.MultTranspose(load, system_load); // 投影右端 P^T M c_original
    std::unique_ptr<mfem::SparseMatrix> system(mfem::RAP(p, *mass, p)); // 投影矩阵 P^T M P
    mfem::GSSmoother preconditioner(*system);
    mfem::CGSolver solver; solver.SetOperator(*system); solver.SetPreconditioner(preconditioner);
    solver.SetRelTol(1e-12); solver.SetAbsTol(0.0); solver.SetMaxIter(1000); solver.SetPrintLevel(0);
    solver.Mult(system_load, solution);
    if (!solver.GetConverged()) { throw std::runtime_error("界面连续约束下的初始浓度投影未收敛"); }
    recover(solution);
}

}
