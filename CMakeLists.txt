cmake_minimum_required(VERSION 3.27)

project(Vulk
  VERSION 1.0.0
  DESCRIPTION "Vulkan C++ wrapper"
)

option(BUILD_TESTBED  "Build the testbed application" ON)

find_package(Vulkan REQUIRED)

set(SRC_FILES
    src/helpers_debug.cpp
    src/Instance.cpp
    src/PhysicalDevice.cpp
    src/Surface.cpp
    src/Device.cpp
    src/Swapchain.cpp
    src/Image.cpp
    src/ImageView.cpp
    src/Sampler.cpp
    src/RenderPass.cpp
    src/Framebuffer.cpp
    src/ShaderModule.cpp
    src/VertexShader.cpp
    src/FragmentShader.cpp
    src/Pipeline.cpp
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
    src/DeviceMemory.cpp
    src/Semaphore.cpp
    src/Fence.cpp
)

set(HEADER_FILES
    include/Vulk/helpers_debug.h
    include/Vulk/helpers_vulkan.h
    include/Vulk/Instance.h
    include/Vulk/PhysicalDevice.h
    include/Vulk/Surface.h
    include/Vulk/Device.h
    include/Vulk/Swapchain.h
    include/Vulk/Image.h
    include/Vulk/ImageView.h
    include/Vulk/Sampler.h
    include/Vulk/RenderPass.h
    include/Vulk/Framebuffer.h
    include/Vulk/ShaderModule.h
    include/Vulk/VertexShader.h
    include/Vulk/FragmentShader.h
    include/Vulk/Pipeline.h
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
    include/Vulk/DeviceMemory.h
    include/Vulk/Semaphore.h
    include/Vulk/Fence.h
)

add_library(${PROJECT_NAME} SHARED
  ${HEADER_FILES}
  ${SRC_FILES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  POSITION_INDEPENDENT_CODE ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    include
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Vulkan::Vulkan
)

target_compile_options(${PROJECT_NAME}
  PRIVATE
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. # make __FILE__ relative to project root
)

add_library(Vulk::Vulk ALIAS ${PROJECT_NAME})
if(BUILD_TESTBED)
  add_subdirectory(testbed)
endif()