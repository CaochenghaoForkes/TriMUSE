#pragma once

#include "physics/thermal/interface_heat_transfer/thermal_interface_heat_transfer_model.hpp"

namespace trimuse
{

class ThermalPerfectContact final : public ThermalInterfaceModel // 处理当前这一块界面片: 找到两侧对应的界面自由度，将它们所在的分组合并，连接关系记录在 assembly 的 representative_ 中
{
public:
    explicit ThermalPerfectContact(const ModelParameterMap&); // 该模型不需要使用额外的参数表
    bool is_perfect_contact(const InterfaceContext&) const override { return true; }
};

}
