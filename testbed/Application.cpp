#include "Application.h"

#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>
#include <limits>

namespace {
#ifdef NDEBUG
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                            uint32_t width,
                            uint32_t height) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    MI_LOG_INFO("capabilities.currentExtent = %d x %d",
                capabilities.currentExtent.width,
                capabilities.currentExtent.height);
    return capabilities.currentExtent;
  } else {
    width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    height =
        std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    MI_LOG_INFO("Extent = %d x %d", width, height);
    return {width, height};
  }
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device,
                                 const std::vector<const char *> &extensions) {
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(
      device, nullptr, &extensionCount, availableExtensions.data());

  std::set<std::string> requiredExtensions{extensions.begin(), extensions.end()};

  for (const auto &ext : availableExtensions) {
    requiredExtensions.erase(ext.extensionName);
  }

  return requiredExtensions.empty();
}

bool isDeviceSuitable(VkPhysicalDevice device, const Vulkan::Surface &surface) {
  auto queueFamilies = Vulkan::PhysicalDevice::findQueueFamilies(device, surface);
  bool isQueueFamiliesComplete = queueFamilies.graphics && queueFamilies.present;

  bool extensionsSupported = checkDeviceExtensionSupport(device, {VK_KHR_SWAPCHAIN_EXTENSION_NAME});

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    auto swapChainSupport = surface.querySupports(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  return isQueueFamiliesComplete && extensionsSupported && swapChainAdequate &&
         (supportedFeatures.samplerAnisotropy != 0U);
}

} // namespace

void Application::init(int width, int height) {
  MainWindow::init(width, height);
  initVulkan();
}

void Application::cleanup() {
  cleanupVulkan();
  MainWindow::cleanup();
}

void Application::run() {
  MainWindow::run();
}

void Application::mainLoop() {
  MainWindow::mainLoop();
  _device.waitIdle();
}

void Application::initVulkan() {
  createInstance();
  createLogicalDevice();

  createRenderPass();
  createSwapchain();

  createCommandBuffers();
  createSyncObjects();

  auto vertShader = createVertexShader(_device);
  auto fragShader = createFragmentShader(_device);
  _pipeline.create(_device, _renderPass, vertShader, fragShader);
}

void Application::cleanupVulkan() {
  _swapchain.destroy();
  _renderPass.destroy();

  _pipeline.destroy();

  _renderFinishedSemaphores.clear();
  _imageAvailableSemaphores.clear();
  _inFlightFences.clear();

  _commandBuffers.clear();
  _commandPool.destroy();
  _device.destroy();
  _surface.destroy();
  _instance.destroy();
}

void Application::createInstance() {
  auto [extensionCount, extensions] = getRequiredInstanceExtensions();

  // Create Vulkan 1.0 instance
  _instance.create(1, 0, {extensions, extensions + extensionCount}, ENABLE_VALIDATION_LAYERS);
  _surface.create(_instance, createWindowSurface(_instance));

  _instance.pickPhysicalDevice(
      _surface, [this](VkPhysicalDevice device) { return isDeviceSuitable(device, _surface); });
}

void Application::createLogicalDevice() {
  const auto &queueFamilies = _instance.physicalDevice().queueFamilies();

  _device.create(_instance.physicalDevice(),
                 {queueFamilies.graphicsIndex(), queueFamilies.presentIndex()},
                 {VK_KHR_SWAPCHAIN_EXTENSION_NAME});

  _device.initQueue("graphics", queueFamilies.graphicsIndex());
  _device.initQueue("present", queueFamilies.presentIndex());
}

void Application::createRenderPass() {
  const auto surfaceFormat = chooseSwapSurfaceFormat(_surface.querySupports().formats);
  _renderPass.create(_device, surfaceFormat.format);
}

void Application::createSwapchain() {
  const auto [capabilities, formats, presentModes] = _surface.querySupports();
  const auto surfaceFormat = chooseSwapSurfaceFormat(formats);
  const auto surfaceExtent = chooseSwapExtent(capabilities, width(), height());
  const auto presentMode = chooseSwapPresentMode(presentModes);
  _swapchain.create(_device, _surface, surfaceExtent, surfaceFormat, presentMode);

  _swapchain.createFramebuffers(_renderPass);
}

void Application::createCommandBuffers() {
  _commandPool.create(_device, _instance.physicalDevice().queueFamilies().graphicsIndex());

  _commandBuffers.resize(_maxFrameInFlight);
  for (auto &buffer : _commandBuffers) {
    buffer.allocate(_commandPool);
  }
}

void Application::createSyncObjects() {
  _imageAvailableSemaphores.resize(_maxFrameInFlight);
  _renderFinishedSemaphores.resize(_maxFrameInFlight);
  _inFlightFences.resize(_maxFrameInFlight);

  for (size_t i = 0; i < _maxFrameInFlight; i++) {
    _imageAvailableSemaphores[i].create(_device);
    _renderFinishedSemaphores[i].create(_device);
    _inFlightFences[i].create(_device, true);
  }
}

void Application::resizeSwapChain() {
  if (isMinimized()) {
    return;
  }
  _device.waitIdle();

  const auto surfaceExtent =
      chooseSwapExtent(_surface.querySupports().capabilities, width(), height());
  _swapchain.resize(surfaceExtent);

  setFramebufferResized(false);
}
