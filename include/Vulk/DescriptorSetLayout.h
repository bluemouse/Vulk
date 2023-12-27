#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/base.h>
#include <Vulk/internal/vulkan_debug.h>

#include <Vulk/ShaderModule.h>

#include <vector>

NAMESPACE_BEGIN(Vulk)

class Device;

class DescriptorSetLayout {
 public:
  using DescriptorSetLayoutBinding = ShaderModule::DescriptorSetLayoutBinding;

 public:
  DescriptorSetLayout() = default;
  DescriptorSetLayout(const Device& device, std::vector<ShaderModule*> shaders);
  ~DescriptorSetLayout();

  // Transfer the ownership from `rhs` to `this`
  DescriptorSetLayout(DescriptorSetLayout&& rhs) noexcept;
  DescriptorSetLayout& operator=(DescriptorSetLayout&& rhs) noexcept(false);

  void create(const Device& device, std::vector<ShaderModule*> shaders);
  void destroy();

  operator VkDescriptorSetLayout() const { return _layout; }
  operator const VkDescriptorSetLayout*() const { return &_layout; }

  [[nodiscard]] bool isCreated() const { return _layout != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device; }

  [[nodiscard]] const std::vector<VkDescriptorPoolSize>& poolSizes() const { return _poolSizes; }
  [[nodiscard]] const std::vector<DescriptorSetLayoutBinding>& bindings() const {
    return _bindings;
  }

 private:
  void moveFrom(DescriptorSetLayout& rhs);

 private:
  VkDescriptorSetLayout _layout = VK_NULL_HANDLE;

  std::vector<DescriptorSetLayoutBinding> _bindings;
  std::vector<VkDescriptorPoolSize> _poolSizes;

  const Device* _device = nullptr;
};

NAMESPACE_END(Vulk)