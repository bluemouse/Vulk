#include <Vulk/engine/Context.h>

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

NAMESPACE_BEGIN(Vulk)

void Context::create(const CreateInfo& createInfo) {
  createInstance(createInfo.versionMajor,
                 createInfo.versionMinor,
                 createInfo.instanceExtensions,
                 createInfo.validationLevel);
  createSurface(createInfo.createWindowSurface);
  pickPhysicalDevice(createInfo.queueFamilies,
                     createInfo.deviceExtensions,
                     createInfo.hasPhysicalDeviceFeatures);
  createDevice(createInfo.queueFamilies, createInfo.deviceExtensions);
  createSwapchain(createInfo.chooseSurfaceExtent,
                  createInfo.chooseSurfaceFormat,
                  createInfo.choosePresentMode);

  createRenderPass(createInfo.chooseSurfaceFormat, createInfo.chooseDepthFormat);
  createPipeline(createInfo.createVertShader, createInfo.createFragShader);
  createDescriptorPool(createInfo.maxDescriptorSets);
}

void Context::destroy() {
  _descriptorPool->destroy();
  _pipeline->destroy();
  _renderPass->destroy();

  _swapchain->destroy();
  _commandPool->destroy();
  _device->destroy();
  _surface->destroy();
  _instance->destroy();
}

void Context::createInstance(int versionMajor,
                             int versionMinor,
                             const std::vector<const char*>& extensions,
                             ValidationLevel validation) {
  _instance = Vulk::Instance::make_shared(versionMajor, versionMinor, extensions, validation);
}

void Context::createSurface(const CreateWindowSurfaceFunc& createWindowSurface) {
  _surface = Vulk::Surface::make_shared(instance(), createWindowSurface(instance()));
}

void Context::pickPhysicalDevice(const PhysicalDevice::QueueFamilies& queueFamilies,
                                 const std::vector<const char*>& deviceExtensions,
                                 const PhysicalDevice::HasDeviceFeaturesFunc& hasDeviceFeatures) {
  _instance->pickPhysicalDevice(surface(), queueFamilies, deviceExtensions, hasDeviceFeatures);
}

void Context::createDevice(const PhysicalDevice::QueueFamilies& requiredQueueFamilies,
                           const std::vector<const char*>& deviceExtensions) {
  _device = Vulk::Device::make_shared(_instance->physicalDevice(), requiredQueueFamilies, deviceExtensions);
  //TODO We hard coded graphics queue. Need to make it customizable.
  _commandPool = Vulk::CommandPool::make_shared(device(), Device::QueueFamilyType::Graphics);

}

void Context::createRenderPass(const Swapchain::ChooseSurfaceFormatFunc& chooseSurfaceFormat,
                               const ChooseDepthFormatFunc& chooseDepthFormat) {
  const auto [_, formats, __] = surface().querySupports();

  VkFormat colorFormat;
  if (chooseSurfaceFormat) {
    colorFormat = chooseSurfaceFormat(formats).format;
  } else {
    colorFormat = chooseDefaultSurfaceFormat(formats).format;
  }

  VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;
  if (chooseDepthFormat) {
    depthStencilFormat = chooseDepthFormat();
    MI_VERIFY(depthStencilFormat != VK_FORMAT_UNDEFINED);
  }

  _renderPass = Vulk::RenderPass::make_shared(device(), colorFormat, depthStencilFormat);
}

void Context::createSwapchain(const Swapchain::ChooseSurfaceExtentFunc& chooseSurfaceExtent,
                              const Swapchain::ChooseSurfaceFormatFunc& chooseSurfaceFormat,
                              const Swapchain::ChoosePresentModeFunc& choosePresentMode) {
  const auto [capabilities, formats, presentModes] = surface().querySupports();

  VkExtent2D extent;
  if (chooseSurfaceExtent) {
    extent = chooseSurfaceExtent(capabilities);
  } else {
    extent = chooseDefaultSurfaceExtent(capabilities);
  }

  // TODO should use RenderPass.colorFormat() instead of choosing a format. RenderPass also call
  // chooseSurfaceFormat() to find the format which should be the same one to use.
  VkSurfaceFormatKHR format;
  if (chooseSurfaceFormat) {
    format = chooseSurfaceFormat(formats);
  } else {
    format = chooseDefaultSurfaceFormat(formats);
  }

  VkPresentModeKHR presentMode;
  if (choosePresentMode) {
    presentMode = choosePresentMode(presentModes);
  } else {
    presentMode = chooseDefaultPresentMode(presentModes);
  }

  _swapchain = Vulk::Swapchain::make_shared(device(), surface(), extent, format, presentMode);
}

void Context::createPipeline(const CreateVertShaderFunc& createVertShader,
                             const CreateFragShaderFunc& createFragShader) {
  const Device& device = this->device();
  auto vertShader      = createVertShader(device);
  auto fragShader      = createFragShader(device);

  _pipeline = Vulk::Pipeline::make_shared(device, renderPass(), vertShader, fragShader);
}

void Context::createDescriptorPool(uint32_t maxSets) {
  _descriptorPool = Vulk::DescriptorPool::make_shared(_pipeline->descriptorSetLayout(), maxSets);
}

void Context::waitIdle() const {
  _device->waitIdle();
}

bool Context::isComplete() const {
  // TODO: Implement this
  return true;
}

NAMESPACE_END(Vulk)
