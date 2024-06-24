#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>

#include <Vulk/internal/base.h>

NAMESPACE_BEGIN(Vulk)

class Instance;
class PhysicalDevice;

class Device : public Sharable<Device>, private NotCopyable {
 public:
  using DeviceCreateInfoOverride = std::function<
      void(VkDeviceCreateInfo*, VkPhysicalDeviceFeatures*, std::vector<VkDeviceQueueCreateInfo>*)>;

  enum QueueFamilyName { Graphics, Compute, Transfer, Present };

 public:
  Device(const PhysicalDevice& physicalDevice,
         const std::vector<uint32_t>& queueFamilies,
         const std::vector<const char*>& extensions = {},
         const DeviceCreateInfoOverride& override   = {});
  ~Device() override;

  void create(const PhysicalDevice& physicalDevice,
              const std::vector<uint32_t>& queueFamilies,
              const std::vector<const char*>& extensions = {},
              const DeviceCreateInfoOverride& override   = {});
  void destroy();

  void waitIdle() const;

  operator VkDevice() const { return _device; }

  [[nodiscard]] const Instance& instance() const;
  [[nodiscard]] const PhysicalDevice& physicalDevice() const { return *_physicalDevice.lock(); }

  void initQueue(QueueFamilyName queueFamilyName, uint32_t queueFamilyIndex);
  [[nodiscard]] VkQueue queue(QueueFamilyName queueFamilyName) const;
  [[nodiscard]] std::optional<uint32_t> queueIndex(QueueFamilyName queueFamilyName) const;
  [[nodiscard]] std::vector<uint32_t> queueIndices() const;

  [[nodiscard]] bool isCreated() const { return _device != VK_NULL_HANDLE; }

 private:
  VkDevice _device = VK_NULL_HANDLE;

  struct QueueFamily {
    uint32_t index;
    VkQueue queue;
  };
  std::map<QueueFamilyName, QueueFamily> _queues;

  std::weak_ptr<const PhysicalDevice> _physicalDevice;
};

NAMESPACE_END(Vulk)