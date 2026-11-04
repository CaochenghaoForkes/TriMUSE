#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "fem/fem_system.hpp"
#include "properties/derived/derived_model_manager.hpp"
#include "properties/external/external_model_manager.hpp"
#include "properties/material/material_model_manager.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class PropertiesSystem
{
public:
    using FieldMap = std::unordered_map<std::string, FieldView>; // 场名 -> 连续场或分域场的只读视图, 用于传入试探场

    explicit PropertiesSystem(const FemSystem& fem_system); // Global、Region、Zone 初始化为 std::nullopt IntegrationPoint 创建空数组 并预存储各个区域的体积

    void set_precomputed(const std::string& name, const ModelParameterValue& value); // assign 前填写已声明的全局预计算属性 precomputed.name, 并以 name 作为公共构造参数键

    void assign(int attribute, const std::string& model_name, const ModelParameterMap& parameters); // 为某个属性在指定 体积域/边界域 上配置一个计算模型 并把该位置已有的“计算完成”标记清掉

    void prepare_dependencies(); // 标记各个属性在各个区域依赖的场有哪些

    void mark_coupled(const std::string& field_name, const std::vector<std::string>& property_names); // 把指定属性中依赖某个求解场的位置标记为“参与耦合更新”，并沿依赖关系向上游传递这个标记

    void update_external(double time, double time_step); // 更新当前时间，然后检查各个外部属性，只在“尚未计算”或“依赖发生变化”时重新计算它们
 
    void update_derived(); // 更新派生属性；累积模型使用上一个已接受值和 external.time_step

    void update_material(); // 检查各个不依赖场的材料属性，只在“尚未计算”或“依赖发生变化”时重新计算它们

    void update_field_dependent_materials(const std::string& field_name, const FieldMap& trial_fields = {}, bool coupled_only = false); // 更新各个依赖场的材料属性

    void accept_current_state(); // 把当前全部属性数据复制一份，保存为“已接受状态”的快照

    const PropertiesFieldData& previous_field_data() const;

    bool coupled_with_field(const std::string& property_name, int region_attribute, const std::string& field_name) const; // 判断: 某个属性在指定区域，是否已被标记为参与指定求解场的耦合更新

    PropertiesFieldData& field_data();

    const PropertiesFieldData& field_data() const;

private:
    enum class PropertyType { Precomputed, External, Derived, Material }; // 属性属于哪一类 强类型枚举，可选值只有这四种
    enum class PropertyScope { Global, Region, Zone, IntegrationPoint }; // 属性在哪些位置存值 强类型枚举，可选值只有这四种

    struct PropertyInfo // 一项属性的说明记录
    {
        std::string name; // 属性名
        PropertyType type; 
        PropertyScope scope; 
        std::unordered_map<std::string, std::unordered_set<int>> field_regions; // 哪些区域的属性依赖某个场(简洁依赖也算依赖) 求解场名称 -> 一组区域编号
        std::unordered_map<std::string, std::unordered_set<int>> coupled_regions; // 哪些区域的属性参与某个场的耦合更新 求解场名称 -> 一组区域编号
    };

    PropertyInfo& property_info(const std::string& name); // 返回该 name 对应属性的 PropertyInfo

    const PropertyInfo& property_info(const std::string& name) const; // 返回该 name 对应属性的 PropertyInfo

    std::size_t property_index(const std::string& name) const; // 返回 name 对应属性在 properties_ 中的索引

    const std::vector<std::string>& model_required_states(const PropertyInfo& property, const std::string& model_name) const; // 返回计算 property 属性的 model_name 模型所需要的参数

    bool depends_on_field(const std::string& property_name, int region_attribute, const std::string& field_name) const; // 判断: 某个属性在指定区域，是否依赖求解场

    static PropertiesStatePoint make_state_point(PropertyScope scope, int attribute); // 根据属性的作用范围和位置编号，创建一个 PropertiesStatePoint 状态点对象，并设置它的区域/边界分区编号

    template <typename Manager>
    void evaluate_and_store(const Manager& manager, const std::string& name, const PropertiesStatePoint& point); // 调用管理器计算某项属性，把结果存到对应位置，并标记该位置已经计算完成 (这个方法只用于 Global、Region、Zone 属性)

    template <typename Manager>
    void evaluate_and_store_points(const Manager& manager, const PropertyInfo& property, int region_attribute, const std::vector<std::string>& required, const FieldMap& trial_fields = {}); // 计算某个属性在指定区域内全部积分点上的值，存入全域积分点数组，最后标记该区域已计算

    void load_required_states(PropertiesStatePoint& point, const std::vector<std::string>& names, std::optional<std::size_t> point_index = std::nullopt) const; // 给 point 装配需要的属性

    void load_model_states(PropertiesStatePoint& point, const PropertyInfo& property, int attribute, const std::vector<std::string>& required) const; // 加载当前依赖；若模型声明依赖自身，则改为加载已接受的旧值

    void load_previous_value(PropertiesStatePoint& point, const PropertyInfo& property, int attribute, std::optional<std::size_t> point_index = std::nullopt) const; // 读取同一位置上一个已接受时刻的属性值

    bool required_states_changed(int attribute, const std::vector<std::string>& required_states) const; // 判断: 某个区域或边界分区所依赖的属性，相比之前保存的快照，是否发生变化

    std::vector<PropertyInfo> properties_ = {
        {"precomputed.region_volume", PropertyType::Precomputed, PropertyScope::Region},
        {"precomputed.nuclide_name", PropertyType::Precomputed, PropertyScope::Global}, // 核素名称, 如 Cs137
        {"precomputed.nuclide_symbol", PropertyType::Precomputed, PropertyScope::Global}, // 元素符号, 如 Cs
        {"external.time", PropertyType::External, PropertyScope::Global},
        {"external.time_step", PropertyType::External, PropertyScope::Global}, // s, 本次推进的时间增量
        {"external.environment_temperature", PropertyType::External, PropertyScope::Zone},
        {"external.environment_nuclide_concentration", PropertyType::External, PropertyScope::Zone}, // atom/m³
        {"external.surface_nuclide_concentration", PropertyType::External, PropertyScope::Zone}, // atom/m³, 指定表面浓度
        {"external.nuclide_flux", PropertyType::External, PropertyScope::Zone}, // atom/(m²·s), 外流为正、流入为负
        {"external.surface_temperature", PropertyType::External, PropertyScope::Zone}, // K, 指定表面温度
        {"external.heat_flux", PropertyType::External, PropertyScope::Zone}, // W/m², 外流为正、流入为负
        {"external.burnup_fima_fraction", PropertyType::External, PropertyScope::Global},
        {"external.power", PropertyType::External, PropertyScope::Region},
        {"external.nuclide_birth_rate", PropertyType::External, PropertyScope::Global}, // atom/s, 全局核素产生率
        {"external.fast_neutron_flux", PropertyType::External, PropertyScope::Global}, // 1/(m²·s), 全局快中子通量
        {"external.uranium_fraction", PropertyType::External, PropertyScope::Region}, // 各区域占全局产生率的份额
        {"derived.heat_transfer_coef", PropertyType::Derived, PropertyScope::Zone},
        {"derived.mass_transfer_coef", PropertyType::Derived, PropertyScope::Zone}, // m/s
        {"derived.volumetric_power", PropertyType::Derived, PropertyScope::IntegrationPoint},
        {"derived.volumetric_nuclide_birth_rate", PropertyType::Derived, PropertyScope::IntegrationPoint}, // atom/(m³·s)
        {"derived.fast_neutron_fluence", PropertyType::Derived, PropertyScope::Global}, // 1/m², 累积快中子注量
        {"material.general.porosity", PropertyType::Material, PropertyScope::IntegrationPoint},
        {"material.general.mass_density", PropertyType::Material, PropertyScope::IntegrationPoint},
        {"material.thermal.conductivity", PropertyType::Material, PropertyScope::IntegrationPoint},
        {"material.thermal.specific_heat", PropertyType::Material, PropertyScope::IntegrationPoint},
        {"material.mechanics.young_modulus", PropertyType::Material, PropertyScope::IntegrationPoint}, // Pa, 杨氏模量
        {"material.mechanics.poisson_ratio", PropertyType::Material, PropertyScope::IntegrationPoint}, // 无量纲, 泊松比
        {"material.radioactivity.nuclide_diffusion_coef", PropertyType::Material, PropertyScope::IntegrationPoint} // m²/s
    };

    inline static const std::unordered_set<std::string> solution_field_states = {"temperature", "concentration", "displacement", "stress", "strain"}; // 求解场的状态名集合 inline: 静态成员变量直接在类定义中初始化，放在头文件里被多个 .cpp 包含，也不会产生重复定义错误；整个程序共享同一个实体

    const FemSystem& fem_system_;

    ModelParameterMap build_parameters_; // 初始化阶段设置的公共构造参数, 在最上层 assign 中合并给所有模型
    MaterialModelManager material_model_manager_; // 材料属性模型管理器
    ExternalModelManager external_model_manager_; // 外部强制属性模型管理器
    DerivedModelManager derived_model_manager_; // 外部强制属性的派生属性模型管理器
    PropertiesFieldData properties_field_data_;
    PropertiesFieldData previous_field_data_;
    std::unordered_map<std::string, std::unordered_map<int, std::string>> assigned_models_; // 属性名->位置名->模型名
    PropertiesFieldData required_state_snapshots_;
    std::unordered_map<std::string, std::unordered_set<int>> computed_locations_; // 属性名 -> 已经计算过的位置编号集合
};

}
