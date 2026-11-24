#include "output/output_manager.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

OutputManager::OutputManager(FemSystem& fem_system, const std::string& output_directory)
    :
    fem_system_(fem_system),
    output_directory_(output_directory)
{
}

void OutputManager::initialize(PropertiesSystem& properties, const OutputConfigs& configs)
{
    properties_ = &properties;
    domain_outputs_ = DomainOutput::initialize(fem_system_, properties, configs.at("domain"), output_directory_);
    auto& names = output_names_["domain"];
    for (const auto& config : configs.at("domain"))
    {
        names.push_back(config.name);
        solve_information_.try_emplace(config.name);
    }
}

void OutputManager::save(int cycle, double time, double time_step)
{
    properties_->sample_required_fields(); // 输出前同步已接受场，避免使用旧采样或试探值
    for (const std::string& name : output_names_.at("domain"))
    {
        domain_outputs_.at(name)->save(cycle, time, time_step);
    }
}

void OutputManager::clear_solve_information(const std::string& field_name) // 清空场对应的求解信息列表
{
    solve_information_[field_name].clear(); // 清空场对应的列表
}

void OutputManager::append_solve_information(const std::string& field_name, const std::string& name, double value) // 追加场对应的求解信息列表
{
    solve_information_[field_name].emplace_back(name, value); // 追加
}

const OutputManager::SolveInformation& OutputManager::solve_information(const std::string& field_name) const // 读取场对应的求解信息列表
{
    return solve_information_.at(field_name); // 读取
}

}
