#include <Vulk/PhysicalDevice.h>

#include <vector>

#include <Vulk/Instance.h>
#include <Vulk/Surface.h>

NAMESPACE_Vulk_BEGIN

PhysicalDevice::PhysicalDevice(const Instance& instance,
                               const IsDeviceSuitableFunc& isDeviceSuitable) {
  instantiate(instance, isDeviceSuitable);
}

PhysicalDevice::~PhysicalDevice() {
  reset();
}

void PhysicalDevice::instantiate(const Instance& instance,
                                 const IsDeviceSuitableFunc& isDeviceSuitable) {
  MI_VERIFY(!isInstantiated());

  _instance = &instance;

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  MI_VERIFY_MSG(deviceCount > 0, "Failed to find physical Vulkan devices!");

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
      _device = device;
      break;
    }
  }

  MI_VERIFY_VKHANDLE(_device);
}

void PhysicalDevice::reset() {
  _device = VK_NULL_HANDLE;
  _queueFamilies = {};
  _instance = nullptr;
}

void PhysicalDevice::initQueueFamilies(const Surface& surface) {
  _queueFamilies = findQueueFamilies(_device, surface);
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

      if (!queueFamilies.present) {
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

NAMESPACE_Vulk_END
