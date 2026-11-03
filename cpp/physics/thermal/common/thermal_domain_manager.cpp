#include "physics/thermal/common/thermal_domain_manager.hpp"
namespace trimuse
{

ThermalDomainManager::ThermalDomainManager(const FemSystem& fem) // 初始化 values 的长度为全域积分点总数 值为 0.0 models_ 初始化为空
    : fem_(fem), values_(fem.domain_quadrature_point_count(), 0.0) 
{
}

std::vector<ThermalDomainConfig> ThermalDomainManager::defaults(const FemSystem& fem, const std::string& model) // 用于给网格中的每个区域生成相同的默认模型配置 返回能被 assign 方法传入的 configs
{
    std::vector<ThermalDomainConfig> configs; // 创建一个空列表，用于保存各区域的模型配置
    for (int region : fem.mesh_data().region_attributes()) 
    { 
        configs.push_back({region, model, {}}); 
    } // 取得网格中所有区域的属性编号 并逐个遍历 为每个区域创建一个 ThermalDomainConfig 并追加到列表中 这里只是安排配置还没生成具体模型 
    // 例如调用 defaults(fem, "fourier"); 得到 configs = {{3,  "fourier", {}},{17,  "fourier", {}}}
    return configs;
}

void ThermalDomainManager::assign(const std::vector<ThermalDomainConfig>& configs) // 根据配置列表{区域编号，每个区域的模型名，模型构造参数} 为每个区域创建具体模型 并保存到 models_ 中
{
    for (const auto& config : configs) // 逐条遍历区域的配置
    {
        const auto& entry = registry_.registration(config.model_name); // 根据模型名称，从注册表中取得对应的注册记录 注意: 注册是具体管理器继承本抽象基类后在具体管理器内部注册的 调用此方法时已经注册好了
        models_.emplace(config.region_attribute, Assignment{entry.factory(config.parameters), entry.required_states}); // entry.factory(config.parameters) 调用工厂函数,传入参数字典,生成具体模型的构造函数返回其智能指针; Assignment{模型指针, 依赖属性列表} 构造区域分配记录 dirty 没有显式填写因此采用默认值 true 表示需要首次组装; 插入区域映射表 models_.emplace(区域编号, 分配记录);
    }
}

void ThermalDomainManager::update(const PropertiesFieldData& properties) // 更新各区域积分点上的系数 values_，并判断它们是否发生变化更新 dirty
{
    for (auto& [region, assignment] : models_) // 遍历已配置模型的区域
    for (int element : fem_.mesh_data().region_elements(region)) // 遍历该区域中的单元
    for (std::size_t point = fem_.domain_quadrature_offset(element); // 遍历该单元中的积分点
         point < fem_.domain_quadrature_offset(element + 1); ++point)
    {
        const double value = assignment.model->evaluate(properties, point); // 调用当前区域的具体模型计算该积分点的新系数 如导热系数取 k 热容模型算 rho*cp
        assignment.dirty = assignment.dirty || value != values_[point]; // 将新/旧系数逐积分点比较 只要一个点的系数改变 就将整个区域记为 dirty=true 若之前已经是 true 了 则会保持 true 直到组装完成后清除
        values_[point] = value; // 将新系数写入缓存供后续组装使用
    }
}

bool ThermalDomainManager::coupled(const PropertiesSystem& properties, const std::string& field) const // 判断: 当前管理器负责的模型中，是否有任何一个与指定求解场耦合
{
    for (const auto& [region, assignment] : models_) // 遍历各区域及其模型分配记录
    for (const auto& name : assignment.required) // 遍历该模型依赖的属性名称
    {
        if (properties.coupled_with_field(name, region, field)) 
        { 
            return true; // 向属性系统查询这个属性在当前区域是否已标记为与 field 耦合, 若耦合则立刻返回 true
        }
    }
    return false;
}

void ThermalDomainManager::mark_coupled(PropertiesSystem& properties, const std::string& field) const // 把当前管理器所依赖的属性，交给属性系统登记与指定场的耦合关系
{
    for (const auto& [region, assignment] : models_) // 遍历各区域的模型分配记录
    { 
        properties.mark_coupled(field, assignment.required); // 把指定场名称和模型依赖的属性列表传给属性系统 例如导热管理器会调用 properties.mark_coupled("temperature",{"material.thermal.conductivity"});属性系统会进一步检查：导热系数在哪些区域依赖温度，只给符合条件的区域设置耦合标记，并向上游依赖传播标记
    }
}

bool ThermalDomainManager::required_no_change(int region) const // 根据 dirty 判断指定区域的系数是否与最近一次组装时一致(dirty是否为False)，因而可以复用已有矩阵或向量
{ 
    return !models_.at(region).dirty; // .dirty 是需要重新组装的标记 !.dirty 即不需要重新组装
}

void ThermalDomainManager::assembled(int region) // 将 region 处的 dirty 记为 false
{ 
    models_.at(region).dirty = false; 
}

std::vector<int> ThermalDomainManager::regions() const // 返回当前管理器中已经配置模型的所有区域编号
{
    std::vector<int> result; // 创建空的整数列表
    for (const auto& [region, assignment] : models_) // 遍历 models_，取出每一项的区域编号 region，追加到列表
    { 
        result.push_back(region); 
    }
    return result;
}

}
