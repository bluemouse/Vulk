#include "Testbed.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <chrono>
#include <set>

namespace {
#ifdef NDEBUG
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif
} // namespace

namespace {
struct UniformBufferObject {
  alignas(sizeof(glm::vec4)) glm::mat4 model;
  alignas(sizeof(glm::vec4)) glm::mat4 view;
  alignas(sizeof(glm::vec4)) glm::mat4 proj;
};

} // namespace

void Testbed::init(int width, int height) {
  MainWindow::init(width, height);

  createContext();
  createRenderable();
  createTextureImage();
  createFrames();
}

void Testbed::cleanup() {
  _frames.clear();

  _textureSampler.destroy();
  _textureImageView.destroy();
  _textureImage.destroy();

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
  _currentFrame->fence.wait();

  auto imageIndex = _context.swapchain().acquireNextImage(_currentFrame->imageAvailableSemaphore);
  if (imageIndex == VK_ERROR_OUT_OF_DATE_KHR) {
    resizeSwapchain();
    return;
  }

  updateUniformBuffer();

  _currentFrame->fence.reset();

  _currentFrame->commandBuffer.reset();
  _currentFrame->commandBuffer.executeCommands(
      [this, imageIndex](const Vulkan::CommandBuffer& commandBuffer) {
        const auto& framebuffer = _context.swapchain().framebuffer(imageIndex);

        commandBuffer.beginRenderPass(_context.renderPass(), framebuffer);

        commandBuffer.bindPipeline(_context.pipeline());

        auto extent = framebuffer.extent();
        commandBuffer.setViewport({0.0F, 0.0F}, {extent.width, extent.height});

        commandBuffer.bindVertexBuffer(_drawable.vertexBuffer(), 0);
        commandBuffer.bindIndexBuffer(_drawable.indexBuffer());
        commandBuffer.bindDescriptorSet(_context.pipeline(), _currentFrame->descriptorSet);

        commandBuffer.drawIndexed(_drawable.numIndices());

        commandBuffer.endRenderpass();
      },
      {&_currentFrame->imageAvailableSemaphore},
      {&_currentFrame->renderFinishedSemaphore},
      _currentFrame->fence);

  auto result = _context.swapchain().present(imageIndex, _currentFrame->renderFinishedSemaphore);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isFramebufferResized()) {
    resizeSwapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("Error: failed to present swap chain image!");
  }

  nextFrame();
}

void Testbed::createContext() {
  auto [extensionCount, extensions] = getRequiredInstanceExtensions();

  Vulkan::Context::CreateInfo createInfo;
  createInfo.extensions = {extensions, extensions + extensionCount};
  createInfo.enableValidation = ENABLE_VALIDATION_LAYERS;

  createInfo.isDeviceSuitable = [this](VkPhysicalDevice device) {
    return isPhysicalDeviceSuitable(device, _context.surface());
  };
  createInfo.createWindowSurface = [this](VkInstance instance) {
    return createWindowSurface(instance);
  };
  createInfo.chooseSurfaceExtent = [this](const VkSurfaceCapabilitiesKHR& caps) {
    return chooseSwapchainSurfaceExtent(caps, width(), height());
  };
  createInfo.chooseSurfaceFormat = [](const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    return chooseSwapchainSurfaceFormat(availableFormats);
  };
  createInfo.choosePresentMode = [](const std::vector<VkPresentModeKHR>& availableModes) {
    return chooseSwapchainPresentMode(availableModes);
  };
  createInfo.maxDescriptorSets = _maxFrameInFlight;
  createInfo.createVertShader = [](const Vulkan::Device& device) {
    return createVertexShader(device);
  };
  createInfo.createFragShader = [](const Vulkan::Device& device) {
    return createFragmentShader(device);
  };

  _context.create(createInfo);
}

