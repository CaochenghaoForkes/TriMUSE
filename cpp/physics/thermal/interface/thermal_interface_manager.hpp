#pragma once

#include "common/model_parameters.hpp"
#include "physics/thermal/thermal_field.hpp"

namespace trimuse
{
struct ThermalInterfaceConfig
{
    int boundary_attribute;
    std::string model_name = "perfect_contact";
    ModelParameterMap parameters;
};

// 默认理想接触用精确自由度约束; conductance 用有限传热系数, insulated 不连接两侧。
class ThermalInterfaceManager
{
public:
    ThermalInterfaceManager(FemSystem& fem, ThermalField& field, const std::vector<ThermalInterfaceConfig>& configs);
    const mfem::SparseMatrix& prolongation() const { return *prolongation_; }
    const mfem::SparseMatrix& matrix() const { return *matrix_; }

private:
    std::unique_ptr<mfem::SparseMatrix> prolongation_;
    std::unique_ptr<mfem::SparseMatrix> matrix_;
};
}
