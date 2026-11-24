#pragma once

#include <map>
#include <memory>
#include <vector>

#include "physics/common/constraint/dof_constraints.hpp"
#include "physics/thermal/interface_heat_transfer/thermal_interface_heat_transfer_model.hpp"

#include "properties/properties_field_data.hpp"

namespace trimuse
{

class ThermalInterfaceManager
{
public:
    ThermalInterfaceManager(FemSystem& fem, TrialField& trial_field, DofConstraints& constraints, const std::vector<ThermalInterfaceConfig>& interface_heat_transfer_configs);
    void update(const PropertiesFieldData& properties);
    const std::vector<double>& values() const { return values_; } // 全网格面积分点热阻，按 face_quadrature_offset(face) + q 访问
    std::vector<int> interface_heat_transfer_attributes() const;
    bool required_no_change(int attribute) const;
    void assembled(int attribute);
    std::unique_ptr<mfem::SparseMatrix> assemble_matrix(int attribute, FemSystem& fem, const TrialField& trial_field, DofConstraints& constraints, const PropertiesFieldData::FaceIntegrationPointValues& weights) const;

private:
    struct InterfaceAssignment
    {
        std::vector<const MeshData::Interface*> faces; // 只保存未建立完美接触约束的面片
        bool dirty = true;
    };

    const FemSystem& fem_;
    std::vector<double> values_; // 全网格面积分点热阻缓存，未参与热阻组装的位置保持零
    std::map<int, InterfaceAssignment> interface_heat_transfer_assignments_; // 材料界面编号 -> 热阻面片及组装状态
};

}