void Testbed::createTextureImage() {
  int texWidth = 0;
  int texHeight = 0;
  int texChannels = 0;

  const char* texFile = "textures/texture.jpg";

  stbi_uc* pixels = stbi_load(texFile, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  MI_VERIFY(pixels != nullptr);

  auto imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * 4);

  Vulkan::StagingBuffer stagingBuffer(_context.device(), imageSize);
  stagingBuffer.copyFromHost(pixels, imageSize);

  stbi_image_free(pixels);

  _textureImage.create(_context.device(),
                       VK_FORMAT_R8G8B8A8_SRGB,
                       {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)});
  _textureImage.allocate();

  Vulkan::CommandBuffer cmdBuffer{_context.commandPool()};
  _textureImage.transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  stagingBuffer.copyToImage(
      cmdBuffer, _textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  _textureImage.transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  _textureImageView.create(_context.device(), _textureImage);
  _textureSampler.create(_context.device(), {VK_FILTER_LINEAR}, {VK_SAMPLER_ADDRESS_MODE_REPEAT});
}

void Testbed::createRenderable() {
  const std::vector<Vertex> vertices = {
      {{-0.5F, -0.5F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
      {{0.5F, -0.5F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
      {{0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
      {{-0.5F, 0.5F}, {1.0F, 1.0F, 1.0F}, {1.0F, 1.0F}}};

  const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

  _drawable.create(
      _context.device(), Vulkan::CommandBuffer{_context.commandPool()}, vertices, indices);
}

void Testbed::createFrames() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.offset = 0;
  bufferInfo.range = VK_WHOLE_SIZE;

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = _textureImageView;
  imageInfo.sampler = _textureSampler;

  _frames.resize(_maxFrameInFlight);
  for (auto& frame : _frames) {
    frame.commandBuffer.allocate(_context.commandPool());
    frame.imageAvailableSemaphore.create(_context.device());
    frame.renderFinishedSemaphore.create(_context.device());
    frame.fence.create(_context.device(), true);

    frame.uniformBuffer.create(_context.device(), bufferSize);
    frame.uniformBufferMapped = frame.uniformBuffer.map();

    bufferInfo.buffer = frame.uniformBuffer;

    std::vector<Vulkan::DescriptorSet::Binding> bindings = {&bufferInfo, &imageInfo};
    frame.descriptorSet.allocate(
        _context.descriptorPool(), _context.pipeline().descriptorSetLayout(), bindings);
  }

  nextFrame();
}

Vulkan::VertexShader Testbed::createVertexShader(const Vulkan::Device& device) {
  Vulkan::VertexShader vertShader{device, "main", "shaders/vert.spv"};
  vertShader.addVertexInputBinding(0, sizeof(Vertex));
  vertShader.addVertexInputAttribute(
      0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos));
  vertShader.addVertexInputAttribute(
      1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));
  vertShader.addVertexInputAttribute(
      2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord));
  vertShader.addDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

  return vertShader;
}

Vulkan::FragmentShader Testbed::createFragmentShader(const Vulkan::Device& device) {
  Vulkan::FragmentShader fragShader{device, "main", "shaders/frag.spv"};
  fragShader.addDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  return fragShader;
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
  using namespace std;

  static auto startTime = chrono::high_resolution_clock::now();

  auto currentTime = chrono::high_resolution_clock::now();
  float time = chrono::duration<float, chrono::seconds::period>(currentTime - startTime).count();

  auto extent = _context.swapchain().surfaceExtent();

  using glm::vec3;
  using glm::mat4;
  using glm::radians;
  UniformBufferObject ubo{};
  ubo.model = glm::rotate(mat4(1.0F), time * radians(90.0F), vec3(0.0F, 0.0F, 1.0F));
  ubo.view = glm::lookAt(vec3(2.0F, 2.0F, 2.0F), vec3(0.0F, 0.0F, 0.0F), vec3(0.0F, 0.0F, 1.0F));
  float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
  ubo.proj = glm::perspective(radians(45.0F), aspect, 0.1F, 10.0F);
  ubo.proj[1][1] *= -1;

  memcpy(_currentFrame->uniformBufferMapped, &ubo, sizeof(ubo));
}

bool Testbed::isPhysicalDeviceSuitable(VkPhysicalDevice device, const Vulkan::Surface& surface) {
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