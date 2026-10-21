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
    FemSystem& fem_system_;
    std::string output_directory_;
    std::unordered_map<std::string, std::unique_ptr<mfem::ParaViewDataCollection>> field_outputs_;
    std::unordered_map<std::string, SolveInformation> solve_information_; // 场名 → 本次求解的有序信息, 允许名称重复
};

}
