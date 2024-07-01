#pragma once

#include <vulkan/vulkan.h>

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
  void destroy();

  void waitIdle() const;

  operator VkDevice() const { return _device; }

  [[nodiscard]] const Instance& instance() const;
  [[nodiscard]] const PhysicalDevice& physicalDevice() const { return *_physicalDevice.lock(); }

  [[nodiscard]] const Queue& queue(QueueFamilyType queueFamilyType) const;
  [[nodiscard]] std::optional<uint32_t> queueFamilyIndex(QueueFamilyType queueFamilyType) const;
  [[nodiscard]] std::vector<uint32_t> queueFamilyIndices() const;

  [[nodiscard]] bool isCreated() const { return _device != VK_NULL_HANDLE; }

 private:
  std::shared_ptr<Queue> getQueue(uint32_t queueFamilyIndex);

 private:
  VkDevice _device = VK_NULL_HANDLE;

  std::vector<std::shared_ptr<Queue>> _queues{NUM_QUEUE_FAMILY_TYPES};

  std::weak_ptr<const PhysicalDevice> _physicalDevice;
};

NAMESPACE_END(Vulk)