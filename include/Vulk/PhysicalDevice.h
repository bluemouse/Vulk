#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <optional>
#include <vector>

#include <Vulk/internal/base.h>
#include <Vulk/internal/vulkan_debug.h>

NAMESPACE_BEGIN(Vulk)

class Instance;
class Surface;

class PhysicalDevice {
 public:
  struct QueueFamilies {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;
    std::optional<uint32_t> present;

    uint32_t graphicsIndex() const { return graphics.value(); }
    uint32_t computeIndex() const { return compute.value(); }
    uint32_t transferIndex() const { return transfer.value(); }
    uint32_t presentIndex() const { return present.value(); }
  };

  using IsDeviceSuitableFunc = std::function<bool(VkPhysicalDevice)>;

 public:
  PhysicalDevice() = default;
  PhysicalDevice(const Instance& instance, const IsDeviceSuitableFunc& isDeviceSuitable);
  ~PhysicalDevice();

  void instantiate(const Instance& instance, const IsDeviceSuitableFunc& isDeviceSuitable);
  void reset();

  void initQueueFamilies(const Surface& surface);

  operator VkPhysicalDevice() const { return _device; }

  [[nodiscard]] const QueueFamilies& queueFamilies() const { return _queueFamilies; }

  [[nodiscard]] const Instance& instance() const { return *_instance; }

  [[nodiscard]] static QueueFamilies findQueueFamilies(VkPhysicalDevice device,
                                                       VkSurfaceKHR surface);

  // Disable copy and assignment operators
  PhysicalDevice(const PhysicalDevice&)            = delete;
  PhysicalDevice& operator=(const PhysicalDevice&) = delete;

  [[nodiscard]] bool isInstantiated() const { return _device != VK_NULL_HANDLE; }

  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features) const;

  bool isFormatSupported(VkFormat format,
                         VkImageTiling tiling,
                         VkFormatFeatureFlags features) const;

 private:
  [[nodiscard]] bool isDeviceSuitable(VkPhysicalDevice device);

 private:
  VkPhysicalDevice _device = VK_NULL_HANDLE;
  QueueFamilies _queueFamilies;

  const Instance* _instance = nullptr;
};

NAMESPACE_END(Vulk)