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
  using shared_ptr = std::shared_ptr<FragmentShader>;
  using weak_ptr   = std::weak_ptr<FragmentShader>;

  template <class... Args>
  static shared_ptr make_shared(Args&&... args) {
    return std::make_shared<FragmentShader>(std::forward<Args>(args)...);
  }

  shared_ptr get_shared() {
    return std::static_pointer_cast<FragmentShader>(ShaderModule::get_shared());
  }
  weak_ptr get_weak() {
    return std::static_pointer_cast<FragmentShader>(ShaderModule::get_weak().lock());
  }
};

NAMESPACE_END(Vulk)