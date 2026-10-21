#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "fem/fem_system.hpp"

namespace trimuse
{

class OutputManager
{
public:
    OutputManager(FemSystem& fem_system, const std::string& output_directory);

    void add_field(const std::string& field_name, const std::string& directory_name);

    void save(const std::string& field_name, int cycle, double time, double time_step);

private:
    FemSystem& fem_system_;
    std::string output_directory_;
    std::unordered_map<std::string, std::unique_ptr<mfem::ParaViewDataCollection>> field_outputs_;
};

}
