#pragma once

#include <string>

namespace trimuse
{

struct RequiredState
{
    RequiredState(const char* name, bool reference = false);
    RequiredState(const std::string& name, bool reference = false);
    bool operator==(const std::string& other) const;

    std::string name;
    bool reference = false; // true 引用完整属性，或为 Region 模型读取区域内标量场的求积点样本；false 读取当前位置的值
};

}
