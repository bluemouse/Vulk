#pragma once

#include <volk/volk.h>

#include <vector>
#include <memory>

#include <Vulk/internal/base.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;
class DescriptorSetLayout;

class DescriptorPool : public Sharable<DescriptorPool>, private NotCopyable {
 public:
  DescriptorPool(const DescriptorSetLayout& layout, uint32_t maxSets, bool setCanBeFreed = true);
  DescriptorPool(const Device& device,
                 std::vector<VkDescriptorPoolSize> poolSizes,
                 uint32_t maxSets,
                 bool setCanBeFreed = true);
  ~DescriptorPool() override;

  void create(const DescriptorSetLayout& layout, uint32_t maxSets, bool setCanBeFreed = true);
  void create(const Device& device,
              std::vector<VkDescriptorPoolSize> poolSizes,
              uint32_t maxSets,
              bool setCanBeFreed = true);
  void destroy();

  void reset();

  operator VkDescriptorPool() const { return _pool; }

  [[nodiscard]] bool isCreated() const { return _pool != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

  [[nodiscard]] bool setCanBeFreed() const { return _setCanBeFreed; }

 private:
  VkDescriptorPool _pool = VK_NULL_HANDLE;

  bool _setCanBeFreed = true;

  std::weak_ptr<const Device> _device;
};

MI_NAMESPACE_END(Vulk)