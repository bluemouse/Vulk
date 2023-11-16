#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/ShaderModule.h>
#include <Vulk/helpers_vulkan.h>

#include <vector>

NAMESPACE_BEGIN(Vulk)

class Device;

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

 private:
  void moveFrom(VertexShader& rhs);
};

NAMESPACE_END(Vulk)