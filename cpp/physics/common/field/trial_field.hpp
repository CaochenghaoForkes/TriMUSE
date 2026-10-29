#pragma once

#include "fem/fem_system.hpp"

namespace trimuse
{

class TrialField // 物理求解使用的独立试探场; 全部矩阵索引采用这里的完整自由度编号
{
public:
    TrialField(FemSystem& fem, FieldData& field); // 使用连续场时的构造函数，负责保存正式场引用，并建立独立的试探场

    TrialField(FemSystem& fem, RegionFieldData& field); // 使用分域场时的构造函数：保存正式场，创建同样布局的试探场，记录各区域的自由度偏移，最后复制场值

    TrialField(TrialField&&) = default; // TrialField 的移动构造函数，让编译器生成默认实现

    bool regional() const { return region_target_ != nullptr; } // 判断该场用的是连续场还是分域场

    int size() const; // 返回当前试探场的全部自由度数量，也就是整体场向量的长度

    mfem::FiniteElementSpace& space(int region) const; // 取得当前试探场所使用的 MFEM 有限元空间 如果是分域场则取 region 找到该区域的 FiniteElementSpaceData

    int local_element(int parent_element) const; // 将原网格的域单元编号转换为当前试探场所属网格中的单元编号

    void element_dofs(int parent_element, mfem::Array<int>& dofs) const; // 根据原网格的域单元编号，将该单元对应的自由度编号填入 dofs

    void face_dofs(int region, int parent_face, mfem::Array<int>& dofs) const; // 取得指定区域一侧、某个网格面上的自由度编号，并填入 dofs region 是区域属性编号用于指定面的哪一侧 face 是原网格中的面编号不是边界单元编号 dofs 输出的自由度编号数组

    mfem::Vector values() const; // 将当前试探场的全部自由度值，作为一个整体向量返回

    void set_values(const mfem::Vector& values); // 将传入的整体场向量写入试探场

    void load(); // 把正式场当前的场值复制到试探场，作为本次求解的初始值

    void accept(); // 把当前试探场的场值复制回正式场，接受本次求解结果

    FieldView view() const; // 用于把当前试探场包装成统一的只读访问接口 FieldView，供 properties 等模块在积分点上读取场值

    FieldData* continuous_target() const { return continuous_target_; }

    RegionFieldData* region_target() const { return region_target_; }

private:
    FieldData* continuous_target_ = nullptr; // 指向外部已有的连续正式场 包含有限元空间和场数据

    RegionFieldData* region_target_ = nullptr; // 指向外部已有的分域正式场，其中包含各区域独立的有限元空间和场数据

    std::unique_ptr<mfem::GridFunction> continuous_trial_; // 独占拥有连续场的试探场向量，用于保存当前迭代中的场值

    std::unique_ptr<RegionFieldData> region_trial_; // 独占拥有分域场的试探场对象，其中各区域分别保存自己的试探场值

    std::map<int, int> offsets_; // 保存分域场的区域编号 -> 该区域自由度在整体拼接向量中的起始偏移
};
}
