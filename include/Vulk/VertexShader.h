#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include <Vulk/internal/base.h>

#include <Vulk/ShaderModule.h>

NAMESPACE_BEGIN(Vulk)

class VertexShader : public ShaderModule {
 public:
  using ShaderModule::ShaderModule;

  void addVertexInputBinding(uint32_t binding,
                             uint32_t stride,
                             VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);

  void addVertexInputBindings(std::vector<VkVertexInputBindingDescription> bindings);

  void addVertexInputAttribute(const std::string& name,
                               const std::string& type,
                               uint32_t location,
                               uint32_t binding,
                               VkFormat format,
                               uint32_t offset = 0);
  void addVertexInputAttributes(std::vector<VertexInputAttribute> attributes);

  void addDescriptorSetLayoutBinding(
      const std::string& name,
      const std::string& type,
      uint32_t binding,
      VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

  const std::vector<VkVertexInputBindingDescription>& vertexInputBindings() const {
    return _vertexInputBindings;
  }

  const std::vector<VertexInputAttribute>& vertexInputAttributes() const {
    return _vertexInputAttributes;
  }

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(VertexShader, ShaderModule);
};

NAMESPACE_END(Vulk)