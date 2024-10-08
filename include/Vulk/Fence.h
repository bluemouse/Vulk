#pragma once

#include <volk/volk.h>

#include <memory>

#include <Vulk/internal/base.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;

class Fence : public Sharable<Fence>, private NotCopyable {
 public:
  Fence() = default;
  explicit Fence(const Device& device, bool signaled = false);
  ~Fence() override;

  void create(const Device& device, bool signaled = false);
  void destroy();

  operator VkFence() const { return _fence; }
  operator const VkFence*() const { return &_fence; }

  [[nodiscard]] bool isCreated() const { return _fence != VK_NULL_HANDLE; }

  void wait(uint64_t timeout = UINT64_MAX) const;
  void reset();

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

 private:
  VkFence _fence = VK_NULL_HANDLE;

  std::weak_ptr<const Device> _device;
};

MI_NAMESPACE_END(Vulk)