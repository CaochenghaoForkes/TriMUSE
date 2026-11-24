#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "fem/fem_system.hpp"
#include "output/output_config.hpp"

namespace trimuse
{

class PropertiesSystem;

using DomainStatisticsRequests = std::unordered_map<std::string, std::vector<std::string>>; // 域属性名 -> 统计操作

class DomainStatisticsOutput // 从原始域积分点统计并输出标量时间序列，不经过 L2 投影
{
public:
    DomainStatisticsOutput(FemSystem& fem_system, PropertiesSystem& properties, const std::string& name, const std::vector<std::string>& processes, const std::string& directory, const OutputLocationNames& region_names);
    static std::unordered_map<std::string, std::unique_ptr<DomainStatisticsOutput>> initialize(FemSystem& fem_system, PropertiesSystem& properties, const DomainStatisticsRequests& requests, const std::string& output_directory, const OutputLocationNames& region_names);
    void save(int cycle, double time, double time_step);

private:
    FemSystem& fem_system_;
    PropertiesSystem& properties_;
    std::string name_;
    OutputLocationNames region_names_;
    std::map<std::string, std::ofstream> files_; // 操作名 -> 对应 CSV，每项统计保持独立文件
};

}
