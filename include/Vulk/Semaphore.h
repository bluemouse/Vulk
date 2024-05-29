#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include <Vulk/internal/base.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class Semaphore : public Sharable<Semaphore>, private NotCopyable {
 public:
  Semaphore() = default;
  explicit Semaphore(const Device& device);
  ~Semaphore() override;

  void create(const Device& device);
  void destroy();

  operator VkSemaphore() const { return _semaphore; }

  [[nodiscard]] bool isCreated() const { return _semaphore != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

 private:
  VkSemaphore _semaphore = VK_NULL_HANDLE;

  std::weak_ptr<const Device> _device;
};

NAMESPACE_END(Vulk)