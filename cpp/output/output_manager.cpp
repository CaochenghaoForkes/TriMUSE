#include "output/output_manager.hpp"
#include "properties/properties_system.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>

namespace trimuse
{

OutputManager::OutputManager(FemSystem& fem_system, const std::string& output_directory)
    :
    fem_system_(fem_system),
    output_directory_(output_directory)
{
}

void OutputManager::initialize(PropertiesSystem& properties, const OutputConfigs& configs, const LocationNames& location_names, const DomainStatisticsRequests& domain_statistics)
{
    properties_ = &properties;
    domain_outputs_ = DomainOutput::initialize(fem_system_, properties, configs.at("domain"), output_directory_);
    scalar_outputs_ = ScalarOutput::initialize(properties, configs.at("scalar"), output_directory_, location_names.at("region"), location_names.at("zone"));
    domain_statistics_outputs_ = DomainStatisticsOutput::initialize(fem_system_, properties, domain_statistics, output_directory_, location_names.at("region"));
    for (const auto& [range, entries] : configs)
    {
        auto& names = output_names_[range];
        for (const auto& config : entries)
        {
            names.push_back(config.name);
        }
    }
    for (const auto& config : configs.at("domain"))
    {
        solve_information_.try_emplace(config.name);
    }
    write_property_units(properties, configs, domain_statistics);
}

void OutputManager::write_property_units(const PropertiesSystem& properties, const OutputConfigs& configs, const DomainStatisticsRequests& domain_statistics) const
{
    std::map<std::string, std::string> units{{"cycle", "[1]"}, {"time", "[s]"}, {"time_step", "[s]"}};
    for (const auto& [range, entries] : configs)
    {
        for (const auto& config : entries)
        {
            units[config.name] = properties.unit(config.name);
        }
    }
    for (const auto& [name, processes] : domain_statistics)
    {
        for (const auto& process : processes)
        {
            units[name + "." + process] = properties.unit(name);
        }
    }
    std::filesystem::create_directories(output_directory_);
    std::ofstream file(std::filesystem::path(output_directory_) / "property_units.json");
    file << "{\n";
    std::size_t count = 0;
    for (const auto& [name, unit] : units)
    {
        file << "  " << std::quoted(name) << ": " << std::quoted(unit) << (++count < units.size() ? ",\n" : "\n");
    }
    file << "}\n";
}

void OutputManager::save(int cycle, double time, double time_step)
{
    properties_->sample_required_fields(); // 输出前同步已接受场，避免使用旧采样或试探值
    for (const std::string& name : output_names_.at("domain"))
    {
        domain_outputs_.at(name)->save(cycle, time, time_step);
    }
    for (const std::string& name : output_names_.at("scalar"))
    {
        scalar_outputs_.at(name)->save(cycle, time, time_step);
    }
    for (const auto& [name, output] : domain_statistics_outputs_)
    {
        output->save(cycle, time, time_step);
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
