#include "physics/radioactivity/diffusion/radioactivity_diffusion_manager.hpp"
#include "physics/radioactivity/diffusion/fick_diffusion.hpp"

namespace trimuse
{

RadioactivityDiffusionManager::RadioactivityDiffusionManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs) // 只给配置的区域安排 Fick 扩散模型
    : RadioactivityDomainManager(fem)
{
    registry_.register_model<FickDiffusion>("fick", {"material.radioactivity.nuclide_diffusion_coef"});
    assign(configs);
}

}
