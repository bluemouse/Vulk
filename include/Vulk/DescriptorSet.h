#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/helpers_vulkan.h>

#include <vector>

NAMESPACE_Vulk_BEGIN

class DescriptorPool;
class DescriptorSetLayout;

class DescriptorSet {
 public:
  struct Binding {
    VkDescriptorBufferInfo* bufferInfo = nullptr;
    VkDescriptorImageInfo* imageInfo = nullptr;

    Binding(VkDescriptorBufferInfo* bufferInfo) : bufferInfo(bufferInfo) {}
    Binding(VkDescriptorImageInfo* imageInfo) : imageInfo(imageInfo) {}
  };

 public:
  DescriptorSet() = default;
  DescriptorSet(const DescriptorPool& pool,
                const DescriptorSetLayout& layout,
                const std::vector<Binding>& bindings);
  ~DescriptorSet();

  // Transfer the ownership from `rhs` to `this`
  DescriptorSet(DescriptorSet&& rhs) noexcept;
  DescriptorSet& operator=(DescriptorSet&& rhs) noexcept(false);

  void allocate(const DescriptorPool& pool,
                const DescriptorSetLayout& layout,
                const std::vector<Binding>& bindings);
  void free();

  operator VkDescriptorSet() const { return _set; }
  operator const VkDescriptorSet*() const { return &_set; }

  [[nodiscard]] bool isAllocated() const { return _set != VK_NULL_HANDLE; }

 private:
  void moveFrom(DescriptorSet& rhs);

 private:
  VkDescriptorSet _set = VK_NULL_HANDLE;

  const DescriptorPool* _pool = nullptr;
};

NAMESPACE_Vulk_END