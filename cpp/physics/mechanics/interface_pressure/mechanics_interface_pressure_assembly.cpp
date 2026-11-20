#include "physics/mechanics/interface_pressure/mechanics_interface_pressure_assembly.hpp"

#include <utility>

namespace trimuse
{

MechanicsInterfacePressureAssembly::MechanicsInterfacePressureAssembly(const FemSystem& fem, const TrialField& trial_field)
    :
    fem_(fem),
    trial_field_(trial_field),
    load_(std::make_unique<mfem::Vector>(trial_field.size()))
{
    *load_ = 0.0; // 不管是否开启该模型，初始化该模型对应载荷向量为0
}

void MechanicsInterfacePressureAssembly::add_pressure(const InterfaceContext& context, int side, double pressure, const PropertiesFieldData::FaceIntegrationPointValues& weights) // 把当前面片某一侧的压力，积分为该侧单元的等效节点载荷，再累加到完整载荷向量中
{
    // fᵉ_(c,i) = ∫Γₑ Nᵢ(−p n꜀) dΓ ≈ Σq wq JΓ,q gq Nᵢ(ξq)(−p n꜀,q)
    // c 为位移方向分量，i 为单元节点编号，q 为面积分点编号
    // wq 为求积权重，JΓ,q 为面片几何映射权重，gq 为降维几何权重
    const auto& interface = context.interface(); // 当前面片的信息，包括面片编号，以及两侧各自的区域和相邻单元编号
    const int region_attribute = interface.sides[side].region_attribute; // 取得受压侧的区域编号和原网格单元编号
    const int element = interface.sides[side].element; // 这一侧紧邻界面的域单元编号 (原网格编号)
    const int dimension = fem_.mesh_data().dimension(); // 求解维数
    const auto* fe = trial_field_.space(region_attribute).GetFE(trial_field_.local_element(element)); // 受压侧有限元对象
    auto& transformation = context.transformations(); // 当前面片及其两侧单元的几何映射对象
    mfem::Vector shape(fe->GetDof()), traction(dimension), local(fe->GetDof() * dimension); // 受压侧单元全部标量形函数的值 当前积分点面力向量 当前面片对受压侧单元贡献的局部载荷向量
    local = 0.0;
    const auto& rule = context.quadrature_rule(); // 当前面片的求积规则
    for (int q = 0; q < rule.GetNPoints(); ++q) // 遍历面片积分点
    {
        const auto& point = rule.IntPoint(q); //取得当前参考面积分点，包含参考坐标与权重
        transformation.SetAllIntPoints(&point); // 准备转换当前参考位置
        const auto& local_point = side == 0 ? transformation.GetElement1IntPoint() : transformation.GetElement2IntPoint(); // 选出该物理点在受压侧参考单元中的坐标
        fe->CalcShape(local_point, shape); // 计算受压侧单元的所以形函数值
        if (dimension == 1) 
        {
            auto* element_transformation = side == 0 ? transformation.Elem1 : transformation.Elem2; // 取得受压侧域单元集合映射
            const double orientation = element_transformation->Jacobian()(0, 0); // 取得一维映射导数 J = dx/dpsi
            traction[0] = (local_point.x < 0.5 ? -1.0 : 1.0) * (orientation > 0.0 ? 1.0 : -1.0); // 先确定界面位于参考线单元哪一端，再结合映射方向，得到物理坐标中的单位外法向，让 traction 暂存该法向
        }
        else
        {
            mfem::CalcOrtho(transformation.Jacobian(), traction); // 2D、3D 直接根据面片映射雅可比计算第一侧单元的外法向
            traction *= (side == 0 ? 1.0 : -1.0) / traction.Norml2(); // 先除以长度，将法向单位化；若受压侧是第二侧，就反向
        }
        traction *= -pressure; // 法向转换为压力对应的面力
        const double weight = std::get<double>(weights[interface.face][q].value());
        for (int component = 0; component < dimension; ++component) // 遍历位移分量
        {
            for (int node = 0; node < shape.Size(); ++node) // 遍历受压侧单元的标量形函数 local 按分量分块排列[分量0的全部节点载荷，分量1的全部节点载荷，……]
            {
                local[component * shape.Size() + node] += weight * shape[node] * traction[component]; // f_(c,i) += w Ni (-p n_c)
            }
        }
    }
    mfem::Array<int> dofs;
    trial_field_.element_dofs(element, dofs);
    load_->AddElementVector(dofs, local); // 只写入受压侧，连接约束由顶层统一处理
}

std::unique_ptr<mfem::Vector> MechanicsInterfacePressureAssembly::finalize()
{
    return std::move(load_);
}

}
