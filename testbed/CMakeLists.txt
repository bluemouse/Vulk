cmake_minimum_required(VERSION 3.27)

project(Testbed
  DESCRIPTION "Testbed of Vulk library"
)

find_package(glfw3 REQUIRED)
add_library(glfw::glfw ALIAS glfw)

set(SRC_FILES
  main.cpp
  MainWindow.cpp
  Testbed.cpp
)

set(HEADER_FILES
  MainWindow.h
  Testbed.h
)

set(SHADER_FILES
  shaders/shader.vert
  shaders/shader.frag
)

set(TEXTURE_FILES
  resources/ScreenTestPattern.png
)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})

set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Vulk::Vulk
    glfw::glfw
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
