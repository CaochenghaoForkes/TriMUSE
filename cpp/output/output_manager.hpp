#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fem/fem_system.hpp"
#include "output/domain_output.hpp"
#include "output/domain_statistics_output.hpp"
#include "output/scalar_output.hpp"

namespace trimuse
{

class PropertiesSystem;

class OutputManager
{
public:
    using OutputConfigs = std::unordered_map<std::string, std::vector<OutputConfig>>; // 输出范围 -> 输出项，domain 为域输出，scalar 为标量时间序列
    using LocationNames = std::unordered_map<std::string, OutputLocationNames>; // region/zone -> 位置编号与名称
    using SolveInformation = std::vector<std::pair<std::string, double>>; // 求解信息如: picard_iterations -> 3 solve_elapsed_seconds -> 0.02 ...

    OutputManager(FemSystem& fem_system, const std::string& output_directory);
    void initialize(PropertiesSystem& properties, const OutputConfigs& configs, const LocationNames& location_names, const DomainStatisticsRequests& domain_statistics = {}); // 分别初始化域输出与标量时间序列
    void save(int cycle, double time, double time_step); // 按统一清单保存域场 ParaView 文件和标量属性 CSV
    void clear_solve_information(const std::string& field_name);
    void append_solve_information(const std::string& field_name, const std::string& name, double value);
    const SolveInformation& solve_information(const std::string& field_name) const;

private:
    void write_property_units(const PropertiesSystem& properties, const OutputConfigs& configs, const DomainStatisticsRequests& domain_statistics) const; // 写出本次输出量的计算单位，不改变数值

    FemSystem& fem_system_; // 有限元系统的引用
    PropertiesSystem* properties_ = nullptr; // 保存前同步已接受场，随后统一读取属性表
    std::unordered_map<std::string, std::vector<std::string>> output_names_; // 输出范围 -> 已登记的域或标量名称
    std::string output_directory_; // 结果输出目录
    std::unordered_map<std::string, std::unique_ptr<DomainOutput>> domain_outputs_; // 域输出名称 -> 对应的投影与文件输出对象
    std::unordered_map<std::string, std::unique_ptr<ScalarOutput>> scalar_outputs_; // 标量属性名称 -> CSV 输出对象
    std::unordered_map<std::string, std::unique_ptr<DomainStatisticsOutput>> domain_statistics_outputs_; // 域属性名 -> 原始积分点统计输出
    std::unordered_map<std::string, SolveInformation> solve_information_; // 场名 -> 该场本次求解的统计信息
};

}
