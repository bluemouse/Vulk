#include <Vulk/Context.h>

#include <set>

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

NAMESPACE_Vulk_BEGIN

void Context::create(const CreateInfo& createInfo) {
  createInstance(createInfo.versionMajor,
                 createInfo.versionMinor,
                 createInfo.extensions,
                 createInfo.validationLevel);
  createSurface(createInfo.createWindowSurface);
  pickPhysicalDevice(createInfo.isDeviceSuitable);
  createLogicalDevice();

  createRenderPass(createInfo.chooseSurfaceFormat, createInfo.chooseDepthFormat);
  createSwapchain(
      createInfo.chooseSurfaceExtent, createInfo.chooseSurfaceFormat, createInfo.choosePresentMode);

  createPipeline(createInfo.createVertShader, createInfo.createFragShader);

  createCommandPool();
  createDescriptorPool(createInfo.maxDescriptorSets);
}

void Context::destroy() {
  _descriptorPool.destroy();
  _commandPool.destroy();

  _pipeline.destroy();

  _swapchain.destroy();
  _renderPass.destroy();

  _device.destroy();
  _surface.destroy();
  _instance.destroy();
}

void Context::createInstance(int versionMajor,
                             int versionMinor,
                             const std::vector<const char*>& extensions,
                             ValidationLevel validation) {
  _instance.create(versionMajor, versionMinor, extensions, validation);
}

void Context::createSurface(const CreateWindowSurfaceFunc& createWindowSurface) {
  _surface.create(_instance, createWindowSurface(_instance));
}

void Context::pickPhysicalDevice(const PhysicalDevice::IsDeviceSuitableFunc& isDeviceSuitable) {
  if (isDeviceSuitable) {
    _instance.pickPhysicalDevice(_surface, [this, &isDeviceSuitable](VkPhysicalDevice device) {
      if (!isDeviceSuitable(device)) {
        return false;
      }

      auto queueFamilies = Vulk::PhysicalDevice::findQueueFamilies(device, _surface);
      bool isQueueFamiliesComplete = queueFamilies.graphics && queueFamilies.present;
      return isQueueFamiliesComplete && _surface.isAdequate(device);
    });
  } else {
    _instance.pickPhysicalDevice(_surface, [this](VkPhysicalDevice device) {
      uint32_t extensionCount = 0;
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
      std::vector<VkExtensionProperties> availableExtensions{extensionCount};
      vkEnumerateDeviceExtensionProperties(
          device, nullptr, &extensionCount, availableExtensions.data());
      std::set<std::string> requiredExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
      for (const auto& ext : availableExtensions) {
        requiredExtensions.erase(ext.extensionName);
      }
      bool extensionsSupported = requiredExtensions.empty();

      VkPhysicalDeviceFeatures supportedFeatures;
      vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

      if (!extensionsSupported || supportedFeatures.samplerAnisotropy == 0U) {
        return false;
      }

      auto queueFamilies = Vulk::PhysicalDevice::findQueueFamilies(device, _surface);
      bool isQueueFamiliesComplete = queueFamilies.graphics && queueFamilies.present;
      return isQueueFamiliesComplete && _surface.isAdequate(device);
    });
  }
}

void Context::createLogicalDevice() {
  const auto& queueFamilies = _instance.physicalDevice().queueFamilies();

  _device.create(_instance.physicalDevice(),
                 {queueFamilies.graphicsIndex(), queueFamilies.presentIndex()},
                 {VK_KHR_SWAPCHAIN_EXTENSION_NAME});

  _device.initQueue("graphics", queueFamilies.graphicsIndex());
  _device.initQueue("present", queueFamilies.presentIndex());
}

void Context::createRenderPass(const Swapchain::ChooseSurfaceFormatFunc& chooseSurfaceFormat,
                               const ChooseDepthFormatFunc& chooseDepthFormat) {
  const auto [_, formats, __] = _surface.querySupports();

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

  _renderPass.create(_device, colorFormat, depthStencilFormat);
}

void Context::createSwapchain(const Swapchain::ChooseSurfaceExtentFunc& chooseSurfaceExtent,
                              const Swapchain::ChooseSurfaceFormatFunc& chooseSurfaceFormat,
                              const Swapchain::ChoosePresentModeFunc& choosePresentMode) {
  const auto [capabilities, formats, presentModes] = _surface.querySupports();

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

  _swapchain.create(_device, _surface, extent, format, presentMode);
  _swapchain.createFramebuffers(_renderPass);
}

void Context::createPipeline(const CreateVertShaderFunc& createVertShader,
                             const CreateFragShaderFunc& createFragShader) {
  auto vertShader = createVertShader(_device);
  auto fragShader = createFragShader(_device);
  _pipeline.create(_device, _renderPass, vertShader, fragShader);
}

void Context::createCommandPool() {
  _commandPool.create(_device, _instance.physicalDevice().queueFamilies().graphicsIndex());
}

void Context::createDescriptorPool(uint32_t maxSets) {
  _descriptorPool.create(_pipeline.descriptorSetLayout(), maxSets);
}

void Context::waitIdle() const {
  _device.waitIdle();
}

bool Context::isComplete() const {
  // TODO: Implement this
  return true;
}

NAMESPACE_Vulk_END
