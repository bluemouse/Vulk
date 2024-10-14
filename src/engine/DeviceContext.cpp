#include <Vulk/engine/DeviceContext.h>

#include <Vulk/internal/debug.h>

namespace {
VkExtent2D chooseDefaultSurfaceExtent(const VkSurfaceCapabilitiesKHR& caps) {
  if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return caps.currentExtent;
  } else {
    return caps.minImageExtent;
  }
}

VkSurfaceFormatKHR chooseDefaultSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR chooseDefaultPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}
} // namespace

MI_NAMESPACE_BEGIN(Vulk)

//
// DeviceContext
//
void DeviceContext::create(const CreateInfo& createInfo) {
  createInstance(createInfo.versionMajor,
                 createInfo.versionMinor,
                 createInfo.instanceExtensions,
                 createInfo.validationLevel);
  createSurface(createInfo.createWindowSurface);
  pickPhysicalDevice(
      createInfo.queueFamilies, createInfo.deviceExtensions, createInfo.hasPhysicalDeviceFeatures);
  createDevice(createInfo.queueFamilies, createInfo.deviceExtensions);
  createSwapchain(
      createInfo.chooseSurfaceExtent, createInfo.chooseSurfaceFormat, createInfo.choosePresentMode);

  _queueFamilies = createInfo.queueFamilies;
}

void DeviceContext::destroy() {
  _swapchain->destroy();
  _device->destroy();
  _surface->destroy();
  _instance->destroy();
}

void DeviceContext::createInstance(int versionMajor,
                                   int versionMinor,
                                   const std::vector<const char*>& extensions,
                                   ValidationLevel validation) {
  _instance = Instance::make_shared(versionMajor, versionMinor, extensions, validation);
}

void DeviceContext::createSurface(const CreateWindowSurfaceFunc& createWindowSurface) {
  _surface = Surface::make_shared(instance(), createWindowSurface(instance()));
}

void DeviceContext::pickPhysicalDevice(
    const PhysicalDevice::QueueFamilies& queueFamilies,
    const std::vector<const char*>& deviceExtensions,
    const PhysicalDevice::HasDeviceFeaturesFunc& hasDeviceFeatures) {
  _instance->pickPhysicalDevice(surface(), queueFamilies, deviceExtensions, hasDeviceFeatures);
}

void DeviceContext::createDevice(const PhysicalDevice::QueueFamilies& requiredQueueFamilies,
                                 const std::vector<const char*>& deviceExtensions) {
  _device = _instance->physicalDevice().createDevice(requiredQueueFamilies, deviceExtensions);
  _device->initQueues();
  _device->initCommandPools();
}

void DeviceContext::createSwapchain(const Swapchain::ChooseSurfaceExtentFunc& chooseSurfaceExtent,
                                    const Swapchain::ChooseSurfaceFormatFunc& chooseSurfaceFormat,
                                    const Swapchain::ChoosePresentModeFunc& choosePresentMode) {
  const auto [capabilities, formats, presentModes] = surface().querySupports();

  VkExtent2D extent = chooseSurfaceExtent ? chooseSurfaceExtent(capabilities)
                                          : chooseDefaultSurfaceExtent(capabilities);

  // TODO should use RenderPass.colorFormat() instead of choosing a format. RenderPass also call
  // chooseSurfaceFormat() to find the format which should be the same one to use.
  VkSurfaceFormatKHR format =
      chooseSurfaceFormat ? chooseSurfaceFormat(formats) : chooseDefaultSurfaceFormat(formats);

  VkPresentModeKHR presentMode =
      choosePresentMode ? choosePresentMode(presentModes) : chooseDefaultPresentMode(presentModes);

  _swapchain = Swapchain::make_shared(device(), surface(), extent, format, presentMode);
}

void DeviceContext::waitIdle() const {
  _device->waitIdle();
}

bool DeviceContext::isComplete() const {
  // TODO: Implement this
  return true;
}

Queue& DeviceContext::queue(Device::QueueFamilyType queueFamily) {
  return _device->queue(queueFamily);
}

const Queue& DeviceContext::queue(Device::QueueFamilyType queueFamily) const {
  return _device->queue(queueFamily);
}

CommandPool& DeviceContext::commandPool(Device::QueueFamilyType queueFamily) {
  return _device->commandPool(queueFamily);
}

const CommandPool& DeviceContext::commandPool(Device::QueueFamilyType queueFamily) const {
  return _device->commandPool(queueFamily);
}

bool DeviceContext::isQueueFamilySupported(Device::QueueFamilyType queueFamily) const {
  switch (queueFamily) {
    case Device::QueueFamilyType::Graphics:
      return _queueFamilies.hasGraphics() || _queueFamilies.hasGraphicsAndCompute();
    case Device::QueueFamilyType::Compute:
      return _queueFamilies.hasCompute() || _queueFamilies.hasGraphicsAndCompute();
    case Device::QueueFamilyType::Transfer: return _queueFamilies.hasTransfer();
    default: break;
  }
  return false;
}

MI_NAMESPACE_END(Vulk)
