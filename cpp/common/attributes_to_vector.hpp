#pragma once

#include <mfem.hpp>
#include <vector>

namespace trimuse
{

inline std::vector<int> attributes_to_vector(const mfem::Array<int>& attributes) // 将 MFEM 属性数组转换为标准库动态数组
{
    std::vector<int> values;
    values.reserve(attributes.Size());
    for (int index = 0; index < attributes.Size(); ++index)
    {
        values.push_back(attributes[index]);
    }
    return values;
}

}
