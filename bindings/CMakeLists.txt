find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
# 优先使用当前 Python 环境中的 pybind11, 不依赖本机绝对路径。
execute_process(
    COMMAND "${Python_EXECUTABLE}" -m pybind11 --cmakedir
    OUTPUT_VARIABLE PYBIND11_CMAKE_DIRECTORY
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(PYBIND11_CMAKE_DIRECTORY)
    list(PREPEND CMAKE_PREFIX_PATH "${PYBIND11_CMAKE_DIRECTORY}")
endif()
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(_core module.cpp)

target_link_libraries(_core PRIVATE trimuse_core)
