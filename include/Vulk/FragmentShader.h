#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/base.h>

#include <Vulk/ShaderModule.h>

NAMESPACE_BEGIN(Vulk)

class FragmentShader : public ShaderModule {
 public:
  using ShaderModule::ShaderModule;

  void addDescriptorSetLayoutBinding(
      const std::string& name,
      const std::string& type,
      uint32_t binding,
      VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(FragmentShader, ShaderModule);
};

NAMESPACE_END(Vulk)