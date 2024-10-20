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
  RenderTaskRepo.cpp
  #
  apps/App.cpp
  apps/ModelViewer.cpp
  apps/ImageViewer.cpp
)

set(HEADER_FILES
  MainWindow.h
  Testbed.h
  RenderTaskRepo.h
  #
  apps/App.h
  apps/ModelViewer.h
  apps/ImageViewer.h
)

set(GLSL_FILES
  shaders/textureMapping.vert
  shaders/textureMapping.frag
)

file(GLOB TEXTURE_FILES "resources/*.jpg" "resources/*.png")
file(GLOB MODEL_FILES "resources/*.obj")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

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

set(SPIRV_OUTPUT_DIR ${RUNTIME_OUTPUT_DIRECTORY}/shaders)

include(AddSPIRVTarget)
add_SPIRV_target(convert-GLSL-to-SPIRV
  TARGET_ENV vulkan1.0
  SOURCES ${GLSL_FILES}
  OUTPUT_DIR ${SPIRV_OUTPUT_DIR}
)
add_dependencies(${PROJECT_NAME} convert-GLSL-to-SPIRV)

file(COPY ${TEXTURE_FILES} DESTINATION ${RUNTIME_OUTPUT_DIRECTORY}/textures)
file(COPY ${MODEL_FILES} DESTINATION ${RUNTIME_OUTPUT_DIRECTORY}/models)
