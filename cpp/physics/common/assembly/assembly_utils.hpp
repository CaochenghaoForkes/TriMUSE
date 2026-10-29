#pragma once

#include <map>
#include <memory>
#include <mfem.hpp>

namespace trimuse
{

// 各部分均采用完整场的整体编号；求和生成新对象，不修改已有贡献。
std::shared_ptr<mfem::SparseMatrix> sum_matrices(const std::map<int, std::shared_ptr<const mfem::SparseMatrix>>& parts, int size);
std::shared_ptr<mfem::Vector> sum_vectors(const std::map<int, std::shared_ptr<const mfem::Vector>>& parts, int size);

}
