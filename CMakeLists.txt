cmake_minimum_required(VERSION 3.27)

project(Vulk
  VERSION 1.0.0
  DESCRIPTION "Vulkan C++ wrapper"
)

option(BUILD_TESTBED  "Build the testbed application" ON)
option(VULKAN_DEBUG_UTILS "Enable VK_EXT_debug_utils and the debug macros" OFF)

# @add the search path of CMake modules
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_SOURCE_DIR}/cmake" # cmake functions/macros
  "${CMAKE_SOURCE_DIR}/cmake/modules"# cmake find modules
)

find_package(Vulkan REQUIRED)

set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "")
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "")
add_subdirectory(external/SPIRV-Reflect)
add_library(spirv-reflect::spirv-reflect ALIAS spirv-reflect-static)


set(SRC_FILES
    # Internal helpers
    src/internal/debug.cpp
    # Vulkan core classes
    src/Instance.cpp
    src/PhysicalDevice.cpp
    src/Surface.cpp
    src/Device.cpp
    src/Swapchain.cpp
    src/Image.cpp
    src/Image2D.cpp
    src/DepthImage.cpp
    src/ImageView.cpp
    src/Sampler.cpp
    src/RenderPass.cpp
    src/Framebuffer.cpp
    src/ShaderModule.cpp
    src/VertexShader.cpp
    src/FragmentShader.cpp
    src/ComputeShader.cpp
    src/Pipeline.cpp
    src/Queue.cpp
    src/CommandPool.cpp
    src/CommandBuffer.cpp
    src/DescriptorPool.cpp
    src/DescriptorSet.cpp
    src/DescriptorSetLayout.cpp
    src/Buffer.cpp
    src/StagingBuffer.cpp
    src/VertexBuffer.cpp
    src/IndexBuffer.cpp
    src/UniformBuffer.cpp
    src/StorageBuffer.cpp
    src/DeviceMemory.cpp
    src/Semaphore.cpp
    src/Fence.cpp
    # Engine classes
    src/engine/Context.cpp
    src/engine/Toolbox.cpp
    src/engine/TypeTraits.cpp
    src/engine/Texture2D.cpp
    src/engine/Camera.cpp
)

set(HEADER_FILES
    # Internal helpers
    include/Vulk/internal/base.h
    include/Vulk/internal/arch.h
    include/Vulk/internal/debug.h
    include/Vulk/internal/helpers.h
    # Vulkan core classes
    include/Vulk/Instance.h
    include/Vulk/PhysicalDevice.h
    include/Vulk/Surface.h
    include/Vulk/Device.h
    include/Vulk/Swapchain.h
    include/Vulk/Image.h
    include/Vulk/Image2D.h
    include/Vulk/DepthImage.h
    include/Vulk/ImageView.h
    include/Vulk/Sampler.h
    include/Vulk/RenderPass.h
    include/Vulk/Framebuffer.h
    include/Vulk/ShaderModule.h
    include/Vulk/VertexShader.h
    include/Vulk/FragmentShader.h
    include/Vulk/ComputeShader.h
    include/Vulk/Pipeline.h
    include/Vulk/Queue.h
    include/Vulk/CommandPool.h
    include/Vulk/CommandBuffer.h
    include/Vulk/DescriptorPool.h
    include/Vulk/DescriptorSet.h
    include/Vulk/DescriptorSetLayout.h
    include/Vulk/Buffer.h
    include/Vulk/StagingBuffer.h
    include/Vulk/VertexBuffer.h
    include/Vulk/IndexBuffer.h
    include/Vulk/UniformBuffer.h
    include/Vulk/StorageBuffer.h
    include/Vulk/DeviceMemory.h
    include/Vulk/Semaphore.h
    include/Vulk/Fence.h
    include/Vulk/Exception.h
    # Engine classes
    include/Vulk/engine/Context.h
    include/Vulk/engine/Drawable.h
    include/Vulk/engine/Toolbox.h
    include/Vulk/engine/TypeTraits.h
    include/Vulk/engine/Texture2D.h
    include/Vulk/engine/Vertex.h
    include/Vulk/engine/Camera.h
    include/Vulk/engine/Bound.h

)

add_library(${PROJECT_NAME} SHARED
  ${HEADER_FILES}
  ${SRC_FILES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 20
  POSITION_INDEPENDENT_CODE ON
  COMPILE_WARNING_AS_ERROR ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set(glm_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/glm)
set(stb_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/stb)
set(tinyobjloader_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/tinyobjloader)

target_compile_definitions(${PROJECT_NAME}
  PUBLIC
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS
    GLM_ENABLE_EXPERIMENTAL
)
if(VULKAN_DEBUG_UTILS)
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      ENABLE_VULKAN_DEBUG_UTILS
      ENABLE_VULKAN_DEBUG_LABEL
  )
endif()

target_include_directories(${PROJECT_NAME}
  PUBLIC
    include
    ${glm_INCLUDE_DIR}
    ${stb_INCLUDE_DIR}
    ${tinyobjloader_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Vulkan::Vulkan
    spirv-reflect::spirv-reflect
    # spirv-cross::c
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
      $<$<CONFIG:Debug>: -fsanitize=undefined -fsanitize-trap>
  )
endif()

add_library(Vulk::Vulk ALIAS ${PROJECT_NAME})
if(BUILD_TESTBED)
  add_subdirectory(testbed)
endif()