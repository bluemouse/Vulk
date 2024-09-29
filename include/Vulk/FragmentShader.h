#pragma once

#include <volk/volk.h>

#include <Vulk/internal/base.h>

#include <Vulk/ShaderModule.h>

MI_NAMESPACE_BEGIN(Vulk)

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

MI_NAMESPACE_END(Vulk)