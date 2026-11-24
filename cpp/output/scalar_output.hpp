#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "output/output_config.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class PropertiesSystem;

class ScalarOutput // 一个 Global、Region 或 Zone 标量属性的 CSV 时间序列输出
{
public:
    ScalarOutput(const PropertiesSystem& properties, const std::string& name, const std::string& directory, const OutputLocationNames& region_names, const OutputLocationNames& zone_names);
    static std::unordered_map<std::string, std::unique_ptr<ScalarOutput>> initialize(const PropertiesSystem& properties, const std::vector<OutputConfig>& configs, const std::string& output_directory, const OutputLocationNames& region_names, const OutputLocationNames& zone_names);
    void save(int cycle, double time, double time_step); // 追加当前时刻的一行，时间和步长单位为秒

private:
    struct Column
    {
        std::string name; // 区域名
        const PropertiesFieldData::CachedValue* value; // 指向属性表中对应值的只读指针，不保存数值副本
    };

    std::vector<Column> columns_; // 所有位置组成的列表
    std::ofstream file_;
};

}
