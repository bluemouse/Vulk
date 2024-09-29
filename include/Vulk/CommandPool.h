#pragma once

#include <volk/volk.h>

#include <memory>

#include <Vulk/internal/base.h>
#include <Vulk/Device.h>

MI_NAMESPACE_BEGIN(Vulk)

class CommandBuffer;
class Queue;

class CommandPool : public Sharable<CommandPool>, private NotCopyable {
 public:
  enum class Mode {
    Transient          = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
    ResetCommandBuffer = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    Default            = 0
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

  [[nodiscard]] bool isCreated() const { return _pool != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }
  [[nodiscard]] const Queue& queue() const { return *_queue.lock(); }

  std::shared_ptr<CommandBuffer> allocatePrimary() const;
  std::shared_ptr<CommandBuffer> allocateSecondary() const;

 private:
  void moveFrom(CommandPool& rhs);

  // Device needs to access this special destroy function to destroy the pools it owns.
  friend class Device;
  void destroy(VkDevice device);

 private:
  VkCommandPool _pool = VK_NULL_HANDLE;

  std::weak_ptr<const Device> _device;
  std::weak_ptr<const Queue> _queue;
};

MI_NAMESPACE_END(Vulk)