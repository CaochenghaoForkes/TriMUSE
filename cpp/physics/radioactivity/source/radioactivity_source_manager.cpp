#include "physics/radioactivity/source/radioactivity_source_manager.hpp"
#include "physics/radioactivity/source/volumetric_nuclide_birth_rate_source.hpp"

namespace trimuse
{

RadioactivitySourceManager::RadioactivitySourceManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs) // 只给输入卡配置过的区域安排源模型
    : RadioactivityDomainManager(fem)
{
    registry_.register_model<VolumetricNuclideBirthRateSource>("volumetric_nuclide_birth_rate", {"derived.volumetric_nuclide_birth_rate"});
    assign(configs);
}

}
