#include <Vulk/Surface.h>

#include <Vulk/Instance.h>
#include <Vulk/internal/debug.h>

MI_NAMESPACE_BEGIN(Vulk)

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
  _instance = instance.get_weak();

  _surface = surface;
}

void Surface::destroy() {
  MI_VERIFY(isCreated());

  vkDestroySurfaceKHR(instance(), _surface, nullptr);

  _surface = VK_NULL_HANDLE;
  _instance.reset();
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
  return querySupports(instance().physicalDevice());
}

bool Surface::isAdequate(VkPhysicalDevice physicalDevice) const {
  const auto supports = querySupports(physicalDevice);
  return !supports.formats.empty() && !supports.presentModes.empty();
}

MI_NAMESPACE_END(Vulk)
