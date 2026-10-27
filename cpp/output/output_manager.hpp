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
    using SolveInformation = std::vector<std::pair<std::string, double>>;

    OutputManager(FemSystem& fem_system, const std::string& output_directory);

    void add_field(const std::string& field_name, const std::string& directory_name);

    void save(const std::string& field_name, int cycle, double time, double time_step);

    void clear_solve_information(const std::string& field_name);

    void append_solve_information(const std::string& field_name, const std::string& name, double value);

    const SolveInformation& solve_information(const std::string& field_name) const;

private:
    // 分域场以父网格上的不连续场输出, 保留界面两侧值及原有 PVD 路径。
    struct FieldOutput
    {
        std::unique_ptr<mfem::L2_FECollection> collection;
        std::unique_ptr<mfem::FiniteElementSpace> space;
        std::unique_ptr<mfem::GridFunction> values;
        std::unique_ptr<mfem::ParaViewDataCollection> collection_output;
    };
    void update_region_output(const std::string& name, mfem::GridFunction& output);

    FemSystem& fem_system_;
    std::string output_directory_;
    std::unordered_map<std::string, FieldOutput> field_outputs_;
    std::unordered_map<std::string, SolveInformation> solve_information_; // 场名 → 本次求解的有序信息, 允许名称重复
};

}
