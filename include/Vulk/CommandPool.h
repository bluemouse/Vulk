#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include <Vulk/internal/base.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class CommandPool : public Sharable<CommandPool>, private NotCopyable {
 public:
  CommandPool(const Device& device, uint32_t queueFamilyIndex);
  ~CommandPool() override;

  void create(const Device& device, uint32_t queueFamilyIndex);
  void destroy();

  operator VkCommandPool() const { return _pool; }
  [[nodiscard]] VkQueue queue() const { return _queue; }

  [[nodiscard]] bool isCreated() const { return _pool != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

 private:
  void moveFrom(CommandPool& rhs);

 private:
  VkCommandPool _pool = VK_NULL_HANDLE;
  VkQueue _queue      = VK_NULL_HANDLE;

  std::weak_ptr<const Device> _device;
};

NAMESPACE_END(Vulk)