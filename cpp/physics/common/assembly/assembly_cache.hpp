#pragma once

#include <map>
#include <memory>

namespace trimuse
{

template<class Value>
struct AssemblyCache // 按区域或边界保存只读组装贡献；更新时替换指针，保留旧状态快照
{
    std::map<int, std::shared_ptr<const Value>> parts;
    std::shared_ptr<const Value> sum;
};

}
