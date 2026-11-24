#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fem/fem_system.hpp"
#include "output/domain_output.hpp"

namespace trimuse
{

class PropertiesSystem;

class OutputManager
{
public:
    using OutputConfigs = std::unordered_map<std::string, std::vector<OutputConfig>>; // 输出范围 -> 输出项，当前实现 domain 域输出
    using SolveInformation = std::vector<std::pair<std::string, double>>; // 求解信息如: picard_iterations -> 3 solve_elapsed_seconds -> 0.02 ...

    OutputManager(FemSystem& fem_system, const std::string& output_directory);
    void initialize(PropertiesSystem& properties, const OutputConfigs& configs); // 登记域输出清单和所需场采样，统一建立输出场
    void save(int cycle, double time, double time_step); // 按统一清单输出：正式 FEM 场按求解阶次插值，积分点量按 0 阶 L2 投影
    void clear_solve_information(const std::string& field_name);
    void append_solve_information(const std::string& field_name, const std::string& name, double value);
    const SolveInformation& solve_information(const std::string& field_name) const;

private:
    FemSystem& fem_system_; // 有限元系统的引用
    PropertiesSystem* properties_ = nullptr; // 保存前同步已接受场，随后统一读取属性表
    std::unordered_map<std::string, std::vector<std::string>> output_names_; // 输出范围 -> 已登记的名称，求解场与域积分点量统一归入 domain
    std::string output_directory_; // 结果输出目录
    std::unordered_map<std::string, std::unique_ptr<DomainOutput>> domain_outputs_; // 域输出名称 -> 对应的投影与文件输出对象
    std::unordered_map<std::string, SolveInformation> solve_information_; // 场名 -> 该场本次求解的统计信息
};

}
