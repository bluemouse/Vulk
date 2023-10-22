#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <map>

#include <Vulk/PhysicalDevice.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_Vulk_BEGIN

class Device {
 public:
  using DeviceCreateInfoOverride = std::function<
      void(VkDeviceCreateInfo*, VkPhysicalDeviceFeatures*, std::vector<VkDeviceQueueCreateInfo>*)>;

 public:
  Device() = default;
  Device(const PhysicalDevice& physicalDevice,
         const std::vector<uint32_t>& queueFamilies,
         const std::vector<const char*>& extensions = {},
         const DeviceCreateInfoOverride& override = {});
  ~Device();

  // Transfer the ownership from `rhs` to `this`
  Device(Device&& rhs) noexcept;
  Device& operator=(Device&& rhs) noexcept(false);

  void create(const PhysicalDevice& physicalDevice,
              const std::vector<uint32_t>& queueFamilies,
              const std::vector<const char*>& extensions = {},
              const DeviceCreateInfoOverride& override = {});
  void destroy();

  void waitIdle() const;

  operator VkDevice() const { return _device; }

  [[nodiscard]] const Instance& instance() const { return physicalDevice().instance(); }
  [[nodiscard]] const PhysicalDevice& physicalDevice() const { return *_physicalDevice; }

  void initQueue(const std::string& queueName, uint32_t queueFamilyIndex);
  [[nodiscard]] VkQueue queue(const std::string& queueName) const;

  [[nodiscard]] bool isCreated() const { return _device != VK_NULL_HANDLE; }

 private:
  void moveFrom(Device& rhs);

 private:
  VkDevice _device = VK_NULL_HANDLE;

  std::map<std::string /*queueName*/, VkQueue> _queues;

  const PhysicalDevice* _physicalDevice = nullptr;
};

NAMESPACE_Vulk_END