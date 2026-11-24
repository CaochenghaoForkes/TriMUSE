#include "properties/properties_system.hpp"
#include "properties/precomputed/precomputed_manager.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace trimuse
{

PropertiesSystem::PropertiesSystem(const FemSystem& fem_system, const std::vector<std::string>& field_names, const ModelParameterMap& precomputed_parameters)
    :
    fem_system_(fem_system),
    field_names_(field_names)
{
    for (PropertyInfo& property : properties_)
    {
        const auto locations = property.scope == PropertyScope::Global ? std::vector<int>{0} : (property.scope == PropertyScope::Region || property.scope == PropertyScope::DomainIntegrationPoint ? fem_system_.mesh_data().region_attributes() : fem_system_.mesh_data().boundary_attributes());
        for (int location : locations)
        {   // 为每项属性的各个有效位置创建一份 PropertyUpdateState 其中 changed_at = 0; evaluated_at = 0;
            property.updates.emplace(location, PropertyUpdateState{});
        }
        if (property.scope == PropertyScope::Global)
        {
            properties_field_data_.global_values.emplace(property.name, std::nullopt); // 初始化各 Global 属性值为 std::nullopt
        }
        else if (property.scope == PropertyScope::Region || property.scope == PropertyScope::Zone)
        {
            auto& properties = property.scope == PropertyScope::Region ? properties_field_data_.region_values : properties_field_data_.zone_values;
            const auto attributes = property.scope == PropertyScope::Region ? fem_system_.mesh_data().region_attributes() : fem_system_.mesh_data().boundary_attributes();
            auto& values = properties[property.name];
            for (const int attribute : attributes)
            {
                values.emplace(attribute, std::nullopt); // 初始化各 Region/Zone 在各个区域的属性值为 std::nullopt
            }
        }
        else if (property.scope == PropertyScope::FaceIntegrationPoint)
        {
            properties_field_data_.face_integration_point_values.emplace(property.name, PropertiesFieldData::FaceIntegrationPointValues{});
        }
        else
        {
            properties_field_data_.domain_integration_point_values.emplace(property.name, PropertiesFieldData::DomainIntegrationPointValues{}); // 初始化各 DomainIntegrationPoint 属性的域求积点数组为空
        }
    }
    PrecomputedManager::initialize_precomputed(*this, fem_system_, field_names_, precomputed_parameters);
    assign_field_properties(); // 初始化时按启用场安排固定属性模型
}

void PropertiesSystem::set_precomputed(const std::string& name, const ModelParameterValue& value) // 为已声明的全局预计算属性填写值，并把它保存为所有模型共用的构造参数
{
    const std::string property_name = "precomputed." + name;
    build_parameters_[name] = value;
    PropertiesFieldData::CachedValue value_to_store = std::visit([](const auto& item) -> StateValue { return item; }, value); // 取出 value 内部实际保存的数据，将它转为属性系统使用的 StateValue，再装入 CachedValue
    auto& stored = properties_field_data_.global_values.at(property_name); // 取得这项全局属性的存储位置，`stored` 是引用，修改它就是修改属性表
    const bool changed = stored != value_to_store; // 在覆盖之前比较新旧值，判断是否发生变化
    stored = std::move(value_to_store); // 将新值移动到属性表中，避免不必要的复制
    record_update(property_name, 0, changed);
}

PropertiesSystem::PropertyInfo& PropertiesSystem::property_info(const std::string& name) // 返回该 name 对应属性的 PropertyInfo
{
    return properties_[property_index(name)];
}

const PropertiesSystem::PropertyInfo& PropertiesSystem::property_info(const std::string& name) const // 返回该 name 对应属性的 PropertyInfo
{
    return properties_[property_index(name)];
}

std::size_t PropertiesSystem::property_index(const std::string& name) const // 返回 name 对应属性在 properties_ 中的索引
{
    // std::find_if 在 [begin, end) 范围内查找第一个满足条件的元素
    // [&name]：按引用捕获外部变量 name
    // 对容器中每个 PropertyInfo 元素，判断其 name 成员是否与要找的名字相等
    const auto property = std::find_if(properties_.begin(), properties_.end(), [&name](const PropertyInfo& item) { return item.name == name; }); // 查找成功返回指向该元素的迭代器；失败则返回 end()
    return static_cast<std::size_t>(property - properties_.begin()); // 两个迭代器相减得到偏移量 (即 properties 中该 name 对应的的下标)
}

const std::vector<RequiredState>& PropertiesSystem::model_required_states(const PropertyInfo& property, const std::string& model_name) const // 返回计算 property 属性的 model_name 模型所需要的参数
{
    if (property.type == PropertyType::External)
    {
        return external_model_manager_.required_states(property.name, model_name);
    }
    if (property.type == PropertyType::Derived)
    {
        return derived_model_manager_.required_states(property.name, model_name);
    }
    if (property.type == PropertyType::Assembly)
    {
        return assembly_model_manager_.required_states(property.name, model_name);
    }
    return material_model_manager_.required_states(property.name, model_name);
}

void PropertiesSystem::assign(int attribute, const std::string& model_name, const ModelParameterMap& parameters) // 为某个属性在指定 体积域/边界域 上配置一个计算模型 并把该位置已有的“计算完成”标记清掉
{
    // 模型名固定为 namespace.model, 如 model_name = "material.thermal.conductivity.common.time_series";
    const auto model_separator = model_name.rfind('.'); // rfind('.') 从右往左找最后一个点的位置
    const auto property_end = model_separator == std::string::npos ? std::string::npos : model_name.rfind('.', model_separator - 1); // std::string::npos 是字符串查找中表示“没找到”的特殊值 如果没找到最后一个点就直接返回；rfind(字符, 起始位置) 表示从指定位置开始，向左寻找该字符，起始位置也会参与检查 如果找到了最后一个点 就从 model_separator - 1 的位置(最后一个点的前一个字符位置) 开始找倒数第二个 .
    const std::string property_name = model_name.substr(0, property_end); // 截取倒数第二个点之前的内容 如 material.thermal.conductivity
    const std::string specific_model_name = model_name.substr(property_end + 1); // 截取倒数第二个点之后点全部内容 如 common.time_series
    const PropertyInfo& property = property_info(property_name); // 查询该属性的分类信息
    ModelParameterMap model_parameters = parameters;
    for (const auto& [name, value] : build_parameters_) // 公共构造参数统一传给各类模型; 同名时以预计算值为准
    {
        model_parameters[name] = value;
    }
    if (property.type == PropertyType::External)
    {
        external_model_manager_.assign(property_name, attribute, specific_model_name, model_parameters);
    }
    else if (property.type == PropertyType::Derived)
    {
        derived_model_manager_.assign(property_name, attribute, specific_model_name, model_parameters);
    }
    else if (property.type == PropertyType::Material)
    {
        material_model_manager_.assign(attribute, property_name, specific_model_name, model_parameters);
    }
    else if (property.type == PropertyType::Assembly)
    {
        assembly_model_manager_.assign(property_name, attribute, specific_model_name, model_parameters);
    }
    else
    {
        throw std::invalid_argument("预计算属性不接受模型配置: " + property_name);
    }
    assigned_models_[property_name][attribute] = specific_model_name; // 记录模型分配关系 属性名->位置编号->模型名 如 assigned_models_["material.thermal.conductivity"][3] = "common.constant";
    const int location = property.scope == PropertyScope::Global ? 0 : attribute; // 统一用于缓存管理的位置编号 全局属性固定为 0 其他属性为使用传入的区域或边界分区编号
    computed_locations_[property_name].erase(location); // 这里配置了新模型 之前的计算结果不能再当作已经更新好的结果使用 需要重新计算
}

void PropertiesSystem::assign_field_properties()
{
    for (const std::string& field : field_names_)
        for (const std::string& name : field_required_properties_.at(field))
        {
            const PropertyInfo& property = property_info(name);
            const std::string model = name + "." + property.fixed_model;
            const auto regions = property.scope == PropertyScope::Global ? std::vector<int>{0} : fem_system_.mesh_data().region_attributes();
            for (int region : regions)
            {
                assign(region, model, {});
            }
        }
}

bool PropertiesSystem::has_assigned_model(const std::string& name, int location) const
{
    const auto assigned = assigned_models_.find(name);
    return assigned != assigned_models_.end() && assigned->second.count(location) != 0;
}

void PropertiesSystem::prepare_dependencies() // 标记各个属性在各个区域依赖的场有哪些
{
    required_field_and_result_names_.clear();
    for (PropertyInfo& property : properties_)
    {
        property.field_regions.clear(); // 清空旧依赖标记
        property.coupled_regions.clear(); // 清空旧耦合标记
    }
    for (std::size_t index = 0; index < properties_.size(); ++index) // 按照属性表顺序逐项分析
    {
        PropertyInfo& property = properties_[index];
        const auto assigned = assigned_models_.find(property.name); // 模型分配表 属性名->区域编号->模型名
        if (assigned == assigned_models_.end())
        {
            continue; // 没有给该属性配置模型，就跳过，继续分析下一个属性
        }
        for (const auto& [region_attribute, model_name] : assigned->second) // 结构化绑定把每一项拆成 位置编号(若是 Region 属性对应的就是 region_attribute 若是 Zone 属性对应的其实是 zone_attribute )->模型名
        {
            const int location = property.scope == PropertyScope::Global ? 0 : region_attribute; // 全局属性位置用 0
            for (const auto& dependency : model_required_states(property, model_name)) // 遍历模型所需状态量
            {
                const std::string& required_name = dependency.name;
                if (required_name == property.name)
                {
                    continue; // 任意类型的属性依赖自身时，读取已接受的旧值，不构成当前时刻的依赖环
                }
                if (field_state_owners_.count(required_name) != 0) // 检查依赖是否为求解场或由求解场恢复的状态量
                {
                    if (property.type == PropertyType::External)
                    {
                        throw std::logic_error("External 属性不允许依赖求解场: " + property.name);
                    }
                    require_field(required_name);
                    property.field_regions[field_state_owners_.at(required_name)].insert(location); // 按所属求解场登记依赖，应力标量随 displacement 更新
                    continue;
                }
                const std::size_t upstream_index = property_index(required_name); // 如果依赖的是其他属性 还要检查该属性(称之为上游属性) 首先查询该上游属性在 properties_ 中的下标
                const PropertyInfo& upstream = properties_[upstream_index]; // 取得上游属性的说明记录
                if (upstream.type > property.type || (upstream.type == property.type && upstream_index >= index)) // 检查依赖方向是否合法 上游类型必须先于下游类型；同类属性的上游必须排在当前属性之前
                {
                    throw std::logic_error("属性依赖必须遵守 external → derived → material → assembly 及类内先后顺序: " + property.name);
                }
                if (dependency.reference) // 当前依赖声明为完整属性引用
                {
                    for (const auto& [field_name, regions] : upstream.field_regions) // upstream.field_regions [场名:上游属性中依赖这个场的位置编号集合]
                    {
                        if (!regions.empty()) // 只要集合非空，就说明上游依赖属性有至少一个位置依赖这个场，则当前属性的当前位置也依赖这个场（因为当前属性引用了完整的上游数组）
                        {
                            property.field_regions[field_name].insert(location);
                        }
                    }
                    continue;
                }
                const int upstream_location = upstream.scope == PropertyScope::Global ? 0 : region_attribute; // 上游若是全局属性就检查位置0 否则检查与当前模型配置编号相同的位置
                for (const auto& [field_name, regions] : upstream.field_regions) // 遍历上游属性已经整理好的场依赖记录
                {
                    if (regions.count(upstream_location) != 0) // 检查: 上游属性在当前需要的位置，是否依赖这个求解场
                    {
                        property.field_regions[field_name].insert(location); // 如果是: 那么当前属性也间接依赖这个场，把标记传递过来
                    }
                }
            }
        }
    }
}

void PropertiesSystem::mark_coupled(const std::string& field_name, const std::vector<RequiredState>& property_names) // 把指定属性中依赖某个求解场的位置标记为“参与耦合更新”，并沿依赖关系向上游传递这个标记
{
    for (const auto& dependency : property_names) // 逐个处理传入的属性名 (某个场依赖的某些属性[不代表属性一定依赖场])
    {
        const std::string& name = dependency.name;
        const auto assigned = assigned_models_.find(name); // 查找该属性的模型分配记录 位置编号 -> 模型名
        if (assigned == assigned_models_.end())
        {
            continue; // 如果该属性没有配置模型，跳过它
        }
        for (const auto& assignment : assigned->second) // 遍历该属性在各位置中的配置
        {
            PropertyInfo& property = property_info(name); // 取得该属性说明记录
            const int location = property.scope == PropertyScope::Global ? 0 : assignment.first; // Global 位置为0 Zone 或 Region 直接为 assignment.first
            if (depends_on_field(name, location, field_name)) // 检查该属性在这个位置，是否直接或间接依赖指定场
            {
                property.coupled_regions[field_name].insert(location); // 将满足条件的位置纳入耦合更新
            }
        }
    }
    for (auto property = properties_.rbegin(); property != properties_.rend(); ++property) // 再次反向遍历参数表 把耦合标记传递给必要的上游属性
    {
        const auto assigned = assigned_models_.find(property->name); // 查询当前属性的模型分配记录
        if (assigned == assigned_models_.end())
        {
            continue; // 如果该属性没有配置模型，跳过它
        }
        for (const auto& [region_attribute, model_name] : assigned->second) // 遍历当前属性在每个位置配置的模型
        {
            const int location = property->scope == PropertyScope::Global ? 0 : region_attribute; // Global 位置为0 Zone 或 Region 直接为 assignment.first
            if (!coupled_with_field(property->name, location, field_name)) // 检查该属性在这个位置，是否被标记耦合指定场
            {
                continue; // 如果当前属性在该位置没有被标记为参与这个场的耦合，就不需要从这里向上游传播，直接跳过
            }
            for (const auto& dependency : model_required_states(*property, model_name)) // 取得当前模型的依赖列表，逐项检查
            {
                const std::string& required_name = dependency.name;
                if (required_name == property->name) // 历史值不参与当前时刻的耦合依赖传播
                {
                    continue;
                }
                if (field_state_owners_.count(required_name) != 0)
                {
                    continue; // 状态量是依赖传播的终点，保留原状态名供后续取值
                }
                PropertyInfo& upstream = property_info(required_name); // 取得上游属性记录
                if (dependency.reference) // 当前依赖为完整属性引用
                {
                    const auto regions = upstream.field_regions.find(field_name); // 查找上游属性中，哪些位置依赖这个场
                    if (regions != upstream.field_regions.end()) // 若上游存在这个场的依赖记录
                    {
                        upstream.coupled_regions[field_name].insert(regions->second.begin(), regions->second.end()); // 就把这些位置全部加入上游的耦合位置集合
                    }
                    continue;
                }
                const int upstream_location = upstream.scope == PropertyScope::Global ? 0 : region_attribute; // 确定上游对应的位置：全局属性使用 0，其他使用相同配置编号
                if (depends_on_field(required_name, upstream_location, field_name)) // 检查这个上游属性是否也依赖当前场 (下游参与温度耦合，不代表它依赖的所有上游属性都需要随温度更新)
                {
                    upstream.coupled_regions[field_name].insert(upstream_location); // 把确实依赖当前场的上游位置，也标记为参与耦合更新
                }
            }
        }
    }
}

PropertiesStatePoint PropertiesSystem::make_state_point(PropertyScope scope, int attribute) const // 根据属性的作用范围和位置编号，创建一个 PropertiesStatePoint 状态点对象，并设置它的区域/边界分区编号
{
    PropertiesStatePoint point;
    point.region_attribute = scope == PropertyScope::Region ? attribute : 0; // 如果是区域属性，就设置区域编号；否则设为 0
    point.zone_attribute = scope == PropertyScope::Zone ? attribute : 0; // 如果是边界分区属性，就设置边界分区编号；否则设为 0
    if (scope == PropertyScope::Region)
    {
        point.region_domain_point_indices = &fem_system_.region_domain_point_indices(attribute);
    }
    return point;
}

template <typename Manager>
void PropertiesSystem::evaluate_and_store(const Manager& manager, const std::string& name, const PropertiesStatePoint& point) // 调用管理器计算某项属性，把结果存到对应位置，并标记该位置已经计算完成 (这个方法只用于 Global、Region、Zone 属性)
{
    PropertiesFieldData::CachedValue value = manager.evaluate(name, point);
    const PropertyScope scope = property_info(name).scope;
    const int location = scope == PropertyScope::Zone ? point.zone_attribute : point.region_attribute;
    auto& stored = scope == PropertyScope::Global ? properties_field_data_.global_values.at(name) : (scope == PropertyScope::Zone ? properties_field_data_.zone_values.at(name).at(location) : properties_field_data_.region_values.at(name).at(location));
    const bool changed = stored != value;
    stored = std::move(value);
    record_update(name, location, changed);
}

void PropertiesSystem::update_field_independent_properties(double time, double time_step) // 更新时间和不依赖场的属性，按变化序号复用结果
{
    for (const auto& [name, value] : {std::pair{"external.time", time}, std::pair{"external.time_step", time_step}})
    {
        auto& stored = properties_field_data_.global_values.at(name);
        const bool changed = stored != PropertiesFieldData::CachedValue{value};
        stored = StateValue{value};
        record_update(name, 0, changed);
    }
    for (const PropertyInfo& property : properties_) // 按属性表顺序更新 External、Derived、Material、Assembly
    {
        if (property.type == PropertyType::Field || property.type == PropertyType::Precomputed)
        {
            continue;
        }
        const std::string& name = property.name;
        const auto assigned = assigned_models_.find(name);
        if (assigned == assigned_models_.end())
        {
            continue;
        }
        for (const auto& [attribute, model_name] : assigned->second)
        {
            const int location = property.scope == PropertyScope::Global ? 0 : attribute;
            // 如果当前属性不是 External，且它在当前 location 依赖任意一个求解场，就跳过常规更新，留给场依赖更新入口处理
            if (property.type != PropertyType::External && std::any_of(property.field_regions.begin(), property.field_regions.end(), [location](const auto& item) { return item.second.count(location) != 0; }))
            {
                continue; // 当前属性在该位置依赖场，交给场依赖更新入口处理
            }
            const auto& required = model_required_states(property, model_name);
            const bool uses_history = std::find(required.begin(), required.end(), name) != required.end();
            if (!uses_history && computed_locations_[name].count(location) != 0 && !required_states_changed(property, location, required))
            {
                continue;
            }
            if (property.type == PropertyType::External)
            {
                evaluate_property(external_model_manager_, property, location, required);
            }
            else if (property.type == PropertyType::Derived)
            {
                evaluate_property(derived_model_manager_, property, location, required);
            }
            else if (property.type == PropertyType::Material)
            {
                evaluate_property(material_model_manager_, property, location, required);
            }
            else
            {
                evaluate_property(assembly_model_manager_, property, location, required);
            }
        }
    }
}

void PropertiesSystem::load_required_states(PropertiesStatePoint& point, const std::vector<RequiredState>& names, std::optional<std::size_t> point_index) const // 给 point 装配需要的属性
{
    for (const auto& dependency : names)
    {
        const std::string& name = dependency.name;
        const PropertyScope scope = property_info(name).scope;
        if (dependency.reference)
        {
            if (scope == PropertyScope::Global)
            {
                point.references[name] = &properties_field_data_.global_values.at(name);
            }
            else if (scope == PropertyScope::Region)
            {
                point.references[name] = &properties_field_data_.region_values.at(name);
            }
            else if (scope == PropertyScope::Zone)
            {
                point.references[name] = &properties_field_data_.zone_values.at(name);
            }
            else if (scope == PropertyScope::FaceIntegrationPoint)
            {
                point.references[name] = &properties_field_data_.face_integration_point_values.at(name);
            }
            else
            {
                point.references[name] = &properties_field_data_.domain_integration_point_values.at(name);
            }
            continue;
        }
        if (scope == PropertyScope::Global)
        {
            point.values[name] = properties_field_data_.global_values.at(name).value();
        }
        else if (scope == PropertyScope::Region)
        {
            point.values[name] = properties_field_data_.region_values.at(name).at(point.region_attribute).value();
        }
        else if (scope == PropertyScope::Zone)
        {
            point.values[name] = properties_field_data_.zone_values.at(name).at(point.zone_attribute).value();
        }
        else
        {
            const auto& values = scope == PropertyScope::FaceIntegrationPoint ? properties_field_data_.face_integration_point_values.at(name) : properties_field_data_.domain_integration_point_values.at(name);
            point.values[name] = values[point_index.value()].value();
        }
    }
}

void PropertiesSystem::load_model_states(PropertiesStatePoint& point, const PropertyInfo& property, int attribute, const std::vector<RequiredState>& required, std::optional<std::size_t> point_index) const // 为 point 加载模型计算依赖的属性，包括可能的历史值
{
    if (std::find(required.begin(), required.end(), property.name) == required.end()) // 如果不包含历史值
    {
        load_required_states(point, required, point_index);
        return;
    }
    load_previous_value(point, property, attribute, point_index); // 从旧属性表查找历史值并添加到 point 中
    std::vector<RequiredState> current_required;
    current_required.reserve(required.size() - 1);
    for (const auto& dependency : required)
    {
        const std::string& name = dependency.name;
        if (name != property.name)
        {
            current_required.push_back(dependency);
        }
    }
    load_required_states(point, current_required, point_index);
}

void PropertiesSystem::load_previous_value(PropertiesStatePoint& point, const PropertyInfo& property, int attribute, std::optional<std::size_t> point_index) const // 从已接受快照读取同一属性、同一位置的旧值
{
    const std::string& name = property.name;
    if (property.scope == PropertyScope::Global)
    {
        const auto value = previous_field_data_.global_values.find(name);
        if (value != previous_field_data_.global_values.end())
        {
            point.previous_value = value->second;
        }
    }
    else if (property.scope == PropertyScope::Region || property.scope == PropertyScope::Zone)
    {
        const auto& values = property.scope == PropertyScope::Region ? previous_field_data_.region_values : previous_field_data_.zone_values;
        const auto property_values = values.find(name);
        if (property_values != values.end())
        {
            const auto value = property_values->second.find(attribute);
            if (value != property_values->second.end())
            {
                point.previous_value = value->second;
            }
        }
    }
    else
    {
        const auto& arrays = property.scope == PropertyScope::FaceIntegrationPoint ? previous_field_data_.face_integration_point_values : previous_field_data_.domain_integration_point_values;
        const auto values = arrays.find(name);
        if (values != arrays.end() && point_index.value() < values->second.size())
        {
            point.previous_value = values->second[point_index.value()];
        }
    }
}

void PropertiesSystem::record_update(const std::string& name, int location, bool changed) // 某项属性在某个位置完成求值后，记录它是否变化，以及本次求值对应的序号
{
    // changed_at 告诉下游“我的结果何时变了”，`evaluated_at` 告诉自己“我已经处理到哪个序号了”
    auto& property = property_info(name); // 取得这项属性的说明记录，使用引用，后续修改直接作用于原对象
    auto& update = property.updates.at(location); // 取得当前 location 对应的更新记录，包含该位置的 changed_at 和 evaluated_at
    if (changed) // 如果输出发生了变化
    {
        update.changed_at = ++change_sequence_; // 1.先将全局变化序号加一 2.再记录当前位置的最新变化序号
        property.changed_at = change_sequence_; // 3.记录整项属性的最新变化序号，供 reference=true 的完整引用判断
    }
    update.evaluated_at = change_sequence_; // 无论输出是否变化，都记录本次求值完成时的全局序号，表示：此前的上游变化，本次已经考虑过了
    computed_locations_[name].insert(location); // 登记这个位置已经计算过，后续就可以根据依赖的变化序号决定是否复用结果
}

bool PropertiesSystem::required_states_changed(const PropertyInfo& property, int location, const std::vector<RequiredState>& required_states) const
{
    const std::size_t evaluated_at = property.updates.at(location).evaluated_at;
    for (const auto& dependency : required_states)
    {
        const auto& upstream = property_info(dependency.name);
        const int upstream_location = upstream.scope == PropertyScope::Global ? 0 : location;
        const std::size_t changed_at = dependency.reference ? upstream.changed_at : upstream.updates.at(upstream_location).changed_at;
        if (changed_at > evaluated_at)
        {
            return true;
        }
    }
    return false;
}

template <typename Manager>
void PropertiesSystem::evaluate_and_store_domain_points(const Manager& manager, const PropertyInfo& property, int region_attribute, const std::vector<RequiredState>& required) // 计算某个属性在指定区域内全部积分点上的值，存入全域积分点数组，最后标记该区域已计算
{
    const mfem::Mesh& mesh = fem_system_.mesh_data().mfem_mesh(); // 取得原网格
    const bool uses_history = std::find(required.begin(), required.end(), property.name) != required.end();
    std::vector<RequiredState> common_required; // 非积分点属性，如时间、区域属性
    std::vector<RequiredState> point_required; // 积分点属性，如积分点孔隙率
    for (const auto& dependency : required)
    {
        const std::string& required_name = dependency.name;
        if (required_name == property.name) // 本属性的旧值在各积分点单独加载
        {
            continue;
        }
        (!dependency.reference && property_info(required_name).scope == PropertyScope::DomainIntegrationPoint ? point_required : common_required).push_back(dependency); // 积分点属性加入 point_required 其他属性加入 common_required
    }
    PropertiesStatePoint common_point; // 建立公共状态点，设置当前区域编号，并加载时间、区域属性等依赖
    common_point.region_attribute = region_attribute;
    load_required_states(common_point, common_required);
    auto& values = properties_field_data_.domain_integration_point_values.at(property.name); // 取得当前属性的全域积分点值数组的引用，后面的赋值直接修改缓存
    values.resize(fem_system_.domain_quadrature_point_count()); // 把数组长度设置为全域积分点总数
    bool changed = false;
    for (const int element : fem_system_.mesh_data().region_elements(region_attribute)) // 遍历区域内单元
    {
        const mfem::Geometry::Type geometry = mesh.GetElementBaseGeometry(element);
        const mfem::IntegrationRule* rule = fem_system_.domain_quadrature_rule(geometry);
        const auto& positions = fem_system_.domain_quadrature_positions(element); // 取得之前缓存的、该单元全部积分点的实际空间坐标，避免重新计算坐标
        const std::size_t offset = fem_system_.domain_quadrature_offset(element); // 取得该单元积分点在全域连续数组中的起始下标
        for (int point_index = 0; point_index < rule->GetNPoints(); ++point_index) // 遍历当前单元的积分点
        {
            const auto& position = positions[point_index]; // 读取这个积分点已经缓存的实际坐标
            const std::size_t index = offset + point_index; // 将局部积分点编号转换为全域积分点下标

            PropertiesStatePoint point; // 构造当前积分点的模型输入
            point.region_attribute = region_attribute;
            point.x = position[0];
            point.y = position[1];
            point.z = position[2];
            point.values = common_point.values;
            point.references = common_point.references;
            if (uses_history)
            {
                load_previous_value(point, property, region_attribute, index);
            }
            load_required_states(point, point_required, index); // 按全域积分点下标 index，读取该点需要的积分点属性
            PropertiesFieldData::CachedValue value = manager.evaluate(property.name, point);
            changed = changed || values[index] != value;
            values[index] = std::move(value); // 计算并保存当前点的属性
        }
    }
    record_update(property.name, region_attribute, changed); // 整个区域求值完成后统一记录变化
}

bool PropertiesSystem::depends_on_field(const std::string& property_name, int region_attribute, const std::string& field_name) const // 判断: 某个属性在指定区域，是否依赖求解场
{
    const auto& tags = property_info(property_name).field_regions; // 判断: 某个属性在指定区域，是否已被标记为参与指定求解场的依赖更新
    const auto field = tags.find(field_name); // 取的 场名称 -> 依赖该场更新的区域编号集合
    return field != tags.end() && field->second.count(region_attribute) != 0; // 若不存在该场或该区域没有被标记依赖该场 则返回 False
}

bool PropertiesSystem::coupled_with_field(const std::string& property_name, int region_attribute, const std::string& field_name) const // 判断: 某个属性在指定区域，是否已被标记为参与指定求解场的耦合更新
{
    const auto& tags = property_info(property_name).coupled_regions; // 取的 场名称 -> 参与该场耦合更新的区域编号集合
    const auto field = tags.find(field_name); // 取参与该场耦合更新的区域编号集合
    return field != tags.end() && field->second.count(region_attribute) != 0; // 若不存在该场或该区域没有被标记参与耦合该场 则返回 False
}

template <typename Manager>
void PropertiesSystem::evaluate_and_store_face_points(const Manager& manager, const PropertyInfo& property, int attribute, const std::vector<RequiredState>& required)
{
    const auto& mesh = fem_system_.mesh_data().mfem_mesh();
    auto& values = properties_field_data_.face_integration_point_values.at(property.name);
    values.resize(fem_system_.face_quadrature_point_count());
    bool changed = false;
    mfem::IsoparametricTransformation transformation;
    mfem::Vector position(mesh.SpaceDimension());
    for (const int face : fem_system_.mesh_data().attribute_faces(attribute))
    {
        mesh.GetFaceTransformation(face, &transformation);
        const auto* rule = fem_system_.boundary_quadrature_rule(transformation.GetGeometryType());
        const std::size_t offset = fem_system_.face_quadrature_offset(face);
        transformation.Transform(mfem::Geometries.GetCenter(transformation.GetGeometryType()), position);
        const std::array<double, 3> face_center{position[0], position.Size() > 1 ? position[1] : 0.0, position.Size() > 2 ? position[2] : 0.0};
        for (int q = 0; q < rule->GetNPoints(); ++q)
        {
            transformation.Transform(rule->IntPoint(q), position);
            PropertiesStatePoint point;
            point.face_attribute = point.zone_attribute = attribute;
            point.face_center = face_center;
            point.face_index = face;
            point.face_point_index = q;
            point.x = position[0];
            point.y = position.Size() > 1 ? position[1] : 0.0;
            point.z = position.Size() > 2 ? position[2] : 0.0;
            load_model_states(point, property, attribute, required, offset + q);
            PropertiesFieldData::CachedValue value = manager.evaluate(property.name, point);
            changed = changed || values[offset + q] != value;
            values[offset + q] = std::move(value);
        }
    }
    record_update(property.name, attribute, changed);
}

void PropertiesSystem::require_field(const std::string& name, const std::vector<double>* values) // 登记需要统一采样的场，并准备存储空间，不计算场值
{
    if (values)
    {
        physics_result_array_sources_[name] = values;
    }
    if (property_info(name).type == PropertyType::Field) // 只有 Field 类型才登记；普通材料属性已经由模型计算，不需要场采样
    {
        required_field_and_result_names_.insert(name); // 将名称加入采样集合，自动去重
        properties_field_data_.domain_integration_point_values.at(name).resize(fem_system_.domain_quadrature_point_count()); // 将对应数组调整为全域积分点总数 新增位置初始为空，后续由 sample_required_fields() 填入实际值
    }
}

void PropertiesSystem::sample_required_fields(const FieldMap& trial_fields)
{
    const auto& mesh = fem_system_.mesh_data().mfem_mesh();
    for (const std::string& name : required_field_and_result_names_) // 逐个处理需要采样的名称，例如温度、浓度、最大剪应力
    {
        const auto registered = physics_result_array_sources_.find(name); // 查找它是否登记了现成的积分点数组。如应力标量走这个来源，后面直接按下标读取
        std::optional<FieldView> view; // 准备一个可为空的场视图。读取现成数组时不需要它；从 FEM 采样时才填写
        if (registered == physics_result_array_sources_.end()) // 如果没有登记数组，就从有限元场取值：
        {
            const auto trial = trial_fields.find(name);
            view = trial != trial_fields.end() ? trial->second : fem_system_.field_view(name);
        }
        auto& values = properties_field_data_.domain_integration_point_values.at(name);
        for (int region : fem_system_.mesh_data().region_attributes())
        {
            bool changed = false;
            for (int element : fem_system_.mesh_data().region_elements(region))
            {
                const auto* rule = fem_system_.domain_quadrature_rule(mesh.GetElementBaseGeometry(element));
                const std::size_t offset = fem_system_.domain_quadrature_offset(element);
                for (int q = 0; q < rule->GetNPoints(); ++q)
                {
                    const std::size_t index = offset + q;
                    PropertiesFieldData::CachedValue value = registered != physics_result_array_sources_.end() ? StateValue{(*registered->second)[index]} : view->evaluate(region, element, rule->IntPoint(q)); // 已登记数组：`registered->second` 是数组指针，解引用后用 [index] 读取，再包装成 StateValue // 有限元场：在该单元的参考积分点位置计算场值
                    changed = changed || values[index] != value;
                    values[index] = std::move(value);
                }
            }
            record_update(name, region, changed);
        }
    }
}

void PropertiesSystem::update_field_dependent_properties(const std::string& field_name, const FieldMap& trial_fields, bool coupled_only)
{
    sample_required_fields(trial_fields);
    for (const PropertyInfo& property : properties_) // 按属性表顺序更新 Derived、Material、Assembly
    {
        if (property.type != PropertyType::Derived && property.type != PropertyType::Material && property.type != PropertyType::Assembly)
        {
            continue;
        }
        const auto assigned = assigned_models_.find(property.name);
        if (assigned == assigned_models_.end())
        {
            continue;
        }
        for (const auto& [attribute, model_name] : assigned->second)
        {
            const int location = property.scope == PropertyScope::Global ? 0 : attribute;
            if (!depends_on_field(property.name, location, field_name) || (coupled_only && !coupled_with_field(property.name, location, field_name)))
            {
                continue;
            }
            const auto& required = model_required_states(property, model_name);
            const bool uses_history = std::find(required.begin(), required.end(), property.name) != required.end();
            if (!uses_history && computed_locations_[property.name].count(location) != 0 && !required_states_changed(property, location, required))
            {
                continue;
            }
            if (property.type == PropertyType::Derived)
            {
                evaluate_property(derived_model_manager_, property, location, required);
            }
            else if (property.type == PropertyType::Material)
            {
                evaluate_property(material_model_manager_, property, location, required);
            }
            else
            {
                evaluate_property(assembly_model_manager_, property, location, required);
            }
        }
    }
}

template <typename Manager>
void PropertiesSystem::evaluate_property(const Manager& manager, const PropertyInfo& property, int location, const std::vector<RequiredState>& required) // 根据存储范围选择求值位置并保存结果
{
    if (property.scope == PropertyScope::FaceIntegrationPoint)
    {
        evaluate_and_store_face_points(manager, property, location, required);
    }
    else if (property.scope == PropertyScope::DomainIntegrationPoint)
    {
        evaluate_and_store_domain_points(manager, property, location, required);
    }
    else
    {
        auto point = make_state_point(property.scope, location);
        load_model_states(point, property, location, required);
        evaluate_and_store(manager, property.name, point);
    }
}

void PropertiesSystem::accept_current_state() // 把当前全部属性数据复制一份，保存为“已接受状态”的快照
{
    previous_field_data_ = properties_field_data_;
}

const PropertiesFieldData& PropertiesSystem::previous_field_data() const
{
    return previous_field_data_;
}

PropertiesFieldData& PropertiesSystem::field_data()
{
    return properties_field_data_;
}

const PropertiesFieldData& PropertiesSystem::field_data() const
{
    return properties_field_data_;
}

}
