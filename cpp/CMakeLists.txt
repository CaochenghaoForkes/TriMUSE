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
    fem/finite_element_space_data.cpp
    fem/field_data.cpp
    fem/fem_system.cpp
    physics/thermal/boundary/thermal_boundary_manager.cpp
    physics/thermal/boundary/thermal_robin_boundary.cpp
    physics/thermal/boundary/thermal_neumann_boundary.cpp
    physics/thermal/source/volumetric_power_source.cpp
    physics/thermal/source/thermal_source_manager.cpp
    physics/thermal/thermal_conduction_manager.cpp
    physics/thermal/thermal_capacity_manager.cpp
    physics/thermal/initial/thermal_initial_condition.cpp
    physics/thermal/thermal_problem.cpp
    solver/solver_system.cpp
    solver/cg_solver.cpp
    output/output_manager.cpp
    
)

file(GLOB_RECURSE PROPERTY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/properties/*.cpp"
)

target_sources(trimuse_core PRIVATE
    common/linear_interpolation.cpp
    ${PROPERTY_SOURCES}
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
