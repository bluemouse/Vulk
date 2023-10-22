#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/ShaderModule.h>
#include <Vulk/helpers_vulkan.h>

#include <vector>

NAMESPACE_Vulk_BEGIN

class VertexShader : public ShaderModule {
 public:
  using ShaderModule::ShaderModule;

  // Transfer the ownership from `rhs` to `this`
  VertexShader(VertexShader&& rhs) noexcept;
  VertexShader& operator=(VertexShader&& rhs) noexcept(false);

  void addVertexInputBinding(uint32_t binding,
                             uint32_t stride,
                             VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);

  void addVertexInputBindings(std::vector<VkVertexInputBindingDescription> bindings);

  void addVertexInputAttribute(uint32_t location,
                               uint32_t binding,
                               VkFormat format,
                               uint32_t offset = 0);
  void addVertexInputAttributes(std::vector<VkVertexInputAttributeDescription> attributes);

  void addDescriptorSetLayoutBinding(uint32_t binding,
                                     VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

  const std::vector<VkVertexInputBindingDescription>& vertexInputBindings() const {
    return _vertexInputBindings;
  }

  const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributes() const {
    return _vertexInputAttributes;
  }

 private:
  void moveFrom(VertexShader& rhs);

 private:
  std::vector<VkVertexInputBindingDescription> _vertexInputBindings;
  std::vector<VkVertexInputAttributeDescription> _vertexInputAttributes;
};

NAMESPACE_Vulk_END