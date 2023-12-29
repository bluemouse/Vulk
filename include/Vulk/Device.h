#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <map>
#include <memory>
#include <functional>

#include <Vulk/internal/base.h>
#include <Vulk/internal/vulkan_debug.h>

NAMESPACE_BEGIN(Vulk)

class Instance;
class PhysicalDevice;

class Device : public Sharable<Device>, private NotCopyable {
 public:
  using DeviceCreateInfoOverride = std::function<
      void(VkDeviceCreateInfo*, VkPhysicalDeviceFeatures*, std::vector<VkDeviceQueueCreateInfo>*)>;

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

  void initQueue(const std::string& queueName, uint32_t queueFamilyIndex);
  [[nodiscard]] VkQueue queue(const std::string& queueName) const;

  [[nodiscard]] bool isCreated() const { return _device != VK_NULL_HANDLE; }

 private:
  VkDevice _device = VK_NULL_HANDLE;

  std::map<std::string /*queueName*/, VkQueue> _queues;

  std::weak_ptr<const PhysicalDevice> _physicalDevice;
};

NAMESPACE_END(Vulk)