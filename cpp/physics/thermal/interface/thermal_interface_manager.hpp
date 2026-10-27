#pragma once

#include <map>
#include <memory>

#include "common/model_registry.hpp"
#include "physics/thermal/interface/thermal_interface_model.hpp"
#include "physics/thermal/thermal_field.hpp"

namespace trimuse
{

class ThermalInterfaceManager
{
public:
    ThermalInterfaceManager(FemSystem& fem, ThermalField& field, const std::vector<ThermalInterfaceConfig>& configs);
    const mfem::SparseMatrix& prolongation() const { return *prolongation_; }

private:
    ModelRegistry<ThermalInterfaceModel> registry_;
    std::map<int, std::unique_ptr<ThermalInterfaceModel>> models_; // 界面物理标识 -> 具体模型
    std::unique_ptr<mfem::SparseMatrix> prolongation_;
};

}
