#include "Testbed.h"

#include <Vulk/Toolbox.h>
#include <Vulk/TypeTraits.h>

#include <glm/glm.hpp>
//#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <set>
#include <cstring>
#include <filesystem>

namespace {
#ifdef NDEBUG
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif

#if defined(__linux__)
std::filesystem::path executablePath() {
  return std::filesystem::canonical("/proc/self/exe").parent_path();
}
#else
std::filesystem::path getExecutablePath() {
  return std::filesystem::path{};
}
#endif
} // namespace

void Testbed::init(int width, int height) {
  MainWindow::init(width, height);

  createContext();
  createRenderable();

  auto textureFile = executablePath() / "textures/ScreenTestPattern.png";
  _texture = Vulk::Toolbox(_context).createTexture(textureFile.string().c_str());

  createFrames();
}

void Testbed::cleanup() {
  _frames.clear();

  _texture.destroy();

  _drawable.destroy();
  _context.destroy();

  MainWindow::cleanup();
}

void Testbed::run() {
  MainWindow::run();
}

void Testbed::mainLoop() {
  MainWindow::mainLoop();
  _context.waitIdle();
}

void Testbed::drawFrame() {
  nextFrame();

  _currentFrame->fence.wait();
  _currentFrame->fence.reset();

  if (_context.swapchain().acquireNextImage(_currentFrame->imageAvailableSemaphore) ==
      VK_ERROR_OUT_OF_DATE_KHR) {
    resizeSwapchain();
    return;
  }

  updateUniformBuffer();

  _currentFrame->commandBuffer.reset();
  _currentFrame->commandBuffer.executeCommands(
      [this](const Vulk::CommandBuffer& commandBuffer) {
        const auto& framebuffer = _context.swapchain().activeFramebuffer();

        commandBuffer.beginRenderPass(_context.renderPass(), framebuffer);

        commandBuffer.bindPipeline(_context.pipeline());

        auto extent = framebuffer.extent();
        commandBuffer.setViewport({0.0F, 0.0F}, {extent.width, extent.height});

        commandBuffer.bindVertexBuffer(_drawable.vertexBuffer(), VertexBufferBinding);
        commandBuffer.bindIndexBuffer(_drawable.indexBuffer());
        commandBuffer.bindDescriptorSet(_context.pipeline(), _currentFrame->descriptorSet);

        commandBuffer.drawIndexed(_drawable.numIndices());

        commandBuffer.endRenderpass();
      },
      {&_currentFrame->imageAvailableSemaphore},
      {&_currentFrame->renderFinishedSemaphore},
      _currentFrame->fence);

  auto result = _context.swapchain().present(_currentFrame->renderFinishedSemaphore);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isFramebufferResized()) {
    resizeSwapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("Error: failed to present swap chain image!");
  }
}

void Testbed::createContext() {
  auto [extensionCount, extensions] = getRequiredInstanceExtensions();

  Vulk::Context::CreateInfo createInfo;
  createInfo.extensions = {extensions, extensions + extensionCount};
  createInfo.enableValidation = kEnableValidationLayers;

  createInfo.isDeviceSuitable = [this](VkPhysicalDevice device) {
    return isPhysicalDeviceSuitable(device, _context.surface());
  };
  createInfo.createWindowSurface = [this](const Vulk::Instance& instance) {
    return createWindowSurface(instance);
  };
  createInfo.chooseSurfaceExtent = [this](const VkSurfaceCapabilitiesKHR& caps) {
    return chooseSwapchainSurfaceExtent(caps, width(), height());
  };
  createInfo.chooseSurfaceFormat = &Testbed::chooseSwapchainSurfaceFormat;
  createInfo.choosePresentMode = &Testbed::chooseSwapchainPresentMode;

  createInfo.maxDescriptorSets = _maxFrameInFlight;
  createInfo.createVertShader = &Testbed::createVertexShader;
  createInfo.createFragShader = &Testbed::createFragmentShader;

  _context.create(createInfo);
}

