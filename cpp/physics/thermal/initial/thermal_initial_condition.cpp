#include "physics/thermal/initial/thermal_initial_condition.hpp"
#include "fem/geometric_weighted_coefficient.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace trimuse
{

namespace
{

class InitialTemperatureCoefficient final : public mfem::Coefficient // 把"区域编号->初始温度"映射表 包装为 MFEM 能在积分时调用的系数对象
{
public:
    explicit InitialTemperatureCoefficient(const std::unordered_map<int, double>& region_values) : region_values_(region_values) // 构造时接收区域初温表 region_values 区域名 -> 温度
    {
    }

    double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint&) override // 初温在每个区域内为常数,所以实际上无需使用积分点坐标
    {
        return region_values_.at(transformation.Attribute); // transformation.Attribute 是当前单元所属区域的属性编号，根据它查表返回初温
    }

private:
    const std::unordered_map<int, double>& region_values_; // 初始温度表 区域编号 -> 初温
};

class ProjectionMassIntegrator final : public mfem::MassIntegrator // 初始场 L2 投影的质量矩阵积分器(int_Omega_e c(x)NiNj dOmega) 沿用 MFEM 的质量矩阵计算方法 并指定使用程序统一的域内求积规则 
{
public:
    ProjectionMassIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system) : mfem::MassIntegrator(coefficient), fem_system_(fem_system) // mfem::MassIntegrator(coefficient) 使用已经包含降维几何权重的单位系数 初始化父类 让它在积分时使用这个系数 fem_system 保存有限元系统的引用，用于查询求积规则
    {
    }

    void AssembleElementMatrix(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::DenseMatrix& matrix) override // 计算一个域单元的局部质量矩阵
    {
        SetIntRule(fem_system_.domain_quadrature_rule(element.GetGeomType())); // 根据当前单元的参考几何类型，取得程序配置的域内求积规则，并设置给积分器
        mfem::MassIntegrator::AssembleElementMatrix(element, transformation, matrix); // 显式调用父类方法，使用刚才设置的求积规则计算 sum_q c(xq) Ni(\xi_q) Nj(\xi_q) wq Jq
    }

private:
    const FemSystem& fem_system_;
};

class ProjectionLFIntegrator final : public mfem::DomainLFIntegrator
{
public:
    ProjectionLFIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system) : mfem::DomainLFIntegrator(coefficient), fem_system_(fem_system) // 组装初始场 L2 投影的右端向量 int_Omega T_initial Ni dOmega
    {
    }

    using mfem::DomainLFIntegrator::AssembleRHSElementVect;

    void AssembleRHSElementVect(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::Vector& vector) override
    {
        SetIntRule(fem_system_.domain_quadrature_rule(element.GetGeomType())); // 计算当前单元的局部右端向量
        mfem::DomainLFIntegrator::AssembleRHSElementVect(element, transformation, vector);
    }

private:
    const FemSystem& fem_system_;
};

}

void initialize_temperature(RegionFieldData& accepted_temperature_field, const std::vector<ThermalInitialConfig>& initial_configs) // 给分域温度场中的各个区域分别设置均匀初温
{
    for (const auto& config : initial_configs)
    {
        if (config.model_name != "uniform") 
        { 
            throw std::invalid_argument("尚未支持的初始温度模型: " + config.model_name); 
        }
        // 分域场两侧的自由度独立，所以这里可以直接逐域赋值。 若界面采用理想接触，后续再由约束下的初值投影处理两侧初温的一致性
        accepted_temperature_field.field_data(config.region_attribute).assign(std::get<double>(config.parameters.at("uniform_value"))); // field_data(config.region_attribute): 取得指定区域的 FieldData parameters.at("uniform_value")：从参数字典取出初温参数 std::get<double>(...)：从 variant 中取出 double 数值 .assign(...)：把该区域场向量的全部自由度设为这个温度
    }
}

