#include "physics/thermal/interface/local_from_constant_resistance.hpp"
#include "physics/thermal/interface/thermal_interface_assembly.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trimuse
{

namespace
{

std::vector<double> numbers(const ModelParameterMap& parameters, const std::string& name) // 按名称从参数字典中读取参数，统一返回 std::vector<double>
{
    const auto& value = parameters.at(name); // 取得该参数对应的 variant 的只读引用
    if (const auto* scalar = std::get_if<double>(&value)) // 若里面存的是 double
    { 
        return {*scalar}; // 包装成只有一个元素的向量并返回
    }
    return std::get<std::vector<double>>(value); // 否则直接去除里面的向量并返回
}
}

ThermalLocalFromConstantResistance::ThermalLocalFromConstantResistance(const ModelParameterMap& parameters) // 构造函数
    : dimension_(parameters.count("theta_mins") ? 2 : 3) // 如果参数表里有 theta_mins 则是 2d 问题 否则是 3d 问题
{
    constexpr double pi = 3.14159265358979323846;
    if (parameters.count("center")) // 如果参数字典中存在 center (python 层传入 geometry_mesh 中的center)，则读取它；不存在时，成员 center_{} 保持默认的 {0, 0, 0}
    {
        const auto center = numbers(parameters, "center");
        if (center.size() != 3) // 2d 使用 {x,y,0}
        { 
            throw std::invalid_argument("center 需要三个坐标分量"); 
        }
        std::copy(center.begin(), center.end(), center_.begin()); // 把局部向量 center 中的三个坐标复制到成员数组 center_
    }
    const auto resistances = numbers(parameters, "constant_resistances"); // 读取每块区域的热阻列表
    const auto minimums = numbers(parameters, dimension_ == 2 ? "theta_mins" : "angular_distance_mins"); // 按维数选择角度下限的参数名 2d 极角下限 theta_mins 3d 相对中心方向的角距离下限 angular_distance_mins
    const auto maximums = numbers(parameters, dimension_ == 2 ? "theta_maxs" : "angular_distance_maxs"); // 按维数选择角度下限的参数名 2d 极角上限 theta_maxs 3d 相对中心方向的角距离上限 angular_distance_maxs
    const auto thetas = dimension_ == 3 ? numbers(parameters, "thetas") : std::vector<double>{}; // 只有 3d 需要每块区域的中心方向角 theta、phi
    const auto phis = dimension_ == 3 ? numbers(parameters, "phis") : std::vector<double>{};
    const auto count = resistances.size(); // 用热阻列表长度作为区域数量
    if (count == 0 || minimums.size() != count || maximums.size() != count || (dimension_ == 3 && (thetas.size() != count || phis.size() != count))) // 至少一块区域 每个热阻都有对应的角度上下限 3d 每个区域还必须有对应的中心方向角
    {
        throw std::invalid_argument("局部热阻的参数列表必须非空且长度一致");
    }
    for (std::size_t i = 0; i < count; ++i) // 逐个区域检查并整理
    {
        if (!std::isfinite(resistances[i]) || resistances[i] <= 0.0 || !(0.0 <= minimums[i] && minimums[i] < maximums[i] && maximums[i] <= pi)) // 热阻大于 0 且有限 角度范围在 0-pi
        {
            throw std::invalid_argument("局部热阻必须为有限正数，角范围必须满足 0 <= min < max <= pi");
        }
        std::array<double, 3> direction{}; // 创建三个分量全为零的数组存储 3d 的方向 2d 不需要
        if (dimension_ == 3)
        {
            if (!(0.0 <= thetas[i] && thetas[i] <= pi && 0.0 <= phis[i] && phis[i] < 2.0 * pi))
            {
                throw std::invalid_argument("3D 中心方向要求 theta 在 [0,pi]、phi 在 [0,2*pi)");
            }
            direction = {std::sin(thetas[i]) * std::cos(phis[i]), std::sin(thetas[i]) * std::sin(phis[i]), std::cos(thetas[i])}; // 球坐标 -> 笛卡尔单位向量
        }
        zones_.push_back({1.0 / resistances[i], minimums[i], maximums[i], direction}); // 按 Zone 的成员顺序构造并追加
    }
}

void ThermalLocalFromConstantResistance::assemble(const InterfaceContext& context, ThermalInterfaceAssembly& assembly) const // 处理当前面片
{
    auto& transformation = context.transformations(); // 取得当前界面片的双侧几何变换对象
    if (transformation.Elem1->GetDimension() != dimension_)
    {
        throw std::invalid_argument("局部热阻的角度参数与网格维数不一致，仅支持 2D/3D");
    }
    mfem::Vector position(transformation.GetSpaceDim()); // 按实际空间维数创建坐标向量，2D 长度为 2，3D 长度为 3
    transformation.Transform(mfem::Geometries.GetCenter(transformation.GetGeometryType()), position); // mfem::Geometries.GetCenter(参考几何形状) 取得该参考几何的中心点 transformation.Transform(..., position); 把参考中心映射到实际界面上，结果写入 position
    const double x = position[0] - center_[0], y = position[1] - center_[1]; // 转换为相对颗粒中心的坐标
    const double z = dimension_ == 3 ? position[2] - center_[2] : 0.0; // 转换为相对颗粒中心的坐标 3d 还要转换 z
    const double radius = std::sqrt(x * x + y * y + z * z); // 计算相对颗粒中心的半径
    for (const auto& zone : zones_) // 逐个检查区域 计算判定角度
    {
        // 2D 坐标为 (r,z)，从上极点起算；3D 与几何卡的球冠/环带方向约定一致。
        const double angle = dimension_ == 2 ? std::atan2(std::abs(x), y) // 2d 计算从轴向正方向 也就是上极点方向起算的极角
            : std::acos(std::clamp((x * zone.direction[0] + y * zone.direction[1] + z * zone.direction[2]) / radius, -1.0, 1.0)); // 3d 计算与 direction 的夹角 cos(alpha) = p \dot d / |p| = (x*dx+y*dy+z*dz) / r 后面俩 -1 是 将结果限制在 [-1,1]，避免浮点舍入得到 1.0000000000000002，导致 acos() 无法计算
        if (zone.minimum <= angle && angle <= zone.maximum) // 落入区域
        {
            mfem::ConstantCoefficient conductance(zone.conductance); // 把保存的 h=1/R 包装成 MFEM 常数系数对象
            assembly.add_transfer(context, conductance); // 用这个常数传热系数组装当前整块面片的界面矩阵
            return;
        }
    }
    assembly.connect(context); // 所有区域都没匹配则是完美接触
}

}
