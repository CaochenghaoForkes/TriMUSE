#pragma once

#include <string>

#include "fem/fem_system.hpp"

namespace trimuse
{

class OutputManager
{
public:
    OutputManager(FemSystem& fem_system, const std::string& output_directory);

    void save(int cycle, double time, double time_step);

private:
    // 引用 FemSystem 的网格与场, 不接管其所有权。
    mfem::ParaViewDataCollection temperature_output_;
};

}
