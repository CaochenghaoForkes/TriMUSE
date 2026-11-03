#include "physics/radioactivity/radioactivity_assembly.hpp"
#include "physics/common/assembly/assembly_utils.hpp"
#include <algorithm>
#include <stdexcept>

namespace trimuse
{
RadioactivityAssembly::RadioactivityAssembly(FemSystem& fem, TrialField& trial_field, double decay_constant, const std::vector<RadioactivityDomainConfig>& sources, const std::vector<RadioactivityBoundaryConfig>& boundaries, const std::vector<RadioactivityInterfaceConfig>& interfaces)
    : decay_constant_(decay_constant), fem_(fem), trial_field_(trial_field), integrator_(fem, trial_field), diffusion_(fem), capacity_(fem), source_(fem, sources),
      boundary_(boundaries), interface_(fem, trial_field, interfaces)
{
    const auto& mesh = fem.mesh_data().mfem_mesh(); // 原网格用于筛选外边界单元
    for (int zone : boundary_.zones()) { boundary_elements_[zone] = {}; }
    for (int zone : boundary_.zones())
    for (int b = 0; b < mesh.GetNBE(); ++b)
    {
        int first, second;
        mesh.GetFaceElements(mesh.GetBdrElementFaceIndex(b), &first, &second); // second < 0 表示该面没有第二侧域单元
        if (second < 0 && mesh.GetBdrAttribute(b) == zone)
        {
            boundary_elements_[zone].push_back(b);
        }
    }
    // 容量恒为 1，M 同时用于时间导数、衰变和初值投影。
    capacity_.update(PropertiesFieldData{}); // 单位容量模型不读取属性，填充全域积分点系数 1
    AssemblyCache<mfem::SparseMatrix> mass_cache;
    refresh_matrix(capacity_, mass_cache, false, "capacity");
    mass_ = mass_cache.sum; // M 与温度、浓度和时间无关，后续直接复用
}

void RadioactivityAssembly::mark_coupled(PropertiesSystem& properties) const // 登记扩散、源和边界模型的浓度依赖
{
    diffusion_.mark_coupled(properties, "concentration");
    source_.mark_coupled(properties, "concentration");
    boundary_.mark_coupled(properties, "concentration");
}

bool RadioactivityAssembly::coupled(const PropertiesSystem& properties) const // 任一方程系数依赖当前浓度时需要 Picard 迭代
{
    return diffusion_.coupled(properties, "concentration") || source_.coupled(properties, "concentration")
        || boundary_.coupled(properties, "concentration");
}

void RadioactivityAssembly::refresh_matrix(RadioactivityDomainManager& manager, AssemblyCache<mfem::SparseMatrix>& cache, bool diffusion, const std::string& counter) // 仅重组 dirty 区域，然后对各区域贡献重新求和
{
    bool changed = !cache.sum; // 首次调用必须建立总矩阵
    for (int region : manager.regions())
    {
        if (cache.parts.count(region) && manager.required_no_change(region)) // 已有贡献且系数未变化时复用
        {
            continue;
        }
        cache.parts[region] = integrator_.domain_matrix(region, manager.values(), diffusion); // 新矩阵使用完整场自由度编号
        manager.assembled(region); // 清除该区域的 dirty 标记
        ++counts_[counter]; changed = true;
    }
    if (changed)
    {
        cache.sum = sum_matrices(cache.parts, trial_field_.size()); // 复用未变化区域的贡献，只重新求和
    }
}

void RadioactivityAssembly::refresh_source() // 按区域缓存体积核素产生率的右端贡献
{
    bool changed = !source_cache_.sum;
    for (int region : source_.regions())
    {
        if (source_cache_.parts.count(region) && source_.required_no_change(region))
        {
            continue;
        }
        source_cache_.parts[region] = integrator_.domain_load(region, source_.values()); // F_birth,i=int_Omega_region q_birth N_i dOmega
        source_.assembled(region);
        ++counts_["source"]; changed = true;
    }
    if (changed)
    {
        source_cache_.sum = sum_vectors(source_cache_.parts, trial_field_.size());
    }
}

void RadioactivityAssembly::refresh_boundary() // 自然边界的矩阵与右端独立判断是否需要重组
{
    bool matrix_changed = !boundary_matrix_cache_.sum, load_changed = !boundary_load_cache_.sum;
    for (int zone : boundary_.zones())
    {
        if (boundary_.essential(zone)) // Dirichlet 在形成求解系统时施加，不参与自然边界积分
        {
            continue;
        }
        if (!boundary_matrix_cache_.parts.count(zone) || !boundary_.required_no_change(zone, RadioactivityBoundaryPart::Matrix))
        {
            boundary_matrix_cache_.parts[zone] = integrator_.boundary_matrix(boundary_elements_.at(zone), boundary_.values(zone).matrix); // Robin 左端 h_m N_i N_j
            boundary_.assembled(zone, RadioactivityBoundaryPart::Matrix);
            ++counts_["boundary_matrix"]; matrix_changed = true;
        }
        if (!boundary_load_cache_.parts.count(zone) || !boundary_.required_no_change(zone, RadioactivityBoundaryPart::Load))
        {
            boundary_load_cache_.parts[zone] = integrator_.boundary_load(boundary_elements_.at(zone), boundary_.values(zone).load); // Neumann -j_n 或 Robin h_m c_infinity
            boundary_.assembled(zone, RadioactivityBoundaryPart::Load);
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
    diffusion_.update(properties);
    source_.update(properties);
    boundary_.update(properties);
    refresh_matrix(diffusion_, diffusion_cache_, true, "diffusion");
    refresh_source();
    refresh_boundary();
    if (last_domain_ != diffusion_cache_.sum || last_boundary_ != boundary_matrix_cache_.sum) // 用缓存指针是否替换判断总 K 是否需要更新
    {
        std::unique_ptr<mfem::SparseMatrix> diffusion(mfem::Add(1.0, *diffusion_cache_.sum, 1.0, *boundary_matrix_cache_.sum)); // K_temp=K_diffusion+K_Robin
        // 单核素衰变是正的反应项，和扩散、Robin 左端一起组成 K。
        if (decay_constant_ != 0.0)
        {
            state_.diffusion.reset(mfem::Add(1.0, *diffusion, decay_constant_, *mass_)); // K=K_temp+lambda M
        }
        else { state_.diffusion = std::move(diffusion); }
        last_domain_ = diffusion_cache_.sum; last_boundary_ = boundary_matrix_cache_.sum;
    }
    if (last_source_ != source_cache_.sum || last_boundary_load_ != boundary_load_cache_.sum) // 源或边界右端变化时才重新汇总 F
    {
        auto load = std::make_shared<mfem::Vector>(*source_cache_.sum);
        *load += *boundary_load_cache_.sum; // F=F_birth+F_boundary
        state_.load = std::move(load);
        last_source_ = source_cache_.sum; last_boundary_load_ = boundary_load_cache_.sum;
    }
    state_.mass = mass_; // 固定单位容量矩阵无需每次重组
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
    const auto& p = interface_.prolongation(); // P 的行是完整自由度，列是完美接触约束后的独立自由度
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
            const int face = mesh.GetBdrElementFaceIndex(b); // 边界单元编号 -> 原网格面编号
            mesh.GetFaceElements(face, &first, &second);
            mfem::Array<int> dofs; trial_field_.face_dofs(mesh.GetAttribute(first), face, dofs); // 取得外表面一侧的完整自由度编号
            for (int dof : dofs)
            for (int j = p.GetI()[dof]; j < p.GetI()[dof + 1]; ++j) // 遍历 P 中该完整自由度对应的非零条目
            {
                if (p.GetData()[j] != 0.0)
                {
                    values[p.GetJ()[j]] = boundary_.values(zone).concentration; // 转为独立自由度编号并记录指定浓度
                }
            }
        }
    }
}

void RadioactivityAssembly::apply_initial_constraints() // 只调整试探浓度值，不修改待求解的矩阵与右端
{
    auto reduced_concentration = interface_.constraints().reduced_values(); // 从完整试探场提取独立自由度浓度
    std::map<int, double> essential;
    essential_values(essential);
    for (const auto& [dof, value] : essential)
    {
        reduced_concentration[dof] = value;
    }
    recover(reduced_concentration); // 经 P 恢复完整试探场，同时满足完美接触和指定表面浓度
    for (int zone : boundary_.zones())
    {
        boundary_.assembled(zone, RadioactivityBoundaryPart::Essential);
    }
}

void RadioactivityAssembly::form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs,
    mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& reduced_concentration) const
{
    std::map<int, double> essential;
    essential_values(essential);
    interface_.constraints().form_system(matrix, rhs, essential, system, load, reduced_concentration); // P^T A P、P^T b，再施加 Dirichlet 消元
}

