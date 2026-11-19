#include "physics/radioactivity/decay/radioactivity_decay_manager.hpp"
#include "physics/radioactivity/decay/pur_decay.hpp"

namespace trimuse
{

RadioactivityDecayManager::RadioactivityDecayManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs)
    :
    RadioactivityDomainManager(fem)
{
    registry_.register_model<PurDecay>("pur_decay", {"precomputed.decay_constant"});
    assign(configs);
}

}
