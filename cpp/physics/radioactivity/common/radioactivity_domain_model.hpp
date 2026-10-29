#pragma once

#include "common/model_parameters.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct RadioactivityDomainConfig // 指定区域采用的域内模型及参数
{
    int region_attribute;
    std::string model_name;
    ModelParameterMap parameters;
};

class RadioactivityDomainModel // 模型返回弱式系数；管理器规定如何更新，积分器规定如何积分
{
public:
    virtual ~RadioactivityDomainModel() = default;

    // point 为原网格全域积分点编号：单元偏移 + 局部积分点编号。
    virtual double evaluate(const PropertiesFieldData& properties, std::size_t point) const = 0;

protected:
    static double scalar(const PropertiesFieldData& properties, const std::string& name, std::size_t point)
    {
        return std::get<double>(properties.integration_point_values.at(name)[point].value());
    }
};

}
