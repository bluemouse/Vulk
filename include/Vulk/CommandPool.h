#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include <Vulk/internal/base.h>
#include <Vulk/Device.h>

NAMESPACE_BEGIN(Vulk)

class CommandBuffer;

class CommandPool : public Sharable<CommandPool>, private NotCopyable {
 public:
  enum class Mode {
    Transient = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
    ResetCommandBuffer =  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    Default = 0
  };

 public:
  CommandPool(const Device& device,
              Device::QueueFamilyType queueFamilyType,
              Mode modes = Mode::ResetCommandBuffer);
  ~CommandPool() override;

  void create(const Device& device,
              Device::QueueFamilyType queueFamilyType,
              Mode modes = Mode::ResetCommandBuffer);

  void destroy();

  operator VkCommandPool() const { return _pool; }

  [[nodiscard]] Device::QueueFamilyType queueFamilyType() const { return _queueFamilyType; }
  [[nodiscard]] bool isCreated() const { return _pool != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

  std::shared_ptr<CommandBuffer> allocatePrimary() const;
  std::shared_ptr<CommandBuffer> allocateSecondary() const;

 private:
  void moveFrom(CommandPool& rhs);

  // Device needs to access this special destroy function to destroy the pools it owns.
  friend class Device;
  void destroy(VkDevice device);

 private:
  VkCommandPool _pool = VK_NULL_HANDLE;
  Device::QueueFamilyType _queueFamilyType;

  std::weak_ptr<const Device> _device;
};

NAMESPACE_END(Vulk)