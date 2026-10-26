#include "fem/fem_system.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

using namespace trimuse;

void check(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}

void test_mesh(mfem::Mesh& mesh, const std::string& path, bool marked, int order, int components)
{
    mfem::Vector center(mesh.SpaceDimension());
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        mesh.GetElementCenter(element, center);
        mesh.GetElement(element)->SetAttribute(center[0] < 0.5 ? 3 : 9);
    }
    mesh.SetAttributes();
    if (marked)
    {
        for (int face = 0; face < mesh.GetNumFaces(); ++face)
        {
            int first, second;
            mesh.GetFaceElements(face, &first, &second);
            if (second >= 0 && mesh.GetAttribute(first) != mesh.GetAttribute(second))
            {
                auto* boundary = mesh.GetFace(face)->Duplicate(&mesh);
                boundary->SetAttribute(41);
                mesh.AddBdrElement(boundary);
            }
        }
        mesh.FinalizeTopology();
    }
    if (order > 1)
    {
        mesh.SetCurvature(order);
        mfem::VectorFunctionCoefficient deformation(mesh.SpaceDimension(), [](const mfem::Vector& x, mfem::Vector& y)
        {
            y = x;
            if (x.Size() > 1) { y[0] += 0.05 * x[0] * (1.0 - x[0]) * x[1] * x[1]; }
        });
        mesh.GetNodes()->ProjectCoefficient(deformation);
    }
    {
        std::ofstream file(path);
        file.precision(17);
        mesh.Print(file);
    }

    FemSystem fem(path, 5, 5);
    fem.add_field("temperature", order);
    fem.add_region_field("concentration", order, components);
    fem.add_region_field("other", 1);
    auto& field = fem.region_field("concentration");
    check(!fem.mesh_data().interfaces().empty(), "missing material interface");
    check(&field.region_mesh(3) == &fem.region_field("other").region_mesh(3), "region mesh not shared between fields");
    check(field.region_mesh(3).GetNE() + field.region_mesh(9).GetNE() == mesh.GetNE(), "missing submesh elements");
    check(field.dof_offsets().Last() > components * fem.finite_element_space("temperature").dof_count(), "interface DOFs not split");
    check(field.dof_offsets().Last() == field.true_dof_offsets().Last(), "unexpected constraints on conforming mesh");
    field.field_data(3).assign(2.0);
    field.field_data(9).assign(7.0);

    mfem::Vector values;
    field.get_values(values);
    check(values.Size() == field.dof_offsets().Last(), "wrong packed vector length");
    for (int dof = 0; dof < values.Size(); ++dof)
    {
        check(values[dof] == (dof < field.dof_offsets()[1] ? 2.0 : 7.0), "wrong block offset");
    }
    values += 1.0;
    field.set_values(values);

    mfem::Array<int> dofs;
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        const int attribute = mesh.GetAttribute(element);
        const auto& submesh = field.region_mesh(attribute);
        check(fem.mesh_data().region_mesh(attribute).parent_element(field.local_element(element)) == element, "wrong parent element map");
        field.element_dofs(element, dofs);
        check(dofs.Size() == field.finite_element_space(attribute).mfem_space().GetFE(field.local_element(element))->GetDof() * components, "wrong element DOF count");
        for (int dof : dofs) { check(values[dof] == (attribute == 3 ? 3.0 : 8.0), "wrong element block DOFs"); }
    }
    for (int face = 0; face < mesh.GetNumFaces(); ++face)
    {
        int first, second;
        mesh.GetFaceElements(face, &first, &second);
        if (second < 0 || mesh.GetAttribute(first) != mesh.GetAttribute(second)) { continue; }
        field.element_dofs(first, dofs);
        const std::set<int> first_dofs(dofs.begin(), dofs.end());
        field.element_dofs(second, dofs);
        int shared = 0;
        for (int dof : dofs) { shared += first_dofs.count(dof); }
        check(shared == components * mfem::H1_FECollection(order, mesh.Dimension()).FiniteElementForGeometry(mesh.GetFaceGeometry(face))->GetDof(), "DOFs inside a region are not H1 continuous");
    }

    for (int index = 0; index < static_cast<int>(fem.mesh_data().interfaces().size()); ++index)
    {
        const auto& interface = fem.mesh_data().interfaces()[index];
        check(interface.boundary_attribute == (marked ? 41 : 0), "lost parent interface attribute");
        std::set<int> first_dofs;
        field.interface_dofs(interface, 0, dofs);
        first_dofs.insert(dofs.begin(), dofs.end());
        field.interface_dofs(interface, 1, dofs);
        for (int dof : dofs) { check(!first_dofs.count(dof), "two sides share a DOF"); }

        auto& transformations = fem.mesh_data().interface_transformations(index);
        check(transformations.Elem1No == interface.sides[0].element && transformations.Elem2No == interface.sides[1].element, "wrong side ordering");
        const auto& rule = mfem::IntRules.Get(mesh.GetFaceGeometry(interface.face), 5);
        for (int q = 0; q < rule.GetNPoints(); ++q)
        {
            const auto& point = rule.IntPoint(q);
            mfem::IntegrationPoint element_points[2];
            transformations.Loc1.Transform(point, element_points[0]);
            transformations.Loc2.Transform(point, element_points[1]);
            mfem::Vector expected(mesh.SpaceDimension());
            transformations.Transform(point, expected);
            for (int side = 0; side < 2; ++side)
            {
                const auto& info = interface.sides[side];
                const int local_element = field.local_element(info.element);
                auto& submesh = field.region_mesh(info.region_attribute);
                mfem::Vector actual(mesh.SpaceDimension());
                submesh.GetElementTransformation(local_element)->Transform(element_points[side], actual);
                actual -= expected;
                check(actual.Norml2() < 1e-12, "face-to-element mapping inconsistent on submesh");
                for (int component = 1; component <= components; ++component)
                {
                    const double trace = field.field_data(info.region_attribute).mfem_grid_function().GetValue(local_element, element_points[side], component);
                    check(std::abs(trace - (info.region_attribute == 3 ? 3.0 : 8.0)) < 1e-12, "interface jump was lost");
                }
                field.interface_dofs(interface, side, dofs);
                check(dofs.Size() == components * mfem::H1_FECollection(order, mesh.Dimension()).FiniteElementForGeometry(mesh.GetFaceGeometry(interface.face))->GetDof(), "wrong trace DOF count");
                if (marked)
                {
                    bool found = false;
                    for (int boundary = 0; boundary < submesh.GetNBE(); ++boundary)
                    {
                        if (submesh.GetBdrElementFaceIndex(boundary) == field.local_face(info.region_attribute, interface.face))
                        {
                            check(submesh.GetBdrAttribute(boundary) == 41, "lost submesh boundary attribute");
                            found = true;
                        }
                    }
                    check(found, "missing regional interface boundary");
                }
            }
        }
    }
    for (int attribute : {3, 9})
    {
        mfem::VectorFunctionCoefficient profile(components, [attribute, components](const mfem::Vector& x, mfem::Vector& value)
        {
            for (int component = 0; component < components; ++component)
            {
                value[component] = attribute + 3.0 * component + (component + 1.0) * x.Sum();
            }
        });
        field.field_data(attribute).mfem_grid_function().ProjectCoefficient(profile);
    }
    for (int index = 0; index < static_cast<int>(fem.mesh_data().interfaces().size()); ++index)
    {
        const auto& interface = fem.mesh_data().interfaces()[index];
        auto& transformations = fem.mesh_data().interface_transformations(index);
        const auto& rule = mfem::IntRules.Get(mesh.GetFaceGeometry(interface.face), 5);
        for (int q = 0; q < rule.GetNPoints(); ++q)
        {
            const auto& point = rule.IntPoint(q);
            mfem::Vector position(mesh.SpaceDimension());
            transformations.Transform(point, position);
            mfem::IntegrationPoint element_points[2];
            transformations.Loc1.Transform(point, element_points[0]);
            transformations.Loc2.Transform(point, element_points[1]);
            for (int side = 0; side < 2; ++side)
            {
                const auto& info = interface.sides[side];
                const int element = field.local_element(info.element);
                for (int component = 1; component <= components; ++component)
                {
                    const double trace = field.field_data(info.region_attribute).mfem_grid_function().GetValue(element, element_points[side], component);
                    const double expected = info.region_attribute + 3.0 * (component - 1) + component * position.Sum();
                    check(std::abs(trace - expected) < 1e-11, "nonconstant trace or vector component orientation incorrect");
                }
            }
        }
    }
    check(fem.field_data("temperature").mfem_grid_function().Norml2() == 0.0, "global H1 field modified");
}

