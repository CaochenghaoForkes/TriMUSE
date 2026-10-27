#include "physics/thermal/thermal_problem.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace trimuse;
namespace
{
int checks = 0;
std::string context;
std::filesystem::path directory;

void check(bool condition, const std::string& message)
{
    ++checks;
    if (!condition) { throw std::runtime_error(context + ": " + message); }
}
void close(double actual, double expected, double tolerance = 1e-7)
{
    check(std::abs(actual - expected) <= tolerance,
          "value mismatch: " + std::to_string(actual) + " / " + std::to_string(expected)
          + ", difference x1e9=" + std::to_string((actual - expected) * 1e9));
}

std::string mesh_file(int dimension, mfem::Element::Type geometry, bool curved, bool scattered = false)
{
    auto mesh = dimension == 1 ? mfem::Mesh::MakeCartesian1D(32)
        : dimension == 2 ? mfem::Mesh::MakeCartesian2D(4, 4, geometry)
        : mfem::Mesh::MakeCartesian3D(2, 2, 2, geometry);
    mfem::Vector center(dimension);
    for (int e = 0; e < mesh.GetNE(); ++e)
    {
        mesh.GetElementCenter(e, center);
        mesh.GetElement(e)->SetAttribute((scattered ? e % 3 == 0 : center[dimension - 1] < 0.5) ? 3 : 17);
    }
    for (int b = 0; b < mesh.GetNBE(); ++b)
    {
        auto* t = mesh.GetBdrElementTransformation(b);
        t->Transform(mfem::Geometries.GetCenter(t->GetGeometryType()), center);
        mesh.GetBdrElement(b)->SetAttribute(center[dimension - 1] < 1e-10 ? 1
            : center[dimension - 1] > 1 - 1e-10 ? 2 : 4);
    }
    // 内界面和外边界都有显式物理标记, 确认内部边界不会被误当成外表面。
    for (int f = 0; f < mesh.GetNumFaces(); ++f)
    {
        int a, b; mesh.GetFaceElements(f, &a, &b);
        if (b < 0 || mesh.GetAttribute(a) == mesh.GetAttribute(b)) { continue; }
        if (dimension == 1) { mesh.AddBdrPoint(f, 41); }
        else
        {
            auto* face = mesh.GetFace(f)->Duplicate(&mesh);
            face->SetAttribute(41); mesh.AddBdrElement(face);
        }
    }
    mesh.FinalizeTopology();
    mesh.SetAttributes();
    if (curved)
    {
        mesh.SetCurvature(2);
        mesh.Transform([](const mfem::Vector& x, mfem::Vector& y)
        {
            y = x;
            for (int c = 0; c < x.Size(); ++c) { y[c] += 0.03 * (c + 1) * x[c] * (1.0 - x[c]); }
        });
    }
    const auto file = directory / ("mesh-" + std::to_string(dimension) + "-" + std::to_string(geometry)
        + "-" + std::to_string(curved) + "-" + std::to_string(scattered));
    std::ofstream stream(file); stream.precision(17); mesh.Print(stream);
    return file.string();
}

FieldSolverConfig solver_config()
{
    FieldSolverConfig config; config.name = "temperature";
    config.linear.preconditioner = "symmetric_gauss_seidel";
    config.linear.relative_tolerance = 1e-15;
    config.linear.max_iterations = 5000;
    config.nonlinear.relative_tolerance = 1e-12;
    config.nonlinear.absolute_tolerance = 1e-10;
    config.nonlinear.max_iterations = 100;
    return config;
}
void materials(PropertiesSystem& properties, bool nonlinear = false)
{
    for (int region : {3, 17})
    {
        properties.assign(region, nonlinear ? "material.thermal.conductivity.kernel.bison_2021_uco"
            : "material.thermal.conductivity.common.constant", {{"constant_value", region == 3 ? 2.0 : 3.0}});
        properties.assign(region, "material.general.mass_density.common.constant", {{"constant_value", 5.0}});
        properties.assign(region, "material.thermal.specific_heat.common.constant", {{"constant_value", 10.0}});
    }
    properties.prepare_dependencies();
}
void update(PropertiesSystem& properties, double time)
{
    properties.update_external(time); properties.update_derived(); properties.update_material();
    properties.update_field_dependent_materials("temperature");
}
void compare(FemSystem& a, FemSystem& b)
{
    const auto first = a.field_view("temperature"), second = b.field_view("temperature");
    for (int e = 0; e < a.mesh_data().element_count(); ++e)
    {
        const auto& mesh = a.mesh_data().mfem_mesh();
        const auto& rule = *a.domain_quadrature_rule(mesh.GetElementBaseGeometry(e));
        for (int q = 0; q < rule.GetNPoints(); ++q)
        {
            close(std::get<double>(first.evaluate(mesh.GetAttribute(e), e, rule.IntPoint(q))),
                  std::get<double>(second.evaluate(mesh.GetAttribute(e), e, rule.IntPoint(q))), 3e-7);
        }
    }
}

void perfect_contact(const std::string& file, int order, double theta, bool nonlinear, const std::string& type = "transient")
{
    context = file + "/p" + std::to_string(order) + "/theta" + std::to_string(theta) + "/nonlinear" + std::to_string(nonlinear) + "/" + type;
    FemSystem continuous(file, 2 * order + 6, 2 * order + 4), regional(file, 2 * order + 6, 2 * order + 4);
    continuous.add_field("temperature", order); regional.add_region_field("temperature", order);
    PropertiesSystem pc(continuous), pr(regional);
    for (auto* p : {&pc, &pr})
    {
        p->assign(2, "derived.heat_transfer_coef.common.constant", {{"constant_value", 3.0}});
        p->assign(2, "external.environment_temperature.common.constant", {{"constant_value", 290.0}});
        materials(*p, nonlinear);
    }
    SolverSystem solver({solver_config()});
    OutputManager oc(continuous, directory.string()), or_(regional, directory.string());
    const std::vector<ThermalInitialConfig> initial{{3, "uniform", {{"uniform_value", 300.0}}},
                                                  {17, "uniform", {{"uniform_value", 360.0}}}};
    const std::vector<ThermalBoundaryConfig> boundary{{2, "robin", {}}};
    ThermalProblem tc(continuous, continuous.field_data("temperature"), pc, solver, oc, type, initial, {}, boundary);
    ThermalProblem tr(regional, regional.region_field("temperature"), pr, solver, or_, type, initial, {}, boundary);
    compare(continuous, regional); // 两种布局下初始阶跃的 L2 投影相同。
    for (int step = 0; step < 4; ++step)
    {
        context += "/step" + std::to_string(step);
        update(pc, step * 1e-5); update(pr, step * 1e-5);
        tc.solve(step == 0 ? 0 : 1e-5, theta); tr.solve(step == 0 ? 0 : 1e-5, theta);
        compare(continuous, regional);
        pc.accept_current_state(); pr.accept_current_state();
    }
    if (!nonlinear)
    {
        check(tr.assembly_counts().at("conduction") == 2, "unchanged conduction reassembled");
        check(tr.assembly_counts().at("capacity") == 2, "unchanged capacity reassembled");
    }
}

void interface_solution(int dimension, mfem::Element::Type geometry, int order)
{
    context = "interface " + std::to_string(dimension) + "/" + std::to_string(geometry);
    FemSystem fem(mesh_file(dimension, geometry, false), 2 * order + 6, 2 * order + 4);
    fem.add_region_field("temperature", order);
    PropertiesSystem properties(fem);
    std::vector<ThermalDomainConfig> sources;
    std::vector<ThermalBoundaryConfig> boundaries{{2, "dirichlet", {}}};
    if (dimension == 1)
    {
        properties.assign(3, "derived.volumetric_power.common.constant", {{"constant_value", 24.0}});
        properties.assign(2, "external.surface_temperature.common.constant", {{"constant_value", 300.0}});
        sources.push_back({3, "volumetric_power", {}});
    }
    else
    {
        properties.assign(1, "external.surface_temperature.common.constant", {{"constant_value", 400.0}});
        properties.assign(2, "external.surface_temperature.common.constant", {{"constant_value", 400.0 - 0.5 - 1.0 - 1.0 / 3.0}});
        boundaries.push_back({1, "dirichlet", {}});
    }
    materials(properties);
    SolverSystem solver({solver_config()}); OutputManager output(fem, directory.string());
    ThermalProblem problem(fem, fem.region_field("temperature"), properties, solver, output, "steady",
        {{3, "uniform", {{"uniform_value", 300.0}}}, {17, "uniform", {{"uniform_value", 300.0}}}},
        sources, boundaries, {{41, "conductance", {{"heat_transfer_coef", 2.0}}}});
    update(properties, 0); problem.solve();
    const auto view = fem.field_view("temperature");
    for (std::size_t i = 0; i < fem.mesh_data().interfaces().size(); ++i)
    {
        const auto& face = fem.mesh_data().interfaces()[i];
        auto& t = fem.mesh_data().interface_transformations(i);
        const auto& rule = *fem.boundary_quadrature_rule(t.GetGeometryType());
        for (int q = 0; q < rule.GetNPoints(); ++q)
        {
            t.SetAllIntPoints(&rule.IntPoint(q));
            double values[2];
            for (int side = 0; side < 2; ++side)
            {
                values[side] = std::get<double>(view.evaluate(face.sides[side].region_attribute, face.sides[side].element,
                    side == 0 ? t.GetElement1IntPoint() : t.GetElement2IntPoint()));
            }
            const double jump = (face.sides[0].region_attribute == 3 ? 1 : -1) * (values[0] - values[1]);
            close(jump, dimension == 1 ? 2.0 : 1.0, 1e-7); // q=h[T], 两侧通量平衡。
        }
    }
    if (dimension > 1)
    {
        for (int e = 0; e < fem.mesh_data().element_count(); ++e)
        {
            const int region = fem.mesh_data().mfem_mesh().GetAttribute(e);
            const auto& ip = mfem::Geometries.GetCenter(fem.mesh_data().mfem_mesh().GetElementBaseGeometry(e));
            mfem::Vector x(dimension); fem.mesh_data().mfem_mesh().GetElementTransformation(e)->Transform(ip, x);
            const double expected = region == 3 ? 400.0 - x[dimension - 1] : 398.5 - 2.0 / 3.0 * (x[dimension - 1] - 0.5);
            close(std::get<double>(view.evaluate(region, e, ip)), expected, 1e-7);
        }
    }
    output.add_field("temperature", "interface-output-" + std::to_string(dimension) + "-" + std::to_string(geometry));
    output.save("temperature", 0, 0.0, 0.0);
}

void cache_invalidation(const std::string& file)
{
    context = "cache " + file;
    FemSystem fem(file, 6, 5); fem.add_region_field("temperature", 2);
    ThermalField field(fem, fem.region_field("temperature"));
    ThermalAssembly assembly(fem, field, {{3, "volumetric_power", {}}}, {{2, "robin", {}}}, {});
    PropertiesFieldData properties;
    for (const auto& name : {"material.thermal.conductivity", "material.general.mass_density",
                             "material.thermal.specific_heat", "derived.volumetric_power"})
    { properties.integration_point_values[name].resize(fem.domain_quadrature_point_count(), StateValue{2.0}); }
    properties.zone_values["derived.heat_transfer_coef"][2] = StateValue{3.0};
    properties.zone_values["external.environment_temperature"][2] = StateValue{300.0};
    const auto original = assembly.update(properties, true);
    const auto counts = assembly.counts();
    const auto same = assembly.update(properties, true);
    check(assembly.counts() == counts && original.mass == same.mass && original.conduction == same.conduction && original.load == same.load,
          "unchanged operators not reused");
    properties.zone_values["external.environment_temperature"][2] = StateValue{310.0};
    const auto warm = assembly.update(properties, true);
    check(warm.conduction == original.conduction && warm.mass == original.mass && warm.load != original.load,
          "Robin environment changed matrix");
    const int e = fem.mesh_data().region_elements(3).front();
    properties.integration_point_values["material.thermal.conductivity"][fem.domain_quadrature_offset(e)] = StateValue{4.0};
    const auto modified = assembly.update(properties, true);
    check(modified.conduction != warm.conduction && modified.mass == warm.mass, "conductivity invalidation failed");
    check(assembly.counts().at("conduction") == counts.at("conduction") + 1, "unaffected region reassembled");
    const double old_mass_norm = original.mass->MaxNorm();
    properties.integration_point_values["material.general.mass_density"][fem.domain_quadrature_offset(e)] = StateValue{5.0};
    const auto dense = assembly.update(properties, true);
    check(dense.mass != modified.mass && dense.conduction == modified.conduction, "capacity invalidation failed");
    close(original.mass->MaxNorm(), old_mass_norm, 0.0); // 历史矩阵不被覆盖。
    properties.zone_values["derived.heat_transfer_coef"][2] = StateValue{4.0};
    const auto robin = assembly.update(properties, true);
    check(robin.conduction != dense.conduction && robin.load != dense.load && robin.mass == dense.mass, "Robin invalidation failed");
}

void insulated(const std::string& file)
{
    context = "insulated " + file;
    FemSystem fem(file, 6, 5); fem.add_region_field("temperature", 2);
    PropertiesSystem properties(fem); materials(properties);
    SolverSystem solver({solver_config()}); OutputManager output(fem, directory.string());
    ThermalProblem problem(fem, fem.region_field("temperature"), properties, solver, output, "transient",
        {{3, "uniform", {{"uniform_value", 300.0}}}, {17, "uniform", {{"uniform_value", 500.0}}}},
        {}, {}, {{41, "insulated", {}}});
    for (int step = 0; step < 4; ++step)
    {
        update(properties, step); problem.solve(step == 0 ? 0.0 : 0.1, 0.5);
        for (int region : {3, 17})
        {
            const auto& values = fem.region_field("temperature").field_data(region).mfem_grid_function();
            for (int j = 0; j < values.Size(); ++j) { close(values[j], region == 3 ? 300.0 : 500.0); }
        }
        properties.accept_current_state();
    }
}

void nonconforming()
{
    context = "continuous nonconforming mesh";
    const auto file = (directory / "nonconforming.mesh").string();
    {
        auto mesh = mfem::Mesh::MakeCartesian2D(4, 4, mfem::Element::QUADRILATERAL);
        mfem::Array<int> refine(1); refine[0] = 0; mesh.GeneralRefinement(refine);
        std::ofstream stream(file); mesh.Print(stream);
    }
    FemSystem fem(file, 8, 6); fem.add_field("temperature", 2);
    check(fem.mesh_data().mfem_mesh().Nonconforming(), "test mesh has no hanging nodes");
    PropertiesSystem properties(fem);
    properties.assign(1, "material.thermal.conductivity.common.constant", {{"constant_value", 2.0}});
    properties.assign(1, "material.general.mass_density.common.constant", {{"constant_value", 5.0}});
    properties.assign(1, "material.thermal.specific_heat.common.constant", {{"constant_value", 10.0}});
    // MFEM 2D 笛卡尔网格: 下边界 1, 上边界 3。
    properties.assign(1, "external.surface_temperature.common.constant", {{"constant_value", 300.0}});
    properties.assign(3, "external.surface_temperature.common.constant", {{"constant_value", 400.0}});
    properties.prepare_dependencies();
    SolverSystem solver({solver_config()}); OutputManager output(fem, directory.string());
    ThermalProblem problem(fem, fem.field_data("temperature"), properties, solver, output, "transient",
        {{1, "uniform", {{"uniform_value", 300.0}}}}, {}, {{1, "dirichlet", {}}, {3, "dirichlet", {}}});
    mfem::FunctionCoefficient exact([](const mfem::Vector& x) { return 300.0 + 100.0 * x[1]; });
    fem.field_data("temperature").mfem_grid_function().ProjectCoefficient(exact);
    for (int step = 0; step < 3; ++step)
    {
        update(properties, step); problem.solve(step == 0 ? 0.0 : 0.1);
        close(fem.field_data("temperature").mfem_grid_function().ComputeL2Error(exact), 0.0, 1e-8);
        properties.accept_current_state();
    }
}
}

int main(int argc, char** argv)
{
    if (argc != 2) { std::cerr << "usage: thermal_regression ARTIFACT_DIRECTORY\n"; return 2; }
    directory = argv[1]; std::filesystem::create_directories(directory);
    for (const auto& [dimension, geometry] : std::vector<std::pair<int, mfem::Element::Type>>{
        {1, mfem::Element::SEGMENT}, {2, mfem::Element::TRIANGLE}, {2, mfem::Element::QUADRILATERAL},
        {3, mfem::Element::TETRAHEDRON}, {3, mfem::Element::HEXAHEDRON}})
    {
        for (bool curved : {false, true})
        for (int order : {1, 2})
        for (double theta : {0.0, 0.5, 1.0})
        {
            perfect_contact(mesh_file(dimension, geometry, curved, true), order, theta, false);
        }
        perfect_contact(mesh_file(dimension, geometry, true), 2, 0.5, true);
        perfect_contact(mesh_file(dimension, geometry, true), 2, 1.0, true, "steady");
        interface_solution(dimension, geometry, 2);
        const auto file = mesh_file(dimension, geometry, false);
        cache_invalidation(file);
        insulated(file);
    }
    nonconforming();
    std::cout << "PASS: " << checks << " thermal checks\n";
}
