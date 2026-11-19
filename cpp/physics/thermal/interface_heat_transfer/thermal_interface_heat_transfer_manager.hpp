#pragma once

#include <map>
#include <memory>
#include <vector>

#include "physics/common/constraint/dof_constraints.hpp"
#include "physics/thermal/interface_heat_transfer/thermal_interface_heat_transfer_model.hpp"

namespace trimuse
{

class PropertiesFieldData;

class ThermalInterfaceManager
{
public:
    ThermalInterfaceManager(FemSystem& fem, TrialField& trial_field, DofConstraints& constraints, const std::vector<ThermalInterfaceConfig>& interface_heat_transfer_configs);
    void update(const PropertiesFieldData& properties);
    std::vector<int> interface_heat_transfer_attributes() const;
    bool required_no_change(int attribute) const;
    void assembled(int attribute);
    std::unique_ptr<mfem::SparseMatrix> assemble_matrix(int attribute, FemSystem& fem, const TrialField& trial_field, DofConstraints& constraints) const;

private:
    struct ResistanceFace
    {
        const MeshData::Interface* interface;
        double resistance = 0.0;
    };

    struct InterfaceAssignment
    {
        std::vector<ResistanceFace> faces;
        bool dirty = true;
    };

    std::map<int, InterfaceAssignment> interface_heat_transfer_assignments_; // 材料界面编号 -> 热阻面片及组装状态
};

}