void initialize_temperature(const FemSystem& fem_system, FieldData& accepted_temperature_field, const std::vector<ThermalInitialConfig>& initial_configs) // 把各区域指定的初温，转换为整个连续 H1 温度场。如果不同区域初温不同，就通过 L2 投影确定共享节点上的温度
{
    std::unordered_map<int, double> region_values; // 建立区域 -> 初温表
    for (const ThermalInitialConfig& config : initial_configs)
    {
        if (config.model_name != "uniform")
        {
            throw std::invalid_argument("尚未支持的初始温度模型: " + config.model_name);
        }
        region_values[config.region_attribute] = std::get<double>(config.parameters.at("uniform_value"));
    }
    const double uniform_value = region_values.begin()->second; // 先取任意一项的温度作为比较基准
    if (std::all_of(region_values.begin(), region_values.end(), [uniform_value](const auto& item) { return item.second == uniform_value; })) // 检查每一项的温度是否都等于基准值 若全部相同
    {
        accepted_temperature_field.assign(uniform_value); // 直接给全部自由度赋相同温度，初始化完成
        return;
    }

    // 分区域阶跃通过 L2 投影得到连续场, 不按区域覆盖公共节点
    // 用户指定区域阶跃初温场 T_0 希望找到连续有限元温度 T_h = sum_j N_jT_j
    // 使得误差最小 int_Omega g(x)(T_h-T_0)^2 dOmega g(x) 是几何权重
    // 我们要确定的未知数是各个自由度的初始值也就是 T_j 这些值和未知 x 无关, 因此上面积分的最小值就是对各个 T_j 求导令其为零
    // 得到 sum_j int_Omega g N_i N_j dOmega T_j = int_Omega g N_i T_0 dOmega
    // 令左侧积分为 M_ij 右侧为 bi 即为解出 sum_j M_ij Tj = bi 即为 MT = b
    mfem::GridFunction& accepted_temperature_values = accepted_temperature_field.mfem_grid_function(); // 取得现有温度场的引用，最后的结果直接写到这里
    mfem::FiniteElementSpace* space = accepted_temperature_values.FESpace(); // 取得它所属的有限元空间，决定使用哪些形函数、有哪些自由度、如何组装
    InitialTemperatureCoefficient initial_temperature(region_values); // 系数对象根据当前单元的区域编号，返回对应初温
    mfem::ConstantCoefficient unit_coefficient(1.0);
    GeometricWeightedCoefficient weighted_unit_coefficient(unit_coefficient, fem_system); // g
    GeometricWeightedCoefficient weighted_initial_temperature(initial_temperature, fem_system); // g*T_0
    mfem::BilinearForm mass_form(space);
    mass_form.AddDomainIntegrator(new ProjectionMassIntegrator(weighted_unit_coefficient, fem_system));
    mass_form.Assemble(); // 组装 M
    mfem::LinearForm right_hand_side(space);
    right_hand_side.AddDomainIntegrator(new ProjectionLFIntegrator(weighted_initial_temperature, fem_system));
    right_hand_side.Assemble(); // 组装 b

    mfem::Array<int> essential_true_dofs; // 投影没有要固定的边界自由度
    mfem::SparseMatrix system_matrix; // 最终系数矩阵
    mfem::Vector true_temperature, true_right_hand_side; // 真实自由度上的温度未知量 真实自由度上的右端向量
    mass_form.FormLinearSystem(essential_true_dofs, accepted_temperature_values, right_hand_side, system_matrix, true_temperature, true_right_hand_side); // 组装 MT=b
    mfem::GSSmoother preconditioner(system_matrix); // 构造 Gauss–Seidel 类预条件器，辅助迭代求解更快收敛
    mfem::CGSolver solver; //使用共轭梯度法
    solver.SetOperator(system_matrix); // 指定矩阵
    solver.SetPreconditioner(preconditioner); // 指定预条件器
    solver.SetRelTol(1e-12);
    solver.SetAbsTol(0.0);
    solver.SetMaxIter(1000);
    solver.SetPrintLevel(0); // 关闭迭代打印信息
    solver.Mult(true_right_hand_side, true_temperature); // 求解 把解写入 true_temperature
    if (!solver.GetConverged()) // 确认数值求解成功
    {
        throw std::runtime_error("初始温度的 L2 投影未收敛");
    }
    mass_form.RecoverFEMSolution(true_temperature, right_hand_side, accepted_temperature_values); // 把真实自由度上的解恢复成 GridFunction 的表示，写回最开始取得的 accepted_temperature_values
}

}
