#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "fem/fem_system.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class PropertiesSystem;

struct OutputConfig
{
    std::string name;
    std::string directory;
};

class DomainOutput // 一个域标量的节点插值、L2 投影与 ParaView 时间序列输出
{
public:
    DomainOutput(FemSystem& fem_system, PropertiesSystem& properties, const std::string& field_name, const std::string& directory);
    static std::unordered_map<std::string, std::unique_ptr<DomainOutput>> initialize(FemSystem& fem_system, PropertiesSystem& properties, const std::vector<OutputConfig>& configs, const std::string& output_directory); // 按域输出清单批量创建输出对象
    void save(int cycle, double time, double time_step); // 判断数据来源，正式场插值输出，属性数组投影输出

private:
    void save_field(const FieldView& view, int cycle, double time, double time_step); // 将正式有限元场插值到输出节点并写出
    void save_quadrature_values(const std::vector<double>& values, const PropertiesFieldData::DomainIntegrationPointValues& weights, int cycle, double time, double time_step); // 将全域积分点数组逐单元投影到 L2 空间并写出
    void write_paraview(int cycle, double time, double time_step); // 设置当前帧的时间信息并保存 ParaView 文件

    FemSystem& fem_system_;
    PropertiesSystem& properties_;
    std::string field_name_;
    std::unique_ptr<mfem::L2_FECollection> collection_; // L2 形函数集合，各单元自由度独立
    std::unique_ptr<mfem::FiniteElementSpace> space_; // 原网格上的标量输出空间
    std::unique_ptr<mfem::GridFunction> values_; // 节点插值或 L2 投影得到的输出自由度值
    std::unique_ptr<mfem::ParaViewDataCollection> collection_output_; // 保存 ParaView 文件与时间序列
};

}
