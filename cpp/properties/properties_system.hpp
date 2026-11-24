#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "fem/fem_system.hpp"
#include "properties/assembly/assembly_model_manager.hpp"
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

    explicit PropertiesSystem(const FemSystem& fem_system, const std::vector<std::string>& field_names = {}, const ModelParameterMap& precomputed_parameters = {}); // 初始化属性存储并填入物理卡参数及几何缓存
    void set_precomputed(const std::string& name, const ModelParameterValue& value); // assign 前填写已声明的全局预计算属性 precomputed.name, 并以 name 作为公共构造参数键
    void assign(int attribute, const std::string& model_name, const ModelParameterMap& parameters); // 为某个属性在指定 体积域/边界域 上配置一个计算模型 并把该位置已有的“计算完成”标记清掉
    void prepare_dependencies(); // 补齐固定关系属性并标记场依赖
    void mark_coupled(const std::string& field_name, const std::vector<RequiredState>& property_names); // 把指定属性中依赖某个求解场的位置标记为“参与耦合更新”，并沿依赖关系向上游传递这个标记
    void update_field_independent_properties(double time, double time_step); // 更新时间和不依赖场的属性，最后保存依赖快照
    void update_field_dependent_properties(const std::string& field_name, const FieldMap& trial_fields = {}, bool coupled_only = false); // 采样本轮所需场一次，再依次更新依赖该场的派生、材料及组装属性
    void accept_current_state(); // 把当前全部属性数据复制一份，保存为“已接受状态”的快照
    const PropertiesFieldData& previous_field_data() const;
    bool coupled_with_field(const std::string& property_name, int region_attribute, const std::string& field_name) const; // 判断: 某个属性在指定区域，是否已被标记为参与指定求解场的耦合更新
    PropertiesFieldData& field_data();
    const PropertiesFieldData& field_data() const;

