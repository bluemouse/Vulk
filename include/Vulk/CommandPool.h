#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/helpers_vulkan.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class CommandPool {
 public:
  CommandPool() = default;
  CommandPool(const Device& device, uint32_t queueFamilyIndex);
  ~CommandPool();

  // Transfer the ownership from `rhs` to `this`
  CommandPool(CommandPool&& rhs) noexcept;
  CommandPool& operator=(CommandPool&& rhs) noexcept(false);

  void create(const Device& device, uint32_t queueFamilyIndex);
  void destroy();

  operator VkCommandPool() const { return _pool; }
  [[nodiscard]] VkQueue queue() const { return _queue; }

  [[nodiscard]] const Device& device() const { return *_device; }

  [[nodiscard]] bool isCreated() const { return _pool != VK_NULL_HANDLE; }

 private:
  void moveFrom(CommandPool& rhs);

 private:
  VkCommandPool _pool = VK_NULL_HANDLE;
  VkQueue _queue      = VK_NULL_HANDLE;

  const Device* _device = nullptr;
};

NAMESPACE_END(Vulk)