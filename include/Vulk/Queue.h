#pragma once

#include <vulkan/vulkan.h>


#include <Vulk/internal/base.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class Queue : public Sharable<Queue>, private NotCopyable {
 public:
  Queue(const Device& device, uint32_t queueFamilyIndex);
  ~Queue() override;

  void destroy();

  operator VkQueue() const { return _queue; }

  uint32_t queueFamilyIndex() const { return _queueFamilyIndex; }
  uint32_t queueIndex() const { return _queueIndex; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

 private:
  VkQueue _queue      = VK_NULL_HANDLE;

  uint32_t _queueFamilyIndex;
  uint32_t _queueIndex;

  std::weak_ptr<const Device> _device;
};

NAMESPACE_END(Vulk)