find_package(MPI REQUIRED COMPONENTS CXX)

find_path(
    MFEM_INCLUDE_DIR
    NAMES mfem.hpp
    HINTS ENV CONDA_PREFIX
    PATH_SUFFIXES include
    REQUIRED
)

find_library(
    MFEM_LIBRARY
    NAMES mfem
    HINTS ENV CONDA_PREFIX
    PATH_SUFFIXES lib
    REQUIRED
)

add_library(
    trimuse_core
    STATIC
    fem/mesh_data.cpp
    fem/region_mesh_data.cpp
    fem/finite_element_space_data.cpp
    fem/field_data.cpp
    fem/field_view.cpp
    fem/region_field_data.cpp
    fem/fem_system.cpp
    physics/radioactivity/boundary/radioactivity_boundary_manager.cpp
    physics/radioactivity/boundary/radioactivity_robin_boundary.cpp
    physics/radioactivity/boundary/radioactivity_neumann_boundary.cpp
    physics/radioactivity/boundary/radioactivity_dirichlet_boundary.cpp
    physics/radioactivity/source/radioactivity_source_manager.cpp
    physics/radioactivity/source/volumetric_nuclide_birth_rate_source.cpp
    physics/radioactivity/initial/radioactivity_initial_condition.cpp
    physics/radioactivity/radioactivity_capacity_manager.cpp
    physics/radioactivity/radioactivity_diffusion_manager.cpp
    physics/radioactivity/radioactivity_problem.cpp
    solver/solver_system.cpp
    solver/cg_solver.cpp
    output/output_manager.cpp
    
)

file(GLOB_RECURSE PROPERTY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/properties/*.cpp"
)
file(GLOB_RECURSE THERMAL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/physics/thermal/*.cpp"
)

target_sources(trimuse_core PRIVATE
    common/linear_interpolation.cpp
    ${PROPERTY_SOURCES}
    ${THERMAL_SOURCES}
)

target_include_directories(
    trimuse_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MFEM_INCLUDE_DIR}
)

target_link_libraries(
    trimuse_core
    PUBLIC
    ${MFEM_LIBRARY}
    MPI::MPI_CXX
)

# macOS 的 Conda MFEM 使用随包 libc++; 扩展也应链接同一运行库。
get_filename_component(MFEM_LIBRARY_DIRECTORY "${MFEM_LIBRARY}" DIRECTORY)
if(APPLE AND EXISTS "${MFEM_LIBRARY_DIRECTORY}/libc++.dylib")
    target_link_libraries(trimuse_core PUBLIC "${MFEM_LIBRARY_DIRECTORY}/libc++.dylib")
endif()

set_target_properties(
    trimuse_core
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

option(TRIMUSE_BUILD_THERMAL_TESTS "Build thermal assembly/interface regression checks" OFF)
if(TRIMUSE_BUILD_THERMAL_TESTS)
    add_executable(trimuse_thermal_test ../tests/thermal_regression.cpp)
    target_link_libraries(trimuse_thermal_test PRIVATE trimuse_core)
endif()
