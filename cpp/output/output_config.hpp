#pragma once

#include <map>
#include <string>

namespace trimuse
{

struct OutputConfig
{
    std::string name;
    std::string directory;
};

using OutputLocationNames = std::map<int, std::string>; // 位置编号 -> 名称，按编号排列输出列

}
