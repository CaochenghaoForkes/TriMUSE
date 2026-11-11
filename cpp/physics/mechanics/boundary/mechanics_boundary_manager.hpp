#pragma once

#include <map>

#include "common/model_registry.hpp"
#include "physics/mechanics/boundary/mechanics_boundary_model.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

class MechanicsBoundaryManager // 按外表面分区管理边界类型、数值和组装变化标记
{
public:
    explicit MechanicsBoundaryManager(const std::vector<MechanicsBoundaryConfig>& configs = {}); // 注册与安排各 Zone 边界模型

    void update(const PropertiesFieldData& properties); // 更新各 Zone 边界系数与 dirty

    void mark_coupled(PropertiesSystem& properties, const std::string& field) const; // 标记由于边界而与位移场耦合的属性

    bool coupled(const PropertiesSystem& properties, const std::string& field) const; // 判断属性是否因边界值而与位移场耦合

    bool required_no_change(int zone) const; // 根据 dirty 判断指定区域的系数是否与最近一次组装时一致(dirty是否为False)，因而可以复用已有矩阵或向量

    void assembled(int zone); // 将 zone 处的 dirty 记为 false

    std::vector<int> zones() const;  // 返回当前管理器中已经配置模型的所有边界区域编号

    const MechanicsBoundaryValue& values(int zone) const; // 指定边界区域 返回不同模型的计算结果 面力、位移保存求解坐标系中的向量，压力保存标量；每个分区只有一种边界条件

    MechanicsBoundaryType type(int zone) const; // 指定边界区域的模型的返回类型 三类力学边界 0-Traction 面力向量 1-Displacement 各向位移 2-Pressure 法向压力

    bool essential(int zone) const; // 判断指定边界区域是否为给定位移的本质边界

private:
    struct Assignment
    {
        std::unique_ptr<MechanicsBoundaryModel> model;
        std::vector<std::string> required;
        MechanicsBoundaryValue value;
        bool dirty = true; // 首次必须处理，之后保持变化标记直到 assembled() 清除
    };

    ModelRegistry<MechanicsBoundaryModel> registry_;
    std::map<int, Assignment> models_;
};

}
