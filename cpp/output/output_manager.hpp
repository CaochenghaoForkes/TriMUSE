#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fem/fem_system.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class OutputManager
{
public:
    using SolveInformation = std::vector<std::pair<std::string, double>>; // 求解信息如: picard_iterations -> 3 solve_elapsed_seconds -> 0.02 ...

    OutputManager(FemSystem& fem_system, const std::string& output_directory);
    void add_field(const std::string& field_name, const std::string& directory_name, bool project_to_l2 = false, int order = 0); // 标量求解场沿用原空间，标量求积点数组按指定阶次建立 L2 输出空间
    void save(const std::string& field_name, const FieldView& values, int cycle, double time, double time_step); // 按输出节点读取传入的正式标量场，不做 L2 投影
    void save(const std::string& field_name, const std::vector<double>& values, const PropertiesFieldData::DomainIntegrationPointValues& weights, int cycle, double time, double time_step); // 传入全域标量求积点数组，逐单元 L2 投影后保存
    void clear_solve_information(const std::string& field_name);
    void append_solve_information(const std::string& field_name, const std::string& name, double value);
    const SolveInformation& solve_information(const std::string& field_name) const;

private:
    struct FieldOutput // 管理每个场的 ParaView 输出对象，以及本次求解的统计信息
    {
        std::unique_ptr<mfem::L2_FECollection> collection; // 拥有一套 L2 有限元基函数集合，供分域场和求积点数据输出使用 L2 空间允许单元之间的场值不连续，因此可以保留材料界面两侧不同的温度或浓度
        std::unique_ptr<mfem::FiniteElementSpace> space; // 建立在原网格上、用于输出的有限元空间，使用上面的 L2 集合
        std::unique_ptr<mfem::GridFunction> values; // 保存这些自由度上的场值 (输出空间中的场值向量)
        std::unique_ptr<mfem::ParaViewDataCollection> collection_output; // MFEM 的 ParaView 输出管理对象，负责登记场变量、设置时间和步数，并保存 PVD、PVTU、VTU 等结果文件
    };

    void write_field(const std::string& field_name, int cycle, double time, double time_step); // 设置时间信息并写出已经填好的输出场
    void update_field_output(const FieldView& view, mfem::GridFunction& output); // 将传入的连续标量场或分域标量场插值到输出节点

    FemSystem& fem_system_; // 有限元系统的引用
    std::string output_directory_; // 结果输出目录
    std::unordered_map<std::string, FieldOutput> field_outputs_; // 场名 -> 该场的输出资源
    std::unordered_map<std::string, SolveInformation> solve_information_; // 场名 -> 该场本次求解的统计信息
};

}