private:
    enum class PropertyType
    {
        Field,
        Precomputed,
        External,
        Derived,
        Material,
        Assembly
    }; // 属性类型
    enum class PropertyScope
    {
        Global,
        Region,
        Zone,
        DomainIntegrationPoint,
        FaceIntegrationPoint
    }; // 属性的作用位置

    struct PropertyInfo // 一项属性的说明记录
    {
        std::string name; // 属性名
        PropertyType type;
        PropertyScope scope;
        std::string fixed_model; // 非空时可由程序自动配置；为空时使用用户配置或程序预置值
        std::unordered_map<std::string, std::unordered_set<int>> field_regions; // 哪些区域的属性依赖某个场(简洁依赖也算依赖) 求解场名称 -> 一组区域编号
        std::unordered_map<std::string, std::unordered_set<int>> coupled_regions; // 哪些区域的属性参与某个场的耦合更新 求解场名称 -> 一组区域编号
    };

    void assign_field_properties(); // 按开发时声明的对应关系，为启用场配置各区域的固定关系属性
    PropertyInfo& property_info(const std::string& name); // 返回该 name 对应属性的 PropertyInfo
    const PropertyInfo& property_info(const std::string& name) const; // 返回该 name 对应属性的 PropertyInfo
    std::size_t property_index(const std::string& name) const; // 返回 name 对应属性在 properties_ 中的索引
    const std::vector<RequiredState>& model_required_states(const PropertyInfo& property, const std::string& model_name) const; // 返回计算 property 属性的 model_name 模型所需要的参数
    bool depends_on_field(const std::string& property_name, int region_attribute, const std::string& field_name) const; // 判断: 某个属性在指定区域，是否依赖求解场
    PropertiesStatePoint make_state_point(PropertyScope scope, int attribute) const; // 根据属性的作用范围和位置编号，创建一个 PropertiesStatePoint 状态点对象，并设置它的区域/边界分区编号
    void sample_required_fields(const FieldMap& trial_fields); // 将本轮所需场统一采样到全域积分点数组，优先读取试探场
    template <typename Manager>
    void evaluate_and_store(const Manager& manager, const std::string& name, const PropertiesStatePoint& point); // 调用管理器计算某项属性，把结果存到对应位置，并标记该位置已经计算完成 (这个方法只用于 Global、Region、Zone 属性)
    template <typename Manager>
    void evaluate_and_store_domain_points(const Manager& manager, const PropertyInfo& property, int region_attribute, const std::vector<RequiredState>& required); // 计算某个属性在指定区域内全部积分点上的值，存入全域积分点数组，最后标记该区域已计算
    template <typename Manager>
    void evaluate_property(const Manager& manager, const PropertyInfo& property, int location, const std::vector<RequiredState>& required); // 根据存储范围求值并保存属性
    template <typename Manager>
    void evaluate_and_store_face_points(const Manager& manager, const PropertyInfo& property, int attribute, const std::vector<RequiredState>& required); // 计算指定外边界或内部界面的各面求积点属性
    void load_required_states(PropertiesStatePoint& point, const std::vector<RequiredState>& names, std::optional<std::size_t> point_index = std::nullopt) const; // 给 point 装配需要的属性，point_index 为域或面积分点数组的全局下标
    void load_model_states(PropertiesStatePoint& point, const PropertyInfo& property, int attribute, const std::vector<RequiredState>& required, std::optional<std::size_t> point_index = std::nullopt) const; // 加载当前依赖；若模型声明依赖自身，则改为加载已接受的旧值
    void load_previous_value(PropertiesStatePoint& point, const PropertyInfo& property, int attribute, std::optional<std::size_t> point_index = std::nullopt) const; // 读取同一位置上一个已接受时刻的属性值，积分点统一使用全局下标
    bool required_states_changed(int attribute, const std::vector<RequiredState>& required_states) const; // 判断: 某个区域或边界分区所依赖的属性，相比之前保存的快照，是否发生变化

    std::vector<PropertyInfo> properties_ = {
        {"temperature", PropertyType::Field, PropertyScope::DomainIntegrationPoint}, // K，温度场的域积分点采样
        {"concentration", PropertyType::Field, PropertyScope::DomainIntegrationPoint}, // mol/m³，浓度场的域积分点采样
        {"displacement", PropertyType::Field, PropertyScope::DomainIntegrationPoint}, // m，位移场的域积分点采样
        {"stress", PropertyType::Field, PropertyScope::DomainIntegrationPoint}, // Pa，位移场恢复的应力状态
        {"strain", PropertyType::Field, PropertyScope::DomainIntegrationPoint}, // 无量纲，位移场恢复的应变状态
        {"precomputed.initial_region_volume", PropertyType::Precomputed, PropertyScope::Region},
        {"precomputed.initial_domain_quadrature_weight", PropertyType::Precomputed, PropertyScope::DomainIntegrationPoint}, // m³，域内求积点初始物理体积权重
        {"precomputed.initial_face_quadrature_weight", PropertyType::Precomputed, PropertyScope::FaceIntegrationPoint}, // m²，初始面求积点物理面积权重
        {"precomputed.geometry_center", PropertyType::Precomputed, PropertyScope::Global}, // m，颗粒中心坐标
        {"precomputed.thermal_expansion_reference_temperature", PropertyType::Precomputed, PropertyScope::Global}, // K, 来自力学卡的固定热膨胀参考温度
        {"precomputed.tensor_transform", PropertyType::Precomputed, PropertyScope::DomainIntegrationPoint}, // 六分量张量 local -> global 的 T
        {"precomputed.tensor_transform_inverse", PropertyType::Precomputed, PropertyScope::DomainIntegrationPoint}, // 六分量张量 global -> local 的 T^{-1}
        {"precomputed.gravity_acceleration", PropertyType::Precomputed, PropertyScope::Global}, // m/s²
        {"precomputed.nuclide_name", PropertyType::Precomputed, PropertyScope::Global}, // 核素名称, 如 Cs137
        {"precomputed.nuclide_symbol", PropertyType::Precomputed, PropertyScope::Global}, // 元素符号, 如 Cs
        {"precomputed.half_life", PropertyType::Precomputed, PropertyScope::Global}, // s, 核素半衰期
        {"precomputed.decay_constant", PropertyType::Precomputed, PropertyScope::Global}, // s^-1，ln(2)/half_life
        {"external.time", PropertyType::External, PropertyScope::Global},
        {"external.time_step", PropertyType::External, PropertyScope::Global}, // s, 本次推进的时间增量
        {"external.environment_temperature", PropertyType::External, PropertyScope::Zone},
        {"external.environment_nuclide_concentration", PropertyType::External, PropertyScope::Zone}, // mol/m³
        {"external.surface_nuclide_concentration", PropertyType::External, PropertyScope::Zone}, // mol/m³, 指定表面浓度
        {"external.nuclide_flux", PropertyType::External, PropertyScope::Zone}, // mol/(m²·s), 外流为正、流入为负
        {"external.surface_temperature", PropertyType::External, PropertyScope::Zone}, // K, 指定表面温度
        {"external.surface_traction", PropertyType::External, PropertyScope::Zone}, // Pa, 求解坐标系中的面力分量
        {"external.surface_displacement", PropertyType::External, PropertyScope::Zone}, // m, 求解坐标系中的位移分量
        {"external.surface_pressure", PropertyType::External, PropertyScope::Zone}, // Pa, 正值沿表面内法向加载
        {"external.heat_flux", PropertyType::External, PropertyScope::Zone}, // W/m², 外流为正、流入为负
        {"external.power", PropertyType::External, PropertyScope::Region},
        {"external.energy_per_fission", PropertyType::External, PropertyScope::Global}, // J，每次裂变的释热能量
        {"external.nuclide_birth_rate", PropertyType::External, PropertyScope::Global}, // mol/s, 全局核素产生率
        {"external.fast_neutron_flux", PropertyType::External, PropertyScope::Global}, // 1/(m²·s), 全局快中子通量
        {"external.initial_heavy_metal_inventory", PropertyType::External, PropertyScope::Region}, // mol，区域初始重金属物质的量
        {"external.uranium_fraction", PropertyType::External, PropertyScope::Region}, // 各区域占全局产生率的份额
        {"derived.region_volume", PropertyType::Derived, PropertyScope::Region}, // m³，当前区域体积
        {"derived.heat_transfer_coef", PropertyType::Derived, PropertyScope::Zone},
        {"derived.mass_transfer_coef", PropertyType::Derived, PropertyScope::Zone}, // m/s
        {"derived.average_volumetric_fission_rate", PropertyType::Derived, PropertyScope::Region}, // 1/(m³·s)，区域平均体积裂变率
        {"derived.fission_gas_production_rate", PropertyType::Derived, PropertyScope::Region}, // mol/s，区域裂变气体总产生率
        {"derived.burnup_fima_fraction", PropertyType::Derived, PropertyScope::Region}, // 0～1 的 FIMA 份额
        {"derived.volumetric_power", PropertyType::Derived, PropertyScope::DomainIntegrationPoint},
        {"derived.volumetric_nuclide_birth_rate", PropertyType::Derived, PropertyScope::DomainIntegrationPoint}, // mol/(m³·s)
        {"derived.irradiation_time", PropertyType::Derived, PropertyScope::Global}, // s，累计辐照时间
        {"derived.fast_neutron_fluence", PropertyType::Derived, PropertyScope::Global}, // 1/m², 累积快中子注量
        {"material.general.porosity", PropertyType::Material, PropertyScope::DomainIntegrationPoint},
        {"material.general.mass_density", PropertyType::Material, PropertyScope::DomainIntegrationPoint},
        {"material.general.crystallite_diameter", PropertyType::Material, PropertyScope::Region}, // m，各区域的微晶直径 Lc
        {"material.general.initial_bacon_anisotropy_factor", PropertyType::Material, PropertyScope::DomainIntegrationPoint}, // 无量纲，初始 BAF
        {"material.general.bacon_anisotropy_factor", PropertyType::Material, PropertyScope::DomainIntegrationPoint}, // 无量纲，当前 BAF
        {"material.thermal.conductivity", PropertyType::Material, PropertyScope::DomainIntegrationPoint},
        {"material.thermal.specific_heat", PropertyType::Material, PropertyScope::DomainIntegrationPoint},
        {"material.mechanics.young_modulus", PropertyType::Material, PropertyScope::DomainIntegrationPoint}, // Pa, [Er, Et]，径向、切向杨氏模量
        {"material.mechanics.poisson_ratio", PropertyType::Material, PropertyScope::DomainIntegrationPoint}, // 无量纲, [nu_rt, nu_tt]，径向受力的切向泊松比、切向之间的泊松比
        {"material.mechanics.shear_modulus", PropertyType::Material, PropertyScope::DomainIntegrationPoint}, // Pa, 标量 Grt；由用户在材料卡中指定模型
        {"material.mechanics.elasticity_tensor", PropertyType::Material, PropertyScope::DomainIntegrationPoint, "common.from_engineering_constants"}, // Pa
        {"material.mechanics.thermal_expansion_coef", PropertyType::Material, PropertyScope::DomainIntegrationPoint}, // K^-1, [alpha_r, alpha_t]，径向、切向热膨胀系数
        {"material.radioactivity.fission_gas_diffusion_coef", PropertyType::Material, PropertyScope::DomainIntegrationPoint}, // m²/s，晶粒尺度裂变气体扩散系数
        {"material.radioactivity.nuclide_diffusion_coef", PropertyType::Material, PropertyScope::DomainIntegrationPoint}, // m²/s
        {"assembly.homogenized_panama_opf", PropertyType::Assembly, PropertyScope::Region}, // 向量：[OPF, H, J, 辐照时长, 上一步平均温度]，基于区域体积平均温度
        {"assembly.local_panama_opf", PropertyType::Assembly, PropertyScope::DomainIntegrationPoint}, // 向量：[OPF, H, J, 辐照时长, 上一步温度]，UO₂ 的 PANAMA CO 模型
        {"assembly.cumulative_co_release", PropertyType::Assembly, PropertyScope::Region}, // mol，累计释放到颗粒内部自由空间的 CO 量
        {"assembly.local_booth_fission_gas_release_state", PropertyType::Assembly, PropertyScope::DomainIntegrationPoint}, // 向量：[累计释放量密度, 累计产生量密度, 释放份额, z₁, …, zM]
        {"assembly.homogenized_booth_fission_gas_release_state", PropertyType::Assembly, PropertyScope::Region}, // 区域均匀化状态：[累计释放量密度, 累计产生量密度, 释放份额, z₁, …, zM]
        {"assembly.cumulative_fission_gas_release", PropertyType::Assembly, PropertyScope::Region}, // mol，区域内累计释放的裂变气体总量
        {"assembly.internal_gas_pressure", PropertyType::Assembly, PropertyScope::Region}, // Pa，储气区域的内部气体压力
        {"assembly.interface_thermal_resistance", PropertyType::Assembly, PropertyScope::FaceIntegrationPoint} // m²·K/W，面积分点热阻
    };

    // 状态名 -> 负责更新它的求解场，模型取值仍使用原状态名
    // inline: 静态成员变量直接在类定义中初始化，放在头文件里被多个 .cpp 包含，也不会产生重复定义错误；整个程序共享同一个实体
    inline static const std::unordered_map<std::string, std::string> field_state_owners_ = {
        {"temperature", "temperature"},
        {"concentration", "concentration"},
        {"displacement", "displacement"},
        {"stress", "displacement"},
        {"strain", "displacement"}
    };
    // 场名 -> 程序内置且无需用户选择模型的固定关系属性
    inline static const std::unordered_map<std::string, std::vector<std::string>> field_required_properties_ = {
        {"temperature", {}},
        {"concentration", {}},
        {"displacement", {"material.mechanics.elasticity_tensor"}}
    };
    const FemSystem& fem_system_;
    std::vector<std::string> field_names_; // 本次启用的求解场，由外部传入
    std::unordered_set<std::string> required_field_names_; // 当前已配置的所有属性模型直接依赖的场名称的集合
    ModelParameterMap build_parameters_; // 初始化阶段设置的公共构造参数, 在最上层 assign 中合并给所有模型
    MaterialModelManager material_model_manager_; // 材料属性模型管理器
    AssemblyModelManager assembly_model_manager_; // 物理模块模型属性管理器
    ExternalModelManager external_model_manager_; // 外部强制属性模型管理器
    DerivedModelManager derived_model_manager_; // 外部强制属性的派生属性模型管理器
    PropertiesFieldData properties_field_data_;
    PropertiesFieldData previous_field_data_;
    std::unordered_map<std::string, std::unordered_map<int, std::string>> assigned_models_; // 属性名->位置名->模型名
    PropertiesFieldData required_state_snapshots_;
    std::unordered_map<std::string, std::unordered_set<int>> computed_locations_; // 属性名 -> 已经计算过的位置编号集合
};

}
