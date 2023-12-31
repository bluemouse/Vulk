#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

#include <Vulk/internal/base.h>

#include <Vulk/ShaderModule.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class DescriptorSetLayout : public Sharable<DescriptorSetLayout>, private NotCopyable {
 public:
  using DescriptorSetLayoutBinding = ShaderModule::DescriptorSetLayoutBinding;

 public:
  DescriptorSetLayout(const Device& device, std::vector<ShaderModule*> shaders);
  ~DescriptorSetLayout();

  void create(const Device& device, std::vector<ShaderModule*> shaders);
  void destroy();

  operator VkDescriptorSetLayout() const { return _layout; }
  operator const VkDescriptorSetLayout*() const { return &_layout; }

  [[nodiscard]] bool isCreated() const { return _layout != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

  [[nodiscard]] const std::vector<VkDescriptorPoolSize>& poolSizes() const { return _poolSizes; }
  [[nodiscard]] const std::vector<DescriptorSetLayoutBinding>& bindings() const {
    return _bindings;
  }

 private:
  VkDescriptorSetLayout _layout = VK_NULL_HANDLE;

  std::vector<DescriptorSetLayoutBinding> _bindings;
  std::vector<VkDescriptorPoolSize> _poolSizes;

  std::weak_ptr<const Device> _device;
};

NAMESPACE_END(Vulk)