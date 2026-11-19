#pragma once

#include "common/model_parameters.hpp"

namespace trimuse
{

struct MechanicsInterfaceConfig // 界面位置、组装模型及参数，各界面项共用相同输入格式
{
    int boundary_attribute;
    std::string model_name;
    ModelParameterMap parameters;
};

}
