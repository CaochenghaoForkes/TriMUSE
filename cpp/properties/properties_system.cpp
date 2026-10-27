#include "properties/properties_system.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace trimuse
{

PropertiesSystem::PropertiesSystem(const FemSystem& fem_system) // Global、Region、Zone 初始化为 std::nullopt IntegrationPoint 创建空数组 并预存储各个区域的体积
    : fem_system_(fem_system)
{
    for (const PropertyInfo& property : properties_)
    { 
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
        else
        {
            properties_field_data_.integration_point_values.emplace(property.name, PropertiesFieldData::IntegrationPointValues{}); // 初始化各 integrationPoint 属性在的积分点值列表为空
        }
    }
    for (const auto& [region_attribute, volume] : fem_system_.mesh_data().region_volumes())
    {
        properties_field_data_.region_values["precomputed.region_volume"][region_attribute] = StateValue{volume}; // 预存储各个区域的体积
    }
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
    const auto property = std::find_if(properties_.begin(), properties_.end(), [&name](const PropertyInfo& item) 
    {
        return item.name == name; // 查找成功返回指向该元素的迭代器；失败则返回 end()
    });
    if (property == properties_.end()) // 未找到时抛出异常
    {
        throw std::out_of_range("未知属性: " + name);
    }
    return static_cast<std::size_t>(property - properties_.begin()); // 两个迭代器相减得到偏移量 (即 properties 中该 name 对应的的下标)
}

const std::vector<std::string>& PropertiesSystem::model_required_states(const PropertyInfo& property, const std::string& model_name) const // 返回计算 property 属性的 model_name 模型所需要的参数
{
    if (property.type == PropertyType::External)
    {
        return external_model_manager_.required_states(property.name, model_name);
    }
    if (property.type == PropertyType::Derived)
    {
        return derived_model_manager_.required_states(property.name, model_name);
    }
    return material_model_manager_.required_states(property.name, model_name);
}

void PropertiesSystem::assign(int attribute, const std::string& model_name, const ModelParameterMap& parameters) // 为某个属性在指定 体积域/边界域 上配置一个计算模型 并把该位置已有的“计算完成”标记清掉
{
    // 模型名固定为 namespace.model, 如 model_name = "material.thermal.conductivity.common.time_series"; 
    const auto model_separator = model_name.rfind('.'); // rfind('.') 从右往左找最后一个点的位置
    const auto property_end = model_separator == std::string::npos ? std::string::npos : model_name.rfind('.', model_separator - 1); // std::string::npos 是字符串查找中表示“没找到”的特殊值 如果没找到最后一个点就直接返回；rfind(字符, 起始位置) 表示从指定位置开始，向左寻找该字符，起始位置也会参与检查 如果找到了最后一个点 就从 model_separator - 1 的位置(最后一个点的前一个字符位置) 开始找倒数第二个 .
    if (property_end == std::string::npos) // 如果找不到倒数第一个和倒数第二个点就报错
    {
        throw std::invalid_argument("模型路径应为完整属性名.common.xxx 或完整属性名.材料分类.xxx: " + model_name);
    }
    const std::string property_name = model_name.substr(0, property_end); // 截取倒数第二个点之前的内容 如 material.thermal.conductivity
    const std::string specific_model_name = model_name.substr(property_end + 1); // 截取倒数第二个点之后点全部内容 如 common.time_series
    const PropertyInfo& property = property_info(property_name); // 查询该属性的分类信息
    if (property.type == PropertyType::External)
    {
        external_model_manager_.assign(property_name, attribute, specific_model_name, parameters);
    }
    else if (property.type == PropertyType::Derived)
    {
        derived_model_manager_.assign(property_name, attribute, specific_model_name, parameters);
    }
    else if (property.type == PropertyType::Material)
    {
        material_model_manager_.assign(attribute, property_name, specific_model_name, parameters);
    }
    else
    {
        throw std::invalid_argument("预计算属性不接受模型配置: " + property_name);
    }
    assigned_models_[property_name][attribute] = specific_model_name; // 记录模型分配关系 属性名->位置编号->模型名 如 assigned_models_["material.thermal.conductivity"][3] = "common.constant";
    const int location = property.scope == PropertyScope::Global ? 0 : attribute; // 统一用于缓存管理的位置编号 全局属性固定为 0 其他属性为使用传入的区域或边界分区编号
    computed_locations_[property_name].erase(location); // 这里配置了新模型 之前的计算结果不能再当作已经更新好的结果使用 需要重新计算
}

void PropertiesSystem::prepare_dependencies() // 标记各个属性在各个区域依赖的场有哪些
{
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
            for (const std::string& required_name : model_required_states(property, model_name)) // 遍历模型所需状态量
            {
                if (solution_field_states.count(required_name) != 0) // 检查依赖名称是否在求解场名称集合中
                {
                    if (property.type != PropertyType::Material) // 只有材料属性允许依赖求解场 外部属性或派生属性如果声明依赖温度、浓度等求解场，就报错
                    {
                        throw std::logic_error("外部量和派生量不能依赖求解场: " + property.name);
                    }
                    property.field_regions[required_name].insert(location); // 记录当前属性在这个位置依赖该场
                    continue;
                }
                const std::size_t upstream_index = property_index(required_name); // 如果依赖的是其他属性 还要检查该属性(称之为上游属性) 首先查询该上游属性在 properties_ 中的下标
                const PropertyInfo& upstream = properties_[upstream_index]; // 取得上游属性的说明记录
                if (upstream.type > property.type || (upstream.type == property.type && upstream_index >= index)) // 检查依赖方向是否合法 1.枚举默认按声明顺序编号 Precomputed=0 External=1 Derived=2 Material=3 2.如果两个属性属于同一类别，那么上游属性必须排在当前属性之前
                {
                    throw std::logic_error("属性依赖必须遵守 external → derived → material 及类内先后顺序: " + property.name);
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

void PropertiesSystem::mark_coupled(const std::string& field_name, const std::vector<std::string>& property_names) // 把指定属性中依赖某个求解场的位置标记为“参与耦合更新”，并沿依赖关系向上游传递这个标记
{
    for (const std::string& name : property_names) // 逐个处理传入的属性名 (某个场依赖的某些属性[不代表属性一定依赖场])
    {
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
            for (const std::string& required_name : model_required_states(*property, model_name)) // 取得当前模型的依赖列表，逐项检查
            {
                if (solution_field_states.count(required_name) != 0)
                {
                    continue; // 如果依赖本身就是 "temperature" 等求解场，跳过
                }
                PropertyInfo& upstream = property_info(required_name); // 取得上游属性记录
                const int upstream_location = upstream.scope == PropertyScope::Global ? 0 : region_attribute; // 确定上游对应的位置：全局属性使用 0，其他使用相同配置编号
                if (depends_on_field(required_name, upstream_location, field_name)) // 检查这个上游属性是否也依赖当前场 (下游参与温度耦合，不代表它依赖的所有上游属性都需要随温度更新)
                {
                    upstream.coupled_regions[field_name].insert(upstream_location); // 把确实依赖当前场的上游位置，也标记为参与耦合更新
                }
            }
        }
    }
}

PropertiesStatePoint PropertiesSystem::make_state_point(PropertyScope scope, int attribute) // 根据属性的作用范围和位置编号，创建一个 PropertiesStatePoint 状态点对象，并设置它的区域/边界分区编号
{
    PropertiesStatePoint point;
    point.region_attribute = scope == PropertyScope::Region ? attribute : 0; // 如果是区域属性，就设置区域编号；否则设为 0
    point.zone_attribute = scope == PropertyScope::Zone ? attribute : 0; // 如果是边界分区属性，就设置边界分区编号；否则设为 0
    return point;
}

template <typename Manager>
void PropertiesSystem::evaluate_and_store(const Manager& manager, const std::string& name, const PropertiesStatePoint& point) // 调用管理器计算某项属性，把结果存到对应位置，并标记该位置已经计算完成 (这个方法只用于 Global、Region、Zone 属性)
{
    StateValue value = manager.evaluate(name, point); // 管理器根据属性名和状态点，找到对应模型并执行计算，返回 StateValue
    const PropertyScope scope = property_info(name).scope; // 查询存储范围
    if (scope == PropertyScope::Global)
    {
        properties_field_data_.global_values[name] = std::move(value);
    }
    else if (scope == PropertyScope::Zone)
    {
        properties_field_data_.zone_values[name][point.zone_attribute] = std::move(value);
    }
    else
    {
        properties_field_data_.region_values[name][point.region_attribute] = std::move(value);
    }
    const int location = scope == PropertyScope::Zone ? point.zone_attribute : point.region_attribute;
    computed_locations_[name].insert(location); // 把该位置加入这项属性的“已计算位置集合”
}

void PropertiesSystem::update_external(double time) // 更新当前时间，然后检查各个外部属性，只在“尚未计算”或“依赖发生变化”时重新计算它们
{
    properties_field_data_.global_values["external.time"] = StateValue{time}; // time 包装为 StateValue 存入时间属性
    for (const PropertyInfo& property : properties_)
    {
        if (property.type != PropertyType::External) // 只处理外部属性
        {
            continue;
        }
        const std::string& name = property.name;
        const auto assigned = assigned_models_.find(name);
        if (assigned == assigned_models_.end()) // 如果没有为该属性配置模型，跳过它
        {
            continue;
        }
        for (const auto& [attribute, model_name] : assigned->second) // 遍历各位置的模型配置
        {
            const auto& required = external_model_manager_.required_states(name, model_name); // 取得模型声明的依赖列表
            const int location = property.scope == PropertyScope::Global ? 0 : attribute; // 确定“已计算标记”使用的位置编号，全局属性统一使用 0
            if (computed_locations_[name].count(location) != 0 && !required_states_changed(attribute, required))
            {
                continue; // 如果这个位置已经被计算过 并且相比快照没有变化 就跳过计算保留缓存值
            }
            if (property.scope != PropertyScope::IntegrationPoint) // 对于全局、区域、边界分区属性，走单个状态点的计算流程
            {
                PropertiesStatePoint point = make_state_point(property.scope, attribute);
                load_required_states(point, required); // // 给 point 装配需要的属性
                evaluate_and_store(external_model_manager_, name, point);
                continue;
            }

            evaluate_and_store_points(external_model_manager_, property, attribute, required);
        }
    }
}

void PropertiesSystem::load_required_states(PropertiesStatePoint& point, const std::vector<std::string>& names, std::optional<std::size_t> point_index) const // 给 point 装配需要的属性
{
    for (const std::string& name : names)
    {
        const PropertyScope scope = property_info(name).scope;
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
            point.values[name] = properties_field_data_.integration_point_values.at(name)[point_index.value()].value();
        }
    }
}

bool PropertiesSystem::required_states_changed(int attribute, const std::vector<std::string>& required_states) const // 判断: 某个区域或边界分区所依赖的属性，相比之前保存的快照，是否发生变化
{
    const PropertiesFieldData& previous = required_state_snapshots_; // 之前保存的依赖状态快照(不一定就是上一个时间步的)
    const PropertiesFieldData& current = properties_field_data_; // 当前属性数据
    for (const std::string& name : required_states) // 逐个检查依赖属性
    {
        const PropertyScope scope = property_info(name).scope; // 找到范围
        if (scope == PropertyScope::Global) // 全局属性直接比较一个值
        {
            const auto old_value = previous.global_values.find(name);
            const auto new_value = current.global_values.find(name);
            if (old_value == previous.global_values.end() || new_value == current.global_values.end() || old_value->second != new_value->second)
            {
                return true; // 若快照无该属性 或 当前属性无该属性 或 二者内部 StateValue 不相等 则返回 true
            }
        }
        else if (scope == PropertyScope::Region || scope == PropertyScope::Zone)
        {
            const auto& previous_values = scope == PropertyScope::Region ? previous.region_values : previous.zone_values; // 先选择对应存储表
            const auto& current_values = scope == PropertyScope::Region ? current.region_values : current.zone_values; // 先选择对应存储表
            const auto old_property = previous_values.find(name); // 然后查找属性 位置编号 → 缓存值
            const auto new_property = current_values.find(name); // 然后查找属性 位置编号 → 缓存值
            if (old_property == previous_values.end() || new_property == current_values.end())
            {
                return true; // 新旧任一侧缺少这项属性，就返回 true
            }
            const auto old_value = old_property->second.find(attribute); // 接着，在属性的内层表中查找指定位置 
            const auto new_value = new_property->second.find(attribute); // 接着，在属性的内层表中查找指定位置
            if (old_value == old_property->second.end() || new_value == new_property->second.end() || old_value->second != new_value->second)
            {
                return true; // 指定位置缺少数据，或者新旧值不同，都返回 true
            }
        }
        else // 积分点属性: 比较指定区域内所有积分点
        {
            const auto& old_values = previous.integration_point_values.at(name); // 取得该属性的新旧全域积分点数组
            const auto& new_values = current.integration_point_values.at(name); // 取得该属性的新旧全域积分点数组
            for (const int element : fem_system_.mesh_data().region_elements(attribute)) // 从预存的区域单元表中，取得指定区域包含的所有原网格单元编号
            {
                const std::size_t end = fem_system_.domain_quadrature_offset(element + 1); // 取得当前单元积分点在连续数组中的结束位置，不包含这个位置
                for (std::size_t index = fem_system_.domain_quadrature_offset(element); index < end; ++index) // 遍历当前单元的全部积分点
                {
                    if (old_values[index] != new_values[index])
                    {
                        return true; // 任意一个积分点的缓存值不同，就立即返回 true
                    }
                }
            }
        }
    }
    return false; // 全部依赖都无变化返回 false
}

template <typename Manager>
void PropertiesSystem::evaluate_and_store_points(const Manager& manager, const PropertyInfo& property, int region_attribute, const std::vector<std::string>& required, const FieldMap& trial_fields) // 计算某个属性在指定区域内全部积分点上的值，存入全域积分点数组，最后标记该区域已计算
{
    const mfem::Mesh& mesh = fem_system_.mesh_data().mfem_mesh(); // 取得原网格
    std::vector<std::string> common_required; // 非积分点属性，如时间、区域属性
    std::vector<std::string> point_required; // 积分点属性，如积分点孔隙率
    std::vector<std::pair<std::string, FieldView>> field_required; // 温度、浓度等求解场, 区域更新时统一选定试探场或已保存场
    for (const std::string& required_name : required)
    {
        if (solution_field_states.count(required_name) != 0)
        {
            const auto trial = trial_fields.find(required_name); // 为当前依赖的求解场选择数据来源: 有试探场就用试探场，否则用 FemSystem 中保存的场，并把选好的视图存入列表
            field_required.emplace_back(required_name,
                trial != trial_fields.end() ? trial->second : fem_system_.field_view(required_name));
            continue;
        }
        (property_info(required_name).scope == PropertyScope::IntegrationPoint ? point_required : common_required).push_back(required_name); // 积分点属性加入 point_required 其他属性加入 common_required
    }
    PropertiesStatePoint common_point; // 建立公共状态点，设置当前区域编号，并加载时间、区域属性等依赖
    common_point.region_attribute = region_attribute;
    load_required_states(common_point, common_required);
    auto& values = properties_field_data_.integration_point_values.at(property.name); // 取得当前属性的全域积分点值数组的引用，后面的赋值直接修改缓存
    values.resize(fem_system_.domain_quadrature_point_count()); // 把数组长度设置为全域积分点总数
    for (const int element : fem_system_.mesh_data().region_elements(region_attribute)) // 遍历区域内单元
    {
        const mfem::Geometry::Type geometry = mesh.GetElementBaseGeometry(element);
        const mfem::IntegrationRule* rule = fem_system_.domain_quadrature_rule(geometry);
        const auto& positions = fem_system_.domain_quadrature_positions(element); // 取得之前缓存的、该单元全部积分点的实际空间坐标，避免重新计算坐标
        const std::size_t offset = fem_system_.domain_quadrature_offset(element); // 取得该单元积分点在全域连续数组中的起始下标
        for (int point_index = 0; point_index < rule->GetNPoints(); ++point_index) // 遍历当前单元的积分点
        {
            const mfem::IntegrationPoint& integration_point = rule->IntPoint(point_index); // 注意 integration_point 是单元中的参考坐标 而我们 field.evaluate() 用的也正是单元中的参考坐标
            const auto& position = positions[point_index]; // 读取这个积分点已经缓存的实际坐标
            const std::size_t index = offset + point_index; // 将局部积分点编号转换为全域积分点下标

            PropertiesStatePoint point; // 构造当前积分点的模型输入
            point.region_attribute = region_attribute;
            point.x = position[0];
            point.y = position[1];
            point.z = position[2];
            point.values = common_point.values;
            load_required_states(point, point_required, index); // 按全域积分点下标 index，读取该点需要的积分点属性
            for (const auto& [name, field] : field_required) // 由 FEM 层统一选择区域、映射单元编号并取值
            {
                point.values[name] = field.evaluate(region_attribute, element, integration_point);
            }
            values[index] = manager.evaluate(property.name, point); // 计算并保存当前点的属性
        }
    }
    computed_locations_[property.name].insert(region_attribute); // 标记整个区域已完成计算
}

void PropertiesSystem::update_derived() // 检查各个派生属性，只在“尚未计算”或“依赖发生变化”时重新计算它们
{
    for (const PropertyInfo& property : properties_)
    {
        if (property.type != PropertyType::Derived)
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
            const auto& required = derived_model_manager_.required_states(name, model_name);
            const int location = property.scope == PropertyScope::Global ? 0 : attribute;
            if (computed_locations_[name].count(location) != 0 && !required_states_changed(attribute, required))
            {
                continue;
            }
            if (property.scope != PropertyScope::IntegrationPoint)
            {
                PropertiesStatePoint point = make_state_point(property.scope, attribute);
                load_required_states(point, required);
                evaluate_and_store(derived_model_manager_, name, point);
                continue;
            }

            evaluate_and_store_points(derived_model_manager_, property, attribute, required);
        }
    }
}

void PropertiesSystem::update_material() // 检查各个不依赖场的材料属性，只在“尚未计算”或“依赖发生变化”时重新计算它们
{
    for (const PropertyInfo& property : properties_)
    {
        if (property.type != PropertyType::Material)
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
            if (std::any_of(property.field_regions.begin(), property.field_regions.end(), [location](const auto& item)
            {
                return item.second.count(location) != 0;
            }))
            {
                continue;
            }
            const auto& required = material_model_manager_.required_states(name, model_name);
            if (computed_locations_[name].count(location) != 0 && !required_states_changed(attribute, required))
            {
                continue;
            }
            if (property.scope != PropertyScope::IntegrationPoint)
            {
                PropertiesStatePoint point = make_state_point(property.scope, attribute);
                load_required_states(point, required);
                evaluate_and_store(material_model_manager_, name, point);
                continue;
            }

            evaluate_and_store_points(material_model_manager_, property, attribute, required);
        }
    }
    required_state_snapshots_ = properties_field_data_;
}

void PropertiesSystem::update_field_dependent_materials(const std::string& field_name, const FieldMap& trial_fields, bool coupled_only) // 更新各个依赖场的材料属性
{
    for (const PropertyInfo& property : properties_)
    {
        if (property.type != PropertyType::Material)
        {
            continue;
        }
        const auto assigned = assigned_models_.find(property.name);
        if (assigned == assigned_models_.end())
        {
            continue;
        }
        for (const auto& [region_attribute, model_name] : assigned->second)
        {
            if (!depends_on_field(property.name, region_attribute, field_name) ||
                (coupled_only && !coupled_with_field(property.name, region_attribute, field_name)))
            {
                continue;
            }
            const auto& required = material_model_manager_.required_states(property.name, model_name);
            evaluate_and_store_points(material_model_manager_, property, region_attribute, required, trial_fields);
        }
    }
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
