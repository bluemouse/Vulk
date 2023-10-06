cmake_minimum_required(VERSION 3.27)

project(Testbed
  DESCRIPTION "Testbed of Vulk library"
)

# @add the search path of CMake modules
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_SOURCE_DIR}/cmake" # cmake functions/macros
  "${CMAKE_SOURCE_DIR}/cmake/modules"# cmake find modules
)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(tinyobjloader REQUIRED)

find_package(PkgConfig)
pkg_get_variable(STB_INCLUDE_DIR stb includedir)

set(SRC_FILES
  main.cpp
  MainWindow.cpp
  Application.cpp
  Testbed.cpp
)

set(HEADER_FILES
  MainWindow.h
  Application.h
  Testbed.h
)

set(SHADER_FILES
  shaders/shader.vert
  shaders/shader.frag
)

set(TEXTURE_FILES
  resources/texture.jpg
)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})

set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)
target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${STB_INCLUDE_DIR}
)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Vulk::Vulk
    glfw
)

include(AddShadersTarget)
add_shaders_target(SPIR-V
  SOURCES ${SHADER_FILES}
  OUTPUT_DIR ${RUNTIME_OUTPUT_DIRECTORY}/shaders
)
add_dependencies(${PROJECT_NAME} SPIR-V)

file(COPY ${TEXTURE_FILES} DESTINATION ${RUNTIME_OUTPUT_DIRECTORY}/textures)
