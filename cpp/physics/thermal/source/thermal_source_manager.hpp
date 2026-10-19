#pragma once

#include <string>
#include <vector>

#include "common/model_parameters.hpp"
#include "fem/fem_system.hpp"
#include "fem/geometric_weighted_coefficient.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct ThermalSourceConfig
{
    int region_attribute;
    std::string model_name;
    ModelParameterMap parameters;
};

class ThermalSourceManager
{
public:
    ThermalSourceManager(FemSystem& fem_system, const std::vector<ThermalSourceConfig>& source_configs);

    void update(const PropertiesFieldData& properties_field_data);

    void add_integrators(mfem::LinearForm& linear_form);

private:
    class SourceCoefficient final : public mfem::Coefficient // 积分器只认识 Coefficient 对象,我们得重写其内部的 Eval 方法
    {
    public:
        explicit SourceCoefficient(int element_count) : values(element_count) {} // 创建 element_count 个空容器 values = [[], [], []...[]] 内侧容器将保存每个 element 的求积点的源 Q

        double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) override
        {   // MFEM 的积分器接收的是 mfem::Coefficient 对象 计算每个求积点时调用这个对象的 Eval(...)，询问当前点的 Q 是多少
            return values[transformation.ElementNo][point.index]; // point.index 该单元内当前求积点的编号 transformation.ElementNo 当前正在积分的单元编号
        }

        std::vector<std::vector<double>> values; // 按单元编号与局部求积点编号保存 Q
    };

    FemSystem& fem_system_;
    std::vector<ThermalSourceConfig> source_configs_;
    std::vector<mfem::Array<int>> region_markers_;
    SourceCoefficient source_coefficient_;
    GeometricWeightedCoefficient weighted_source_coefficient_;
};

}
