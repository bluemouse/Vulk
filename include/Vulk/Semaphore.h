#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/helpers_vulkan.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class Semaphore {
 public:
  Semaphore() = default;
  explicit Semaphore(const Device& device);
  ~Semaphore();

  // Transfer the ownership from `rhs` to `this`
  Semaphore(Semaphore&& rhs) noexcept;
  Semaphore& operator=(Semaphore&& rhs) noexcept(false);

  void create(const Device& device);
  void destroy();

  operator VkSemaphore() const { return _semaphore; }
  operator const VkSemaphore*() const { return &_semaphore; }

  [[nodiscard]] bool isCreated() const { return _semaphore != VK_NULL_HANDLE; }

 private:
  void moveFrom(Semaphore& rhs);

 private:
  VkSemaphore _semaphore = VK_NULL_HANDLE;

  const Device* _device = nullptr;
};

NAMESPACE_END(Vulk)