#include "physics/thermal/thermal_field.hpp"
#include <stdexcept>

namespace trimuse
{

ThermalField::ThermalField(FemSystem&, FieldData& field) // 使用连续温度场时的构造函数，负责保存正式场引用，并建立独立的试探场
    : continuous_target_(&field), // 将正式场 field 的地址保存到 continuous_target_
      continuous_trial_(std::make_unique<mfem::GridFunction>(field.mfem_grid_function().FESpace())) // field.mfem_grid_function().FESpace() 取得正式场所使用的有限元空间指针 std::make_unique<mfem::GridFunction>(该空间指针) 在同一个有限元空间上创建新的试探 GridFunction
{
    if (continuous_trial_->FESpace()->GetVDim() != 1) // 检查场的分量数是否为 1，因为温度是标量
    { 
        throw std::invalid_argument("温度场必须为标量 H1 场"); 
    }
    load(); // 将正式场当前的温度复制到新建的试探场中
}

ThermalField::ThermalField(FemSystem& fem, RegionFieldData& field) // 使用分域温度场时的构造函数：保存正式场，创建同样布局的试探场，记录各区域的自由度偏移，最后复制温度值
    : region_target_(&field)
{
    const auto attributes = fem.mesh_data().region_attributes(); // 取得网格中所有区域的属性编号
    const auto& first_space = field.finite_element_space(attributes.front()); // attributes.front() 取得第一个区域的编号 再取得该区域的有限元空间
    if (first_space.vector_dimension() != 1) // 温度场是标量场
    { 
        throw std::invalid_argument("温度场必须为标量 H1 场"); 
    }
    region_trial_ = std::make_unique<RegionFieldData>(fem.mesh_data(), first_space.order()); // 根据同一个网格和相同插值阶次，创建新的分域试探场
    for (std::size_t i = 0; i < attributes.size(); ++i) // 建立“区域编号 → 整体自由度偏移”的映射
    { 
        offsets_[attributes[i]] = field.dof_offsets()[i]; // 最后一个是整体自由度总数,不存入 attributes = {3, 17}; field.dof_offsets() = {0, 10, 18};
    }
    load(); // 将正式分域场的温度值复制到试探场，完成初始化
}

int ThermalField::size() const // 返回当前试探温度场的全部自由度数量，也就是整体温度向量的长度
{
    return regional() ? region_trial_->dof_offsets().Last() : continuous_trial_->Size(); // 如果是分域场 dof_offsets() 各区域完整自由度块的起始位置，包含全部分量及已知边界自由度 最后一项等于各区域自由度数量之和 .Last() 返回最后一项 (不同区域在公共界面上的自由度分别计数) 如果是连续场 直接返回 GridFunction 的向量长度 这里返回的是储存的完整自由度,还没有扣除理想接触等约束,也没有扣除 Dirichlet 边界
}

mfem::FiniteElementSpace& ThermalField::space(int region) const // 取得当前试探温度场所使用的 MFEM 有限元空间 如果是分域场则取 region 找到该区域的 FiniteElementSpaceData
{
    return regional() ? region_trial_->finite_element_space(region).mfem_space() : *continuous_trial_->FESpace(); // 如果是分域场则取 region 找到该区域的 FiniteElementSpaceData，再取出其中的 mfem::FiniteElementSpace 如果是连续场直接返回空间指针
}

int ThermalField::local_element(int element) const // 将原网格的域单元编号转换为当前试探场所属网格中的单元编号
{
    return regional() ? region_trial_->local_element(element) : element; // 如果是分域场，先根据原单元确定所属区域，再返回它在该区域子网格中的局部单元编号 如果是连续场，场直接定义在原网格上，原样返回编号即可
}

void ThermalField::element_dofs(int element, mfem::Array<int>& dofs) const // 根据原网格的域单元编号，将该单元对应的自由度编号填入 dofs
{
    if (regional()) 
    { 
        region_trial_->element_dofs(element, dofs); // 如果使用分域场 内部 1.确定单元所属区域 2.讲原单元编号映射为子网格局部单元编号 3.取得该单元在区域空间中的自由度编号 4. 加上区域偏移转换为分域场整体拼接向量中的编号
    }
    else 
    { 
        continuous_trial_->FESpace()->GetElementVDofs(element, dofs); // 如果使用连续场，直接从原网格上的有限元空间取得该单元的自由度编号
    }
}

void ThermalField::face_dofs(int region, int face, mfem::Array<int>& dofs) const // 取得指定区域一侧、某个网格面上的自由度编号，并填入 dofs region 是区域属性编号用于指定面的哪一侧 face 是原网格中的面编号不是边界单元编号 dofs 输出的自由度编号数组
{
    space(region).GetFaceVDofs(regional() ? region_trial_->local_face(region, face) : face, dofs); // space(region): 取得当前区域的试探场空间;连续场则取得整个网格的空间 GetFaceVDofs(...) 取得该面上的自由度 分域场需要先把原网格面编号转换成对应子网格的局部面编号 连续场直接使用 face
    if (regional()) 
    { 
        for (int& dof : dofs) 
        { 
            dof += offsets_.at(region); // 分域场刚取得的是区域空间内的自由度编号，加上区域偏移后，转换成整体拼接向量中的编号
        } 
    }
}

mfem::Vector ThermalField::values() const // 将当前试探温度场的全部自由度值，作为一个整体向量返回
{
    mfem::Vector result;
    if (regional()) 
    { 
        region_trial_->get_values(result); 
    }
    else 
    { 
        result = *continuous_trial_; 
    }
    return result;
}

void ThermalField::set_values(const mfem::Vector& values) // 将传入的整体温度向量写入试探场
{
    if (regional()) 
    { 
        region_trial_->set_values(values); 
    }
    else 
    { 
        *continuous_trial_ = values; 
    }
}

void ThermalField::load() // 把正式场当前的温度复制到试探场，作为本次求解的初始值
{
    if (regional()) // 若当前使用分域场
    {
        mfem::Vector values; 
        region_target_->get_values(values); // 把正式分域场中各区域的自由度值，按区域偏移拼接到 values 中
        region_trial_->set_values(values); // 再将这个整体向量按相同布局拆分，写入各区域的试探场
    }
    else { *continuous_trial_ = continuous_target_->mfem_grid_function(); } // 如果用的是连续场则直接把正式场的全部自由度值复制到试探 GridFunction 中
}

void ThermalField::accept() // 把当前试探场的温度复制回正式场，接受本次求解结果
{
    if (regional()) 
    { 
        region_target_->set_values(values()); 
    }
    else 
    { 
        continuous_target_->mfem_grid_function() = *continuous_trial_; 
    }
}

FieldView ThermalField::view() const // 用于把当前试探场包装成统一的只读访问接口 FieldView，供 properties 等模块在积分点上读取温度
{
    if (regional()) 
    { 
        return region_trial_.get(); // 从 unique_ptr 取得 RegionFieldData*，随后通过 FieldView 对应的构造函数，自动转换成 FieldView
    }
    return continuous_trial_.get(); // 如果是连续场，取得 GridFunction*，同样构造相应的 FieldView
}

}
