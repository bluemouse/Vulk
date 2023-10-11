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

} // namespace

void Application::init(int width, int height) {
  MainWindow::init(width, height);
  initVulkan();

  nextFrame();
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

  createFrames();

  createPipeline();
}

void Application::cleanupVulkan() {
  _swapchain.destroy();
  _renderPass.destroy();

  _pipeline.destroy();

  _frames.clear();

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

  _instance.pickPhysicalDevice(_surface, [this](VkPhysicalDevice device) {
    return isPhysicalDeviceSuitable(device, _surface);
  });
}

void Application::createLogicalDevice() {
  const auto& queueFamilies = _instance.physicalDevice().queueFamilies();

  _device.create(_instance.physicalDevice(),
                 {queueFamilies.graphicsIndex(), queueFamilies.presentIndex()},
                 {VK_KHR_SWAPCHAIN_EXTENSION_NAME});

  _device.initQueue("graphics", queueFamilies.graphicsIndex());
  _device.initQueue("present", queueFamilies.presentIndex());
}

void Application::createRenderPass() {
  const auto surfaceFormat = chooseSwapchainSurfaceFormat(_surface.querySupports().formats);
  _renderPass.create(_device, surfaceFormat.format);
}

void Application::createSwapchain() {
  const auto [capabilities, formats, presentModes] = _surface.querySupports();
  const auto extent = surfaceExtent(capabilities, width(), height());
  const auto format = chooseSwapchainSurfaceFormat(formats);
  const auto presentMode = chooseSwapchainPresentMode(presentModes);
  _swapchain.create(_device, _surface, extent, format, presentMode);

  _swapchain.createFramebuffers(_renderPass);
}

void Application::createFrames() {
  _commandPool.create(_device, _instance.physicalDevice().queueFamilies().graphicsIndex());

  _frames.resize(_maxFrameInFlight);
  for (auto& frame : _frames) {
    frame.commandBuffer.allocate(_commandPool);
    frame.imageAvailableSemaphore.create(_device);
    frame.renderFinishedSemaphore.create(_device);
    frame.fence.create(_device, true);
  }
}

void Application::createPipeline() {
  auto vertShader = createVertexShader(_device);
  auto fragShader = createFragmentShader(_device);
  _pipeline.create(_device, _renderPass, vertShader, fragShader);
}

void Application::resizeSwapchain() {
  if (isMinimized()) {
    return;
  }
  _device.waitIdle();

  const auto extent = surfaceExtent(_surface.querySupports().capabilities, width(), height());
  _swapchain.resize(extent);

  setFramebufferResized(false);
}

VkExtent2D Application::surfaceExtent(const VkSurfaceCapabilitiesKHR& caps,
                                      uint32_t windowWidth,
                                      uint32_t windowHeight) const {
  if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return caps.currentExtent;
  } else {
    auto width = std::clamp(windowWidth, caps.minImageExtent.width, caps.maxImageExtent.width);
    auto height = std::clamp(windowHeight, caps.minImageExtent.height, caps.maxImageExtent.height);
    return {width, height};
  }
}

bool Application::isPhysicalDeviceSuitable(VkPhysicalDevice device,
                                           const Vulkan::Surface& surface) {
  auto queueFamilies = Vulkan::PhysicalDevice::findQueueFamilies(device, surface);
  bool isQueueFamiliesComplete = queueFamilies.graphics && queueFamilies.present;

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

  return isQueueFamiliesComplete && extensionsSupported && surface.isAdequate(device) &&
         (supportedFeatures.samplerAnisotropy != 0U);
}

void Application::nextFrame() {
  _currentFrameIdx = (_currentFrameIdx + 1) % _maxFrameInFlight;
  _currentFrame = &_frames[_currentFrameIdx];
}
