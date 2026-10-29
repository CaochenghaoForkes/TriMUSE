#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fem/fem_system.hpp"

namespace trimuse
{

class OutputManager
{
public:
    using SolveInformation = std::vector<std::pair<std::string, double>>; // 求解信息如: picard_iterations -> 3 solve_elapsed_seconds -> 0.02 ...

    OutputManager(FemSystem& fem_system, const std::string& output_directory);

    void add_field(const std::string& field_name, const std::string& directory_name);

    void save(const std::string& field_name, int cycle, double time, double time_step);

    void clear_solve_information(const std::string& field_name);

    void append_solve_information(const std::string& field_name, const std::string& name, double value);

    const SolveInformation& solve_information(const std::string& field_name) const;

private:
    struct FieldOutput // 管理每个场的 ParaView 输出对象，以及本次求解的统计信息
    {
        std::unique_ptr<mfem::L2_FECollection> collection; // 拥有一套 L2 有限元基函数集合，供分域场输出使用 L2 空间允许单元之间的场值不连续，因此可以保留材料界面两侧不同的温度或浓度

        std::unique_ptr<mfem::FiniteElementSpace> space; // 建立在原网格上、用于输出的有限元空间，使用上面的 L2 集合

        std::unique_ptr<mfem::GridFunction> values; // 保存这些自由度上的场值 (输出空间中的场值向量)

        std::unique_ptr<mfem::ParaViewDataCollection> collection_output; // MFEM 的 ParaView 输出管理对象，负责登记场变量、设置时间和步数，并保存 PVD、PVTU、VTU 等结果文件
    };

    void update_region_output(const std::string& name, mfem::GridFunction& output); // 在保存结果前，根据场名找到对应分域场，把各区域的当前场值更新到 output 这个输出 GridFunction 中

    FemSystem& fem_system_; // 有限元系统的引用

    std::string output_directory_; // 结果输出目录

    std::unordered_map<std::string, FieldOutput> field_outputs_; // 场名 -> 该场的输出资源

    std::unordered_map<std::string, SolveInformation> solve_information_; // 场名 -> 该场本次求解的统计信息
};

}
