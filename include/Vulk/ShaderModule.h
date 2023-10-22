#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/helpers_vulkan.h>

#include <vector>

NAMESPACE_Vulk_BEGIN

class Device;

class ShaderModule {
 public:
  ShaderModule() = default;
  ShaderModule(const Device& device, const std::vector<char>& codes, const char* entry = "main");
  ShaderModule(const Device& device, const char* shaderFile, const char* entry = "main");
  virtual ~ShaderModule();

  void create(const Device& device, const std::vector<char>& codes, const char* entry = "main");
  void create(const Device& device, const char* shaderFile, const char* entry = "main");
  void destroy();

  // Transfer the ownership from `rhs` to `this`
  ShaderModule(ShaderModule&& rhs) noexcept;
  ShaderModule& operator=(ShaderModule&& rhs) noexcept(false);

  operator VkShaderModule() const { return _shader; }

  [[nodiscard]] bool isCreated() const { return _shader != VK_NULL_HANDLE; }

  void addDescriptorSetLayoutBinding(uint32_t binding,
                                     VkDescriptorType descriptorType,
                                     VkShaderStageFlags stageFlags);

  [[nodiscard]] const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings() const {
    return _descriptorSetLayoutBindings;
  }

  [[nodiscard]] const char* entry() const { return _entry; }


 protected:
  void moveFrom(ShaderModule& rhs);

 protected:
  VkShaderModule _shader = VK_NULL_HANDLE;

  const char* _entry = nullptr;
  std::vector<VkDescriptorSetLayoutBinding> _descriptorSetLayoutBindings;

  const Device* _device;
};

NAMESPACE_Vulk_END