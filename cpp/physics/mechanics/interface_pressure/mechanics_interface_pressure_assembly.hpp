#pragma once

#include "physics/common/field/trial_field.hpp"
#include "physics/common/interface/interface_context.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class MechanicsInterfacePressureAssembly // 将单侧压力积分为完整位移自由度上的载荷，不登记连接约束
{
public:
    MechanicsInterfacePressureAssembly(const FemSystem& fem, const TrialField& trial_field);
    void add_pressure(const InterfaceContext& context, int side, double pressure, const PropertiesFieldData::FaceIntegrationPointValues& weights);
    std::unique_ptr<mfem::Vector> finalize();

private:
    const FemSystem& fem_;
    const TrialField& trial_field_;
    std::unique_ptr<mfem::Vector> load_;
};

}
