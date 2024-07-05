#include <Vulk/PhysicalDevice.h>

#include <vector>
#include <set>

#include <Vulk/internal/debug.h>

#include <Vulk/Instance.h>
#include <Vulk/Surface.h>
#include <Vulk/Device.h>

NAMESPACE_BEGIN(Vulk)

PhysicalDevice::PhysicalDevice(const Instance& instance,
                               const QueueFamilies& queueFamilies,
                               const std::vector<const char*>& deviceExtensions,
                               const PhysicalDevice::HasDeviceFeaturesFunc& hasPhysicalDeviceFeatures) {
  instantiate(instance, nullptr, queueFamilies, deviceExtensions, hasPhysicalDeviceFeatures);
}

PhysicalDevice::PhysicalDevice(const Instance& instance,
                               const Surface* surface,
                               const QueueFamilies& queueFamilies,
                               const std::vector<const char*>& deviceExtensions,
                               const PhysicalDevice::HasDeviceFeaturesFunc& hasPhysicalDeviceFeatures) {
  instantiate(instance, surface, queueFamilies, deviceExtensions, hasPhysicalDeviceFeatures);
}

PhysicalDevice::~PhysicalDevice() {
  reset();
}

void PhysicalDevice::instantiate(const Instance& instance,
                                 const Surface* surface,
                                 const QueueFamilies& requiredQueueFamilies,
                                 const std::vector<const char*>& requiredDeviceExtensions,
                                 const PhysicalDevice::HasDeviceFeaturesFunc& hasPhysicalDeviceFeatures) {
  MI_VERIFY(!isInstantiated());

  _instance = instance.get_weak();

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  MI_VERIFY_MSG(deviceCount > 0, "Failed to find physical Vulkan devices!");

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  auto isDeviceSuitable = [&requiredQueueFamilies, &requiredDeviceExtensions, hasPhysicalDeviceFeatures]
    (VkPhysicalDevice device, const Surface* surface) {
      if (surface) {
        if (!surface->isAdequate(device)) {
          return false;
        }

        auto queueFamilies = Vulk::PhysicalDevice::findQueueFamilies(device, *surface);
        if (requiredQueueFamilies.graphics && !queueFamilies.graphics) {
          return false;
        }
        if (requiredQueueFamilies.compute && !queueFamilies.compute) {
          return false;
        }
        if (requiredQueueFamilies.transfer && !queueFamilies.transfer) {
          return false;
        }
        if (requiredQueueFamilies.present && !queueFamilies.present) {
          return false;
        }
      }

      uint32_t extensionCount = 0;
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
      std::vector<VkExtensionProperties> availableExtensions{extensionCount};
      vkEnumerateDeviceExtensionProperties(
          device, nullptr, &extensionCount, availableExtensions.data());
      std::set<std::string> requiredExtensions{requiredDeviceExtensions.begin(), requiredDeviceExtensions.end()};
      for (const auto& ext : availableExtensions) {
        requiredExtensions.erase(ext.extensionName);
      }
      if (!requiredExtensions.empty()) {
        return false;
      }

      VkPhysicalDeviceFeatures supportedFeatures;
      vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
      return hasPhysicalDeviceFeatures(supportedFeatures);
    };

  for (const auto& device : devices) {
    if (isDeviceSuitable(device, surface)) {
      _device = device;
      break;
    }
  }

  MI_VERIFY_VKHANDLE(_device);
}

void PhysicalDevice::reset() {
  _device        = VK_NULL_HANDLE;
  _queueFamilies = {};
  _instance.reset();
}

std::shared_ptr<Device> PhysicalDevice::createDevice(const QueueFamilies& requiredQueueFamilies,
                                                     const std::vector<const char*>& extensions) const {
  return Vulk::Device::make_shared(*this, requiredQueueFamilies, extensions);
}

void PhysicalDevice::initQueueFamilies(const Surface& surface) {
  _queueFamilies = findQueueFamilies(_device, surface);
}
void PhysicalDevice::initQueueFamilies() {
  _queueFamilies = findQueueFamilies(_device, VK_NULL_HANDLE);
}

PhysicalDevice::QueueFamilies PhysicalDevice::findQueueFamilies(VkPhysicalDevice device,
                                                                VkSurfaceKHR surface) {
  QueueFamilies queueFamilies;

  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
  if (count > 0) {
    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props.data());

    for (size_t i = 0; i < props.size(); ++i) {
      if (!queueFamilies.graphics && ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U)) {
        queueFamilies.graphics = i;
      }
      if (!queueFamilies.compute && ((props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U)) {
        queueFamilies.compute = i;
      }
      if (!queueFamilies.transfer && ((props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0U)) {
        queueFamilies.transfer = i;
      }

      if (surface != VK_NULL_HANDLE && !queueFamilies.present) {
        VkBool32 presentSupport = 0U;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport != 0U) {
          queueFamilies.present = i;
        }
      }
    }
  }

  return queueFamilies;
}

VkFormat PhysicalDevice::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                             VkImageTiling tiling,
                                             VkFormatFeatureFlags features) const {
  if (tiling == VK_IMAGE_TILING_LINEAR) {
    for (VkFormat format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(*this, format, &props);
      if ((props.linearTilingFeatures & features) == features) {
        return format;
      }
    }
  }

  if (tiling == VK_IMAGE_TILING_OPTIMAL) {
    for (VkFormat format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(*this, format, &props);
      if ((props.optimalTilingFeatures & features) == features) {
        return format;
      }
    }
  }

  throw std::runtime_error("Error: failed to find supported format!");
}

bool PhysicalDevice::isFormatSupported(VkFormat format,
                                       VkImageTiling tiling,
                                       VkFormatFeatureFlags features) const {
  return format == findSupportedFormat({format}, tiling, features);
}

NAMESPACE_END(Vulk)
