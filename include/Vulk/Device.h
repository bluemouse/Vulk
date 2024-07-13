#pragma once

#include <volk/volk.h>

#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>

#include <Vulk/internal/base.h>
#include <Vulk/PhysicalDevice.h>

NAMESPACE_BEGIN(Vulk)

class Instance;
class Queue;
class CommandPool;

class Device : public Sharable<Device>, private NotCopyable {
 public:
  using DeviceCreateInfoOverride = std::function<
      void(VkDeviceCreateInfo*, VkPhysicalDeviceFeatures*, std::vector<VkDeviceQueueCreateInfo>*)>;

  enum QueueFamilyType { Graphics = 0, Compute, Transfer, Present, NUM_QUEUE_FAMILY_TYPES };

 public:
  Device(const PhysicalDevice& physicalDevice,
         const PhysicalDevice::QueueFamilies& requiredQueueFamilies,
         const std::vector<const char*>& extensions = {},
         const DeviceCreateInfoOverride& override   = {});
  ~Device() override;

  void create(const PhysicalDevice& physicalDevice,
              const PhysicalDevice::QueueFamilies& requiredQueueFamilies,
              const std::vector<const char*>& extensions = {},
              const DeviceCreateInfoOverride& override   = {});
  void initQueues();
  void initCommandPools();
  void destroy();

  void waitIdle() const;

  operator VkDevice() const { return _device; }

  [[nodiscard]] const Instance& instance() const;
  [[nodiscard]] const PhysicalDevice& physicalDevice() const { return *_physicalDevice.lock(); }

  [[nodiscard]] Queue& queue(QueueFamilyType queueFamilyType);
  [[nodiscard]] const Queue& queue(QueueFamilyType queueFamilyType) const;
  [[nodiscard]] std::optional<uint32_t> queueFamilyIndex(QueueFamilyType queueFamilyType) const;
  [[nodiscard]] std::vector<uint32_t> queueFamilyIndices() const;

  [[nodiscard]] CommandPool& commandPool(QueueFamilyType queueFamilyType);
  [[nodiscard]] const CommandPool& commandPool(QueueFamilyType queueFamilyType) const;

  [[nodiscard]] bool isCreated() const { return _device != VK_NULL_HANDLE; }

  void setObjectName(VkObjectType type, uint64_t object, const char* name);

 private:
  VkDevice _device = VK_NULL_HANDLE;

  struct QueueFamily {
    QueueFamilyType type;
    uint32_t index;
  };
  std::vector<QueueFamily> _queueFamilies;

  std::vector<std::shared_ptr<Queue>> _queues{NUM_QUEUE_FAMILY_TYPES};
  std::vector<std::shared_ptr<CommandPool>> _commandPools{NUM_QUEUE_FAMILY_TYPES};

  std::weak_ptr<const PhysicalDevice> _physicalDevice;
};

NAMESPACE_END(Vulk)