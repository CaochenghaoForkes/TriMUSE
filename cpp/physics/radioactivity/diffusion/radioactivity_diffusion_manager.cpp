#include "physics/radioactivity/diffusion/radioactivity_diffusion_manager.hpp"
#include "physics/radioactivity/diffusion/fick_diffusion.hpp"

namespace trimuse
{

RadioactivityDiffusionManager::RadioactivityDiffusionManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs) // 未显式配置时，每个区域默认使用 Fick 模型
    : RadioactivityDomainManager(fem)
{
    registry_.register_model<FickDiffusion>("fick", {"material.radioactivity.nuclide_diffusion_coef"});
    assign(configs.empty() ? defaults(fem, "fick") : configs);
}

}
