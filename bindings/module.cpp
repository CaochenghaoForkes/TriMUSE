#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>
#include <tuple>

#include "fem/fem_system.hpp"
#include "output/output_manager.hpp"
#include "physics/thermal/thermal_problem.hpp"
#include "properties/properties_system.hpp"
#include "solver/solver_system.hpp"

namespace py = pybind11;

namespace
{

using ThermalConfigEntry = std::tuple<int, std::string, trimuse::ModelParameterMap>;
using LinearSolverEntry = std::tuple<std::string, std::string, double, int>;
using NonlinearSolverEntry = std::tuple<std::string, double, double, int>;
using FieldSolverEntry = std::tuple<std::string, LinearSolverEntry, NonlinearSolverEntry>;

template <typename Config>
std::vector<Config> thermal_configs(const std::vector<ThermalConfigEntry>& entries)
{
    std::vector<Config> configs;
    configs.reserve(entries.size());
    for (const auto& [attribute, model_name, parameters] : entries)
    {
        configs.push_back({attribute, model_name, parameters});
    }
    return configs;
}

}

PYBIND11_MODULE(_core, module)
{
    module.doc() = "TriMUSE finite element core";

    py::class_<trimuse::FemSystem>(module, "FemSystem")
        .def(
            py::init<const std::string&, int, int>(),
            py::arg("mesh_path"),
            py::arg("domain_quadrature_order"),
            py::arg("boundary_quadrature_order")
        )
        .def(
            "add_field",
            &trimuse::FemSystem::add_field,
            py::arg("name"),
            py::arg("order"),
            py::arg("vector_dimension") = 1
        );
    
    py::class_<trimuse::OutputManager>(module, "OutputManager")
        .def(py::init<trimuse::FemSystem&, const std::string&>(),
             py::arg("fem_system"), py::arg("output_directory"), py::keep_alive<1, 2>())
        .def("save", &trimuse::OutputManager::save,
             py::arg("cycle"), py::arg("time"), py::arg("time_step"));

    py::class_<trimuse::PropertiesSystem>(module, "PropertiesSystem")
        .def(py::init<const trimuse::FemSystem&>(), py::arg("fem_system"), py::keep_alive<1, 2>())
        .def(
            "assign",
            &trimuse::PropertiesSystem::assign,
            py::arg("attribute"),
            py::arg("model_name"),
            py::arg("parameters")
        )
        .def("prepare_dependencies", &trimuse::PropertiesSystem::prepare_dependencies)
        .def("update_external", &trimuse::PropertiesSystem::update_external, py::arg("time"))
        .def("update_derived", &trimuse::PropertiesSystem::update_derived)
        .def("update_material", &trimuse::PropertiesSystem::update_material)
        .def("update_field_dependent_materials", [](trimuse::PropertiesSystem& system, const std::string& field_name)
        {
            system.update_field_dependent_materials(field_name);
        }, py::arg("field_name"))
        .def("accept_current_state", &trimuse::PropertiesSystem::accept_current_state);

    py::class_<trimuse::SolverSystem>(module, "SolverSystem")
        .def(py::init([](const std::vector<FieldSolverEntry>& entries)
        {
            std::vector<trimuse::FieldSolverConfig> configs;
            configs.reserve(entries.size());
            for (const auto& [name, linear, nonlinear] : entries)
            {
                const auto& [linear_method, preconditioner, linear_tolerance, linear_iterations] = linear;
                const auto& [nonlinear_method, nonlinear_tolerance, absolute_tolerance, nonlinear_iterations] = nonlinear;
                configs.push_back({name,
                    {linear_method, preconditioner, linear_tolerance, linear_iterations},
                    {nonlinear_method, nonlinear_tolerance, absolute_tolerance, nonlinear_iterations}});
            }
            return std::make_unique<trimuse::SolverSystem>(configs);
        }), py::arg("configs"));

    py::class_<trimuse::ThermalProblem>(module, "ThermalProblem")
        .def(py::init([](trimuse::FemSystem& fem, trimuse::PropertiesSystem& properties,
                         trimuse::SolverSystem& solver, const std::string& problem_type,
                         const std::vector<ThermalConfigEntry>& initial_configs,
                         const std::vector<ThermalConfigEntry>& source_configs,
                         const std::vector<ThermalConfigEntry>& boundary_configs)
        {
            return std::make_unique<trimuse::ThermalProblem>(fem, fem.field_data("temperature"), properties, solver, problem_type,
                thermal_configs<trimuse::ThermalInitialConfig>(initial_configs),
                thermal_configs<trimuse::ThermalSourceConfig>(source_configs),
                thermal_configs<trimuse::ThermalBoundaryConfig>(boundary_configs));
        }), py::arg("fem_system"), py::arg("properties_system"), py::arg("solver_system"), py::arg("problem_type"),
            py::arg("initial_configs"), py::arg("source_configs"), py::arg("boundary_configs"),
            py::keep_alive<1, 2>(), py::keep_alive<1, 3>(), py::keep_alive<1, 4>())
        .def("solve", &trimuse::ThermalProblem::solve, py::arg("time_step") = 0.0, py::arg("theta") = 1.0);
}