void RadioactivityAssembly::recover(const mfem::Vector& reduced_concentration)
{
    const auto full = interface_.constraints().recover(reduced_concentration);
    trial_field_.set_values(full); // 仅修改本次求解的试探场，正式场由 accept() 更新
}

void RadioactivityAssembly::project_initial()
{
    // 带几何权重的 L2 投影保持总盘存；逐自由度平均不具有这个性质。
    const auto& p = interface_.prolongation();
    auto guess = interface_.constraints().reduced_values(); // 独立自由度上的初始猜测
    mfem::Vector original = trial_field_.values(), projected(trial_field_.size());
    p.Mult(guess, projected); projected -= original; // 已满足连续约束时无须再次投影
    if (projected.Normlinf() <= 1e-12 * original.Normlinf()) { return; }
    mfem::Vector full_rhs(trial_field_.size()), rhs(p.Width());
    mass_->Mult(original, full_rhs); p.MultTranspose(full_rhs, rhs); // 投影右端 P^T M c_original
    std::unique_ptr<mfem::SparseMatrix> system(mfem::RAP(p, *mass_, p)); // 投影矩阵 P^T M P
    mfem::GSSmoother preconditioner(*system);
    mfem::CGSolver solver; solver.SetOperator(*system); solver.SetPreconditioner(preconditioner);
    solver.SetRelTol(1e-12); solver.SetAbsTol(0.0); solver.SetMaxIter(1000); solver.SetPrintLevel(0);
    solver.Mult(rhs, guess);
    if (!solver.GetConverged()) { throw std::runtime_error("界面连续约束下的初始浓度投影未收敛"); }
    recover(guess);
}

}
