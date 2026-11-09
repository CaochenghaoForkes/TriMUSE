#pragma once

#include "common/model_parameters.hpp"

namespace trimuse
{

struct MechanicsDomainConfig // 某个材料区域采用的力学域内模型及其参数
{
    int region_attribute;
    std::string model_name;
    ModelParameterMap parameters;
};

}
