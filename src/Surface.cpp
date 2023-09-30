#include <Vulk/Surface.h>

#include <Vulk/Instance.h>

NAMESPACE_VULKAN_BEGIN

Surface::Surface(const Instance& instance, VkSurfaceKHR surface) {
  create(instance, surface);
}

Surface::~Surface() {
  if (isCreated()) {
    destroy();
  }
}

void Surface::create(const Instance& instance, VkSurfaceKHR surface) {
  MI_VERIFY(!isCreated());
  _instance = &instance;

  _surface = surface;
}

void Surface::destroy() {
  MI_VERIFY(isCreated());

  vkDestroySurfaceKHR(*_instance, _surface, nullptr);

  _surface = VK_NULL_HANDLE;
  _instance = nullptr;
}

Surface::Supports Surface::querySupports(VkPhysicalDevice physicalDevice) const {
  Supports supports;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, &supports.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &formatCount, nullptr);
  if (formatCount > 0) {
    supports.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice, _surface, &formatCount, supports.formats.data());
  }

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &presentModeCount, nullptr);
  if (presentModeCount > 0) {
    supports.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, _surface, &presentModeCount, supports.presentModes.data());
  }

  return supports;
}

Surface::Supports Surface::querySupports() const {
  return querySupports(_instance->physicalDevice());
}

NAMESPACE_VULKAN_END
