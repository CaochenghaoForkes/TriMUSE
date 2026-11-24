#include "output/scalar_output.hpp"
#include "properties/properties_system.hpp"

#include <filesystem>
#include <iomanip>

namespace trimuse
{

ScalarOutput::ScalarOutput(const PropertiesSystem& properties, const std::string& name, const std::string& directory, const OutputLocationNames& region_names, const OutputLocationNames& zone_names) // 在初始化时，把 CSV 列名与属性表中对应值的存储位置绑定起来
{
    const auto& data = properties.field_data();
    const auto global = data.global_values.find(name);
    if (global != data.global_values.end())
    {
        columns_.push_back({"value", &global->second});
    }
    else
    {
        const bool regional = data.region_values.count(name) != 0;
        const auto& values = regional ? data.region_values.at(name) : data.zone_values.at(name);
        const auto& names = regional ? region_names : zone_names;
        for (const auto& [attribute, location_name] : names)
        {
            const auto& value = values.at(attribute);
            if (value || properties.has_assigned_model(name, attribute)) // 只列出已配置模型或已有预计算值的位置
            {
                columns_.push_back({location_name, &value}); // 保存指针后续 `save()` 就能通过指针读取最新值，不必再次查找属性名和位置编号
            }
        }
    }
    std::filesystem::create_directories(directory);
    file_.open(std::filesystem::path(directory) / (name + ".csv"));
    file_ << std::setprecision(16) << "cycle,time,time_step";
    for (const auto& column : columns_)
    {
        file_ << ',' << column.name;
    }
    file_ << '\n';
}

std::unordered_map<std::string, std::unique_ptr<ScalarOutput>> ScalarOutput::initialize(const PropertiesSystem& properties, const std::vector<OutputConfig>& configs, const std::string& output_directory, const OutputLocationNames& region_names, const OutputLocationNames& zone_names)
{
    std::unordered_map<std::string, std::unique_ptr<ScalarOutput>> outputs;
    for (const auto& config : configs)
    {
        const auto directory = std::filesystem::path(output_directory) / config.directory;
        outputs[config.name] = std::make_unique<ScalarOutput>(properties, config.name, directory.string(), region_names, zone_names);
    }
    return outputs;
}

void ScalarOutput::save(int cycle, double time, double time_step)
{
    file_ << cycle << ',' << time << ',' << time_step;
    for (const auto& column : columns_)
    {
        file_ << ',';
        if (*column.value)
        {
            file_ << std::get<double>(column.value->value());
        }
    }
    file_ << std::endl; // 刷新本行，运行过程中即可读取已保存的时间序列
}

}