int main(int argc, char** argv)
{
    if (argc != 2) { throw std::runtime_error("usage: region_field_test TEMP_DIRECTORY"); }
    int cases = 0;
    for (int dimension : {1, 2, 3})
    for (bool simplex : {true, false})
    for (bool marked : {false, true})
    for (int order : {1, 2, 3})
    for (int components : {1, 3})
    {
        if (dimension == 1 && !simplex) { continue; }
        auto mesh = dimension == 1 ? mfem::Mesh::MakeCartesian1D(4)
            : dimension == 2 ? mfem::Mesh::MakeCartesian2D(4, 2, simplex ? mfem::Element::TRIANGLE : mfem::Element::QUADRILATERAL)
            : mfem::Mesh::MakeCartesian3D(4, 2, 1, simplex ? mfem::Element::TETRAHEDRON : mfem::Element::HEXAHEDRON);
        const auto path = (std::filesystem::path(argv[1]) / ("region-" + std::to_string(cases) + ".mesh")).string();
        std::cout << "case " << cases << ": dim=" << dimension << " simplex=" << simplex << " marked=" << marked << " order=" << order << " components=" << components << std::endl;
        test_mesh(mesh, path, marked, order, components);
        ++cases;
    }
    std::cout << "PASS: " << cases << " regional H1 / two-sided interface cases\n";
}
