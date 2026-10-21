#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/model_parameters.hpp"
#include "fem/fem_system.hpp"
#include "fem/geometric_weighted_coefficient.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct ThermalBoundaryConfig
{
    int boundary_attribute;
    std::string model_name;
    ModelParameterMap parameters;
};

class ThermalBoundaryManager
{
public:
    ThermalBoundaryManager(FemSystem& fem_system, const std::vector<ThermalBoundaryConfig>& boundary_configs);

    void update(const PropertiesFieldData& properties_field_data);

    void add_integrators(mfem::BilinearForm& bilinear_form);

    void add_integrators(mfem::LinearForm& linear_form);

    void apply_essential_conditions(mfem::GridFunction& temperature, mfem::Array<int>& essential_true_dofs) const;

private:
    struct BoundaryCoefficients
    {
        explicit BoundaryCoefficients(const FemSystem& fem_system)
            : weighted_matrix(matrix, fem_system), weighted_rhs(rhs, fem_system) {}

        mfem::ConstantCoefficient matrix{0.0}; // Robin: h; Neumann 无左端项
        mfem::ConstantCoefficient rhs{0.0}; // Robin: h T_infinity; Neumann: -q_n
        mfem::ConstantCoefficient temperature{0.0}; // Dirichlet: 指定表面温度, 不乘几何权重
        GeometricWeightedCoefficient weighted_matrix;
        GeometricWeightedCoefficient weighted_rhs;
    };

    FemSystem& fem_system_;
    std::vector<ThermalBoundaryConfig> boundary_configs_;
    std::vector<mfem::Array<int>> boundary_markers_;
    mfem::Array<int> essential_boundary_marker_; // 所有 Dirichlet 分区的并集
    // 与配置一一对应, 对象地址固定, 供积分器持续引用。
    std::vector<std::unique_ptr<BoundaryCoefficients>> boundary_coefficients_;
};

}