void Testbed::createRenderable() {
  const std::vector<Vertex> vertices = {{{-1.0F, -1.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
                                        {{-1.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 1.0F}},
                                        {{1.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
                                        {{1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}, {1.0F, 0.0F}}};

  const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

  _drawable.create(
      _context.device(), Vulk::CommandBuffer{_context.commandPool()}, vertices, indices);
}

void Testbed::createFrames() {
  _frames.resize(_maxFrameInFlight);
  for (auto& frame : _frames) {
    frame.commandBuffer.allocate(_context.commandPool());
    frame.imageAvailableSemaphore.create(_context.device());
    frame.renderFinishedSemaphore.create(_context.device());
    frame.fence.create(_context.device(), true);

    frame.uniformBuffer.create(_context.device(), sizeof(Transformation));
    frame.uniformBufferMapped = frame.uniformBuffer.map();
  }

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = _texture;
  imageInfo.sampler = _texture;

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.offset = 0;
  bufferInfo.range = VK_WHOLE_SIZE;

  for (auto& frame : _frames) {
    bufferInfo.buffer = frame.uniformBuffer;

    std::vector<Vulk::DescriptorSet::Binding> bindings = {&bufferInfo, &imageInfo};
    frame.descriptorSet.allocate(
        _context.descriptorPool(), _context.pipeline().descriptorSetLayout(), bindings);
  }

  nextFrame();
}

Vulk::VertexShader Testbed::createVertexShader(const Vulk::Device& device) {
  auto shaderFile = executablePath() / "shaders/vert.spv";
  return {device, shaderFile.string().c_str()};
}

Vulk::FragmentShader Testbed::createFragmentShader(const Vulk::Device& device) {
  auto shaderFile = executablePath() / "shaders/frag.spv";
  return {device, shaderFile.string().c_str()};
}

void Testbed::resizeSwapchain() {
  if (isMinimized()) {
    return;
  }
  _context.waitIdle();

  const auto caps = _context.surface().querySupports().capabilities;
  const auto extent = chooseSwapchainSurfaceExtent(caps, width(), height());
  _context.swapchain().resize(extent);

  setFramebufferResized(false);
}

void Testbed::updateUniformBuffer() {
  using glm::vec3;
  using glm::vec4;
  using glm::mat4;

  auto [textureW, textureH] = _texture.extent();
  float textureAspect = static_cast<float>(textureW) / static_cast<float>(textureH);

  Transformation xform{};

  // Make the aspect ratio of the rendered texture match the physical texture.
  xform.model = mat4{1.0F};
  if (textureAspect > 1.0F) {
    xform.model[1][1] = 1.0F / textureAspect;
  } else {
    xform.model[0][0] = 1.0F / textureAspect;
  }

  vec3 cameraPos{0.0F, 0.0F, -1.0F};
  vec3 cameraLookAt{0.0F, 0.0F, 0.0F};
  vec3 cameraUp{0.0F, -1.0F, 0.0F};
  auto cameraDist = glm::distance(cameraPos, cameraLookAt);

  xform.view = glm::lookAt(cameraPos, cameraLookAt, cameraUp);

  auto [surfaceW, surfaceH] = _context.swapchain().surfaceExtent();
  float surfaceAspect = static_cast<float>(surfaceW) / static_cast<float>(surfaceH);

  vec4 roi = surfaceAspect > 1.0F ? vec4{-surfaceAspect, surfaceAspect, 1.0F, -1.0F}
                                  : vec4{-1.0F, 1.0F, 1.0F / surfaceAspect, -1.0F / surfaceAspect};

// #define USE_PERSPECTIVE_PROJECTION
#if defined(USE_PERSPECTIVE_PROJECTION)
  float fovy = glm::angle(cameraPos - cameraLookAt, cameraUp * (roi[2] - roi[3]) / 2.0F);
  ubo.proj = glm::perspective(fovy, surfaceAspect, 0.0F, cameraDist * 2);
  ubo.proj[1][1] *= -1;
#else
  auto zNear = cameraLookAt.z - cameraDist;
  auto zFar = cameraLookAt.z + cameraDist;
  xform.proj = glm::ortho(roi[0], roi[1], roi[2], roi[3], zNear, zFar);
#endif

  memcpy(_currentFrame->uniformBufferMapped, &xform, sizeof(xform));
}

bool Testbed::isPhysicalDeviceSuitable(VkPhysicalDevice device, const Vulk::Surface& surface) {
  auto queueFamilies = Vulk::PhysicalDevice::findQueueFamilies(device, surface);
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

VkExtent2D Testbed::chooseSwapchainSurfaceExtent(const VkSurfaceCapabilitiesKHR& caps,
                                                 uint32_t windowWidth,
                                                 uint32_t windowHeight) {
  if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return caps.currentExtent;
  } else {
    auto width = std::clamp(windowWidth, caps.minImageExtent.width, caps.maxImageExtent.width);
    auto height = std::clamp(windowHeight, caps.minImageExtent.height, caps.maxImageExtent.height);
    return {width, height};
  }
}

VkSurfaceFormatKHR Testbed::chooseSwapchainSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR Testbed::chooseSwapchainPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

void Testbed::nextFrame() {
  _currentFrameIdx = (_currentFrameIdx + 1) % _maxFrameInFlight;
  _currentFrame = &_frames[_currentFrameIdx];
}