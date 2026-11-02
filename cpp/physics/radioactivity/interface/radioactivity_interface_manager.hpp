#pragma once

#include "common/model_registry.hpp"
#include "physics/radioactivity/interface/radioactivity_interface_model.hpp"

namespace trimuse
{

class RadioactivityInterfaceManager
{
public:
    RadioactivityInterfaceManager(FemSystem& fem, const TrialField& trial_field,
        const std::vector<RadioactivityInterfaceConfig>& configs);

    const DofConstraints& constraints() const { return constraints_; }
    const mfem::SparseMatrix& prolongation() const { return constraints_.prolongation(); }

private:
    ModelRegistry<RadioactivityInterfaceModel> registry_;
    std::map<int, std::unique_ptr<RadioactivityInterfaceModel>> models_;
    DofConstraints constraints_;
};

}
