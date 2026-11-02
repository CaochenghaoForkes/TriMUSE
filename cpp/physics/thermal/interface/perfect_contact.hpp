#pragma once

#include "physics/thermal/interface/thermal_interface_model.hpp"

namespace trimuse
{

class ThermalPerfectContact final : public ThermalInterfaceModel // 处理当前这一块界面片: 找到两侧对应的界面自由度，将它们所在的分组合并，连接关系记录在 assembly 的 representative_ 中
{
public:
    explicit ThermalPerfectContact(const ModelParameterMap&) {} // 该模型不需要使用额外的参数表

    bool requires_region_field() const override { return false; } // 判断该模型是否强制要求有限元空间使用分域场,该模型对连续域自然成立,因此不要求
    
    void assemble(const InterfaceContext& context, ThermalInterfaceAssembly& assembly) const override; // 分域场：配对当前界面片两侧自由度，并在 representative_ 中合并分组 连续场：已共享界面自由度，无需额外连接
};

}
