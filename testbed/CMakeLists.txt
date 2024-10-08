cmake_minimum_required(VERSION 3.27)

project(Testbed
  DESCRIPTION "Testbed of Vulk library"
)

find_package(glfw3 REQUIRED)
add_library(glfw::glfw ALIAS glfw)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/cxxopts external/cxxopts)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/tinyobjloader external/tinyobjloader)
add_library(tinyobjloader::tinyobjloader ALIAS tinyobjloader)

set(SRC_FILES
  main.cpp
  MainWindow.cpp
  Testbed.cpp
  RenderTask.cpp
)

set(HEADER_FILES
  MainWindow.h
  Testbed.h
  RenderTask.h
)

set(SHADER_FILES
  shaders/shader.vert
  shaders/shader.frag
)

file(GLOB TEXTURE_FILES "resources/*.jpg" "resources/*.png")
file(GLOB MODEL_FILES "resources/*.obj")

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})

set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 20
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Vulk::Vulk
    glfw::glfw
    cxxopts::cxxopts
    tinyobjloader::tinyobjloader
)

if(MSVC)
  target_compile_options(${PROJECT_NAME}
    PRIVATE
      /W4 /WX
  )
else()
  target_compile_options(${PROJECT_NAME}
    PRIVATE
      -Wall -Wextra -Wpedantic -Werror
      -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. # make __FILE__ relative to project root
  )
endif()

include(AddShadersTarget)
add_shaders_target(SPIR-V
  SOURCES ${SHADER_FILES}
  OUTPUT_DIR ${RUNTIME_OUTPUT_DIRECTORY}/shaders
)
add_dependencies(${PROJECT_NAME} SPIR-V)

file(COPY ${TEXTURE_FILES} DESTINATION ${RUNTIME_OUTPUT_DIRECTORY}/textures)
file(COPY ${MODEL_FILES} DESTINATION ${RUNTIME_OUTPUT_DIRECTORY}/models)
