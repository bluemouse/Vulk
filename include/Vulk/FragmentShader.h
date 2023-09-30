#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/ShaderModule.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_VULKAN_BEGIN

class FragmentShader : public ShaderModule {
 public:
  using ShaderModule::ShaderModule;

  // Transfer the ownership from `rhs` to `this`
  FragmentShader(FragmentShader&& rhs) noexcept;
  FragmentShader& operator=(FragmentShader&& rhs) noexcept(false);

  void addDescriptorSetLayoutBinding(uint32_t binding,
                                     VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

 private:
  void moveFrom(FragmentShader& rhs);
};

NAMESPACE_VULKAN_END