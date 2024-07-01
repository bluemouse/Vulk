#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include <Vulk/internal/base.h>
#include <Vulk/Device.h>

NAMESPACE_BEGIN(Vulk)

class CommandPool : public Sharable<CommandPool>, private NotCopyable {
 public:
  CommandPool(const Device& device,
              Device::QueueFamilyType queueFamilyType,
              VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  ~CommandPool() override;

  void create(const Device& device,
              Device::QueueFamilyType queueFamilyType,
              VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  void destroy();

  operator VkCommandPool() const { return _pool; }

  [[nodiscard]] Device::QueueFamilyType queueFamilyType() const { return _queueFamilyType; }
  [[nodiscard]] bool isCreated() const { return _pool != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

 private:
  void moveFrom(CommandPool& rhs);

 private:
  VkCommandPool _pool = VK_NULL_HANDLE;
  Device::QueueFamilyType _queueFamilyType;

  std::weak_ptr<const Device> _device;
};

NAMESPACE_END(Vulk)