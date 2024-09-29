#pragma once

#include <volk/volk.h>

#include <vector>
#include <memory>
#include <string>

#include <Vulk/internal/base.h>

MI_NAMESPACE_BEGIN(Vulk)

class DescriptorPool;
class DescriptorSetLayout;

class DescriptorSet : public Sharable<DescriptorSet>, private NotCopyable {
 public:
  struct Binding {
    std::string name;
    std::string type;

    VkDescriptorBufferInfo* bufferInfo = nullptr;
    VkDescriptorImageInfo* imageInfo   = nullptr;

    Binding(const std::string& name, const std::string& type, VkDescriptorBufferInfo* bufferInfo)
        : name(name), type(type), bufferInfo(bufferInfo) {}
    Binding(const std::string& name, const std::string& type, VkDescriptorImageInfo* imageInfo)
        : name(name), type(type), imageInfo(imageInfo) {}
  };

 public:
  DescriptorSet() = default;
  DescriptorSet(const DescriptorPool& pool, const DescriptorSetLayout& layout);
  DescriptorSet(const DescriptorPool& pool,
                const DescriptorSetLayout& layout,
                const std::vector<Binding>& bindings);
  ~DescriptorSet();

  void allocate(const DescriptorPool& pool, const DescriptorSetLayout& layout);
  void free();

  void bind(const std::vector<Binding>& bindings) const;

  operator VkDescriptorSet() const { return _set; }
  operator const VkDescriptorSet*() const { return &_set; }

  [[nodiscard]] bool isAllocated() const { return _set != VK_NULL_HANDLE; }

  [[nodiscard]] const DescriptorPool& pool() const { return *_pool.lock(); }

 private:
  VkDescriptorSet _set = VK_NULL_HANDLE;
  std::weak_ptr<const DescriptorSetLayout> _layout;

  std::weak_ptr<const DescriptorPool> _pool;
};

MI_NAMESPACE_END(Vulk)