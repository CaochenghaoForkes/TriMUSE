#include "output/domain_statistics_output.hpp"
#include "common/value_reduction.hpp"
#include "properties/properties_system.hpp"

#include <filesystem>
#include <iomanip>
#include <utility>

namespace trimuse
{

DomainStatisticsOutput::DomainStatisticsOutput(FemSystem& fem_system, PropertiesSystem& properties, const std::string& name, const std::vector<std::string>& processes, const std::string& directory, const OutputLocationNames& region_names)
    :
    fem_system_(fem_system),
    properties_(properties),
    name_(name),
    region_names_(region_names)
{
    properties_.require_field(name_); // FEM 场为统计补充积分点采样，已有积分点属性直接读取
    std::filesystem::create_directories(directory);
    for (const auto& process : processes)
    {
        auto& file = files_[process];
        file.open(std::filesystem::path(directory) / (name_ + "." + process + ".csv"));
        file << std::setprecision(16);
    }
}

std::unordered_map<std::string, std::unique_ptr<DomainStatisticsOutput>> DomainStatisticsOutput::initialize(FemSystem& fem_system, PropertiesSystem& properties, const DomainStatisticsRequests& requests, const std::string& output_directory, const OutputLocationNames& region_names)
{
    std::unordered_map<std::string, std::unique_ptr<DomainStatisticsOutput>> outputs;
    const auto directory = std::filesystem::path(output_directory) / "scalar_properties";
    for (const auto& [name, processes] : requests)
    {
        if (!processes.empty())
        {
            outputs[name] = std::make_unique<DomainStatisticsOutput>(fem_system, properties, name, processes, directory.string(), region_names);
        }
    }
    return outputs;
}

void DomainStatisticsOutput::save(int cycle, double time, double time_step)
{
    const auto& data = properties_.field_data().domain_integration_point_values;
    const auto& samples = data.at(name_);
    const auto& point_weights = data.at("precomputed.initial_domain_quadrature_weight");
    std::vector<double> values, weights;
    std::vector<std::pair<std::string, double>> region_averages;
    for (const auto& [region, region_name] : region_names_)
    {
        std::vector<double> region_values, region_weights;
        for (std::size_t index : fem_system_.region_domain_point_indices(region))
        {
            if (samples[index]) // 未配置位置不参与统计，零值正常保留
            {
                region_values.push_back(std::get<double>(*samples[index]));
                region_weights.push_back(std::get<double>(*point_weights[index]));
            }
        }
        if (!region_values.empty())
        {
            if (files_.count("region_average"))
            {
                region_averages.emplace_back(region_name, weighted_average(region_values, region_weights));
            }
            values.insert(values.end(), region_values.begin(), region_values.end());
            weights.insert(weights.end(), region_weights.begin(), region_weights.end());
        }
    }
    for (auto& [process, file] : files_)
    {
        if (file.tellp() == 0) // 首次取到已计算结果后确定有效区域列
        {
            file << "cycle,time,time_step";
            if (process == "region_average")
            {
                for (const auto& [region_name, value] : region_averages)
                {
                    file << ',' << region_name;
                }
            }
            else
            {
                file << ",global";
            }
            file << '\n';
        }
        file << cycle << ',' << time << ',' << time_step;
        if (process == "region_average")
        {
            for (const auto& [region_name, value] : region_averages)
            {
                file << ',' << value;
            }
        }
        else
        {
            file << ',';
            if (!values.empty())
            {
                file << (process == "maximum" ? maximum(values) : process == "minimum" ? minimum(values) : weighted_average(values, weights));
            }
        }
        file << std::endl;
    }
}

}
