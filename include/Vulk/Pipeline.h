#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include <Vulk/DescriptorSetLayout.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_VULKAN_BEGIN

class Device;
class RenderPass;
class VertexShader;
class FragmentShader;

class Pipeline {
 public:
  Pipeline() = default;
  Pipeline(const Device& device,
           const RenderPass& renderPass,
           const VertexShader& vertShader,
           const FragmentShader& fragShader);
  ~Pipeline();

  // Transfer the ownership from `rhs` to `this`
  Pipeline(Pipeline&& rhs) noexcept;
  Pipeline& operator=(Pipeline&& rhs) noexcept(false);

  void create(const Device& device,
              const RenderPass& renderPass,
              const VertexShader& vertShader,
              const FragmentShader& fragShader);
  void destroy();

  operator VkPipeline() const { return _pipeline; }
  [[nodiscard]] VkPipelineLayout layout() const { return _layout; }

  [[nodiscard]] bool isCreated() const { return _pipeline != VK_NULL_HANDLE; }

  [[nodiscard]] const DescriptorSetLayout& descriptorSetLayout() const {
    return _descriptorSetLayout;
  }

 private:
  void moveFrom(Pipeline& rhs);

 private:
  VkPipeline _pipeline = VK_NULL_HANDLE;
  VkPipelineLayout _layout = VK_NULL_HANDLE;

  DescriptorSetLayout _descriptorSetLayout;

  const Device* _device = nullptr;
};

NAMESPACE_VULKAN_END