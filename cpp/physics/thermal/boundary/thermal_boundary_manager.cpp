#include "physics/thermal/boundary/thermal_boundary_manager.hpp"
#include "physics/thermal/boundary/thermal_robin_boundary.hpp"
#include "physics/thermal/boundary/thermal_neumann_boundary.hpp"
#include "physics/thermal/boundary/thermal_dirichlet_boundary.hpp"

#include <stdexcept>

namespace trimuse
{

namespace
{

class BoundaryMassIntegrator final : public mfem::MassIntegrator // 重写边界单元积分器 \[K^\Gamma_{ij}=\int_{\Gamma_e}N_i\,h\,N_j\,\mathrm dS\]
{   // 如果用默认的 MassIntegrator MFEM 会选择默认积分规则，我们做这个小包装就是希望它遵守输入中的 boundary_order
public:
    BoundaryMassIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : mfem::MassIntegrator(coefficient), fem_system_(fem_system) {}

    void AssembleElementMatrix(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::DenseMatrix& matrix) override // 重写 MFEM 装配边界单元时的方法 传入该边界单元的有限元、几何映射，以及用于接收计算结果的单元矩阵
    {   // element 当前边界单元使用的参考有限元，包括参考形状、基函数和自由度等信息
        SetIntRule(fem_system_.boundary_quadrature_rule(element.GetGeomType())); // 根据边界单元形状，选取我们保存的求积规则
        mfem::MassIntegrator::AssembleElementMatrix(element, transformation, matrix); // 调用基类的方法，真正计算：积分_边界单元(Ni*h*Nj)dS 传入矩阵
    }

private:
    const FemSystem& fem_system_;
};

class BoundaryLFIntegrator final : public mfem::BoundaryLFIntegrator // 边界右端积分, 系数为 Robin 的 hT_infinity 或 Neumann 的 -q_n
{
public:
    BoundaryLFIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : mfem::BoundaryLFIntegrator(coefficient), fem_system_(fem_system) {}

    using mfem::BoundaryLFIntegrator::AssembleRHSElementVect;

    void AssembleRHSElementVect(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::Vector& vector) override
    {
        SetIntRule(fem_system_.boundary_quadrature_rule(element.GetGeomType())); // 按边界单元形状选求积规则
        mfem::BoundaryLFIntegrator::AssembleRHSElementVect(element, transformation, vector); // 计算积分_边界单元(Ni*Coef)dS, 写入边界单元载荷向量 vector
    }

private:
    const FemSystem& fem_system_;
};

}

ThermalBoundaryManager::ThermalBoundaryManager(FemSystem& fem_system, const std::vector<ThermalBoundaryConfig>& boundary_configs)
    : fem_system_(fem_system),
      boundary_configs_(boundary_configs),
      boundary_markers_(boundary_configs.size())
{
    const int marker_size = fem_system_.mesh_data().mfem_mesh().bdr_attributes.Max(); // 最大边界物理组编号
    essential_boundary_marker_.SetSize(marker_size);
    essential_boundary_marker_ = 0;
    for (std::size_t index = 0; index < boundary_configs_.size(); ++index)
    {
        mfem::Array<int>& marker = boundary_markers_[index];
        marker.SetSize(marker_size);
        marker = 0;
        marker[boundary_configs_[index].boundary_attribute - 1] = 1;
        if (boundary_configs_[index].model_name == "dirichlet")
        {
            essential_boundary_marker_[boundary_configs_[index].boundary_attribute - 1] = 1;
        }
        boundary_coefficients_.push_back(std::make_unique<BoundaryCoefficients>(fem_system)); // 为当前边界创建系数存储并交给智能指针管理
    }
}

void ThermalBoundaryManager::update(const PropertiesFieldData& properties_field_data)
{
    for (std::size_t index = 0; index < boundary_configs_.size(); ++index)
    {
        const ThermalBoundaryConfig& config = boundary_configs_[index];
        BoundaryCoefficients& coefficients = *boundary_coefficients_[index];
        if (config.model_name == "robin")
        {
            const RobinValues values = ThermalRobinBoundary::evaluate(properties_field_data, config.boundary_attribute);
            coefficients.matrix.constant = values.matrix_coefficient;
            coefficients.rhs.constant = values.rhs_coefficient;
        }
        else if (config.model_name == "neumann")
        {
            coefficients.rhs.constant = ThermalNeumannBoundary::evaluate(properties_field_data, config.boundary_attribute);
        }
        else if (config.model_name == "dirichlet")
        {
            coefficients.temperature.constant = ThermalDirichletBoundary::evaluate(properties_field_data, config.boundary_attribute);
        }
    }
}

void ThermalBoundaryManager::add_integrators(mfem::BilinearForm& bilinear_form, mfem::LinearForm& linear_form) // 积分器BoundaryMassIntegrator/BoundaryLFIntegrator负责算出单元的矩阵或向量，BilinearForm、LinearForm 负责按照自由度编号，把它们组装到总体左侧矩阵和右端向量中
{
    for (std::size_t index = 0; index < boundary_configs_.size(); ++index)
    {
        BoundaryCoefficients& coefficients = *boundary_coefficients_[index];
        if (boundary_configs_[index].model_name == "robin")
        {
            bilinear_form.AddBoundaryIntegrator(new BoundaryMassIntegrator(coefficients.weighted_matrix, fem_system_), boundary_markers_[index]);
            linear_form.AddBoundaryIntegrator(new BoundaryLFIntegrator(coefficients.weighted_rhs, fem_system_), boundary_markers_[index]);
        }
        else if (boundary_configs_[index].model_name == "neumann")
        {
            linear_form.AddBoundaryIntegrator(new BoundaryLFIntegrator(coefficients.weighted_rhs, fem_system_), boundary_markers_[index]);
        }
        else if (boundary_configs_[index].model_name != "dirichlet")
        {
            throw std::logic_error("尚未实现的热边界模型: " + boundary_configs_[index].model_name);
        }
    }
}

void ThermalBoundaryManager::apply_essential_conditions(mfem::GridFunction& temperature, mfem::Array<int>& essential_true_dofs) const
{   
    for (std::size_t index = 0; index < boundary_configs_.size(); ++index)
    {
        if (boundary_configs_[index].model_name == "dirichlet")
        {
            // 找到该分区上的温度自由度 把它们的值设为指定表面温度
            // boundary_coefficients_[index]->temperature 该分区指定的温度系数
            // boundary_markers_[index] 选择该边界分区的 marker，告诉 MFEM 在哪里设置温度
            temperature.ProjectBdrCoefficient(boundary_coefficients_[index]->temperature, boundary_markers_[index]); 
        }
    }
    // 根据 Dirichlet 边界标记，找出所有需要约束的真实自由度编号
    // essential_boundary_marker_ 标明哪些边界物理组属于 Dirichlet 边界
    // essential_true_dofs 输出，MFEM 将这些边界对应的真实自由度编号(节点温度未知量的编号)填进去
    temperature.FESpace()->GetEssentialTrueDofs(essential_boundary_marker_, essential_true_dofs);
}

}
