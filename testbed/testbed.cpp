#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <chrono>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

#include <Vulk/Instance.h>
#include <Vulk/PhysicalDevice.h>
#include <Vulk/Surface.h>
#include <Vulk/Swapchain.h>
#include <Vulk/Device.h>
#include <Vulk/Pipeline.h>
#include <Vulk/RenderPass.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/CommandPool.h>
#include <Vulk/VertexShader.h>
#include <Vulk/FragmentShader.h>
#include <Vulk/DescriptorPool.h>
#include <Vulk/DescriptorSet.h>
#include <Vulk/DescriptorSetLayout.h>
#include <Vulk/Buffer.h>
#include <Vulk/UniformBuffer.h>
#include <Vulk/VertexBuffer.h>
#include <Vulk/IndexBuffer.h>
#include <Vulk/StagingBuffer.h>
#include <Vulk/Image.h>
#include <Vulk/ImageView.h>
#include <Vulk/Sampler.h>
#include <Vulk/Semaphore.h>
#include <Vulk/Fence.h>
#include <Vulk/helpers_vulkan.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
  glm::vec2 texCoord;
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

const std::vector<Vertex> vertices = {{{-0.5F, -0.5F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
                                      {{0.5F, -0.5F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
                                      {{0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
                                      {{-0.5F, 0.5F}, {1.0F, 1.0F, 1.0F}, {1.0F, 1.0F}}};

const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

class HelloTriangleApplication {
 public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

 private:
  GLFWwindow *window;

  Vulkan::Instance _instance;

  Vulkan::Surface _surface;
  Vulkan::Device _device;
  Vulkan::Swapchain _swapchain;

  Vulkan::RenderPass _renderPass;
  Vulkan::Pipeline _pipeline;

  Vulkan::CommandPool _commandPool;

  Vulkan::Image _textureImage;
  Vulkan::ImageView _textureImageView;
  Vulkan::Sampler _textureSampler;

  Vulkan::VertexBuffer _vertexBuffer;
  Vulkan::IndexBuffer _indexBuffer;

  std::vector<Vulkan::UniformBuffer> _uniformBuffers;
  std::vector<void *> _uniformBuffersMapped;

  Vulkan::DescriptorPool _descriptorPool;
  std::vector<Vulkan::DescriptorSet> _descriptorSets;

  std::vector<Vulkan::CommandBuffer> _commandBuffers;

  std::vector<Vulkan::Semaphore> _imageAvailableSemaphores;
  std::vector<Vulkan::Semaphore> _renderFinishedSemaphores;
  std::vector<Vulkan::Fence> _inFlightFences;

  uint32_t currentFrame = 0;

  bool framebufferResized = false;

  void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  }

  static void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
  }

  void initVulkan() {
    createInstance();
    createLogicalDevice();
    createSwapchain();

    _renderPass.create(_device, _swapchain.imageFormat());
    _swapchain.createFramebuffers(_renderPass);

    Vulkan::VertexShader vertShader{_device, "main", "shaders/vert.spv"};
    vertShader.addVertexInputBinding(0, sizeof(Vertex));
    vertShader.addVertexInputAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos));
    vertShader.addVertexInputAttribute(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));
    vertShader.addVertexInputAttribute(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord));
    vertShader.addDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    Vulkan::FragmentShader fragShader{_device, "main", "shaders/frag.spv"};
    fragShader.addDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    _pipeline.create(_device, _renderPass, vertShader, fragShader);

    _commandPool.create(_device, _instance.physicalDevice().queueFamilies().graphicsIndex());

    createTextureImage();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
  }

  void mainLoop() {
    while (glfwWindowShouldClose(window) == 0) {
      glfwPollEvents();
      drawFrame();
    }

    _device.waitIdle();
  }

  void cleanup() {
    _swapchain.destroy();

    _pipeline.destroy();
    _renderPass.destroy();

    _uniformBuffers.clear();

    _descriptorSets.clear();
    _descriptorPool.destroy();

    _textureSampler.destroy();
    _textureImageView.destroy();
    _textureImage.destroy();

    _indexBuffer.destroy();
    _vertexBuffer.destroy();

    _renderFinishedSemaphores.clear();
    _imageAvailableSemaphores.clear();
    _inFlightFences.clear();

    _commandBuffers.clear();
    _commandPool.destroy();
    _device.destroy();
    _surface.destroy();
    _instance.destroy();

    glfwDestroyWindow(window);

    glfwTerminate();
  }

  void recreateSwapChain() {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window, &width, &height);
      glfwWaitEvents();
    }

    _device.waitIdle();

    _swapchain.destroy();

    createSwapchain();

    _swapchain.createFramebuffers(_renderPass);
  }

  void createInstance() {
    uint32_t extensionCount = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    // Create Vulkan 1.0 instance
    _instance.create(1, 0, {extensions, extensions + extensionCount}, enableValidationLayers);

    VkSurfaceKHR surface;
    MI_VERIFY_VKCMD(glfwCreateWindowSurface(_instance, window, nullptr, &surface));
    _surface.create(_instance, surface);

    _instance.pickPhysicalDevice(
        _surface, [this](VkPhysicalDevice device) { return isDeviceSuitable(device); });
  }

  void createLogicalDevice() {
    const auto &queueFamilies = _instance.physicalDevice().queueFamilies();

    _device.create(_instance.physicalDevice(),
                   {queueFamilies.graphicsIndex(), queueFamilies.presentIndex()},
                   deviceExtensions);

    _device.initQueue("graphics", queueFamilies.graphicsIndex());
    _device.initQueue("present", queueFamilies.presentIndex());
  }

  void createSwapchain() {
    auto chooseSwapSurfaceFormatFunc =
        [this](const std::vector<VkSurfaceFormatKHR> &formats) -> VkSurfaceFormatKHR {
      return chooseSwapSurfaceFormat(formats);
    };
    auto chooseSwapPresentModeFunc =
        [this](const std::vector<VkPresentModeKHR> &modes) -> VkPresentModeKHR {
      return chooseSwapPresentMode(modes);
    };
    auto chooseSwapExtentFunc = [this](const VkSurfaceCapabilitiesKHR &caps) -> VkExtent2D {
      return chooseSwapExtent(caps);
    };
    _swapchain.create(_device,
                      _surface,
                      chooseSwapSurfaceFormatFunc,
                      chooseSwapPresentModeFunc,
                      chooseSwapExtentFunc);
  }

  void createTextureImage() {
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;

    const char *texFile = "textures/texture.jpg";

    stbi_uc *pixels = stbi_load(texFile, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    MI_VERIFY(pixels != nullptr);

    auto imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * 4);

    Vulkan::StagingBuffer stagingBuffer(_device, imageSize);
    stagingBuffer.copyFromHost(pixels, imageSize);

    stbi_image_free(pixels);

    _textureImage.create(_device,
                         VK_FORMAT_R8G8B8A8_SRGB,
                         {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)});
    _textureImage.allocate();

    Vulkan::CommandBuffer cmdBuffer{_commandPool};
    _textureImage.transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    stagingBuffer.copyToImage(cmdBuffer,
                              _textureImage,
                              static_cast<uint32_t>(texWidth),
                              static_cast<uint32_t>(texHeight));
    _textureImage.transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    _textureImageView.create(_device, _textureImage);
    _textureSampler.create(_device, {VK_FILTER_LINEAR}, {VK_SAMPLER_ADDRESS_MODE_REPEAT});
  }

  void createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    Vulkan::StagingBuffer stagingBuffer(_device, bufferSize);
    stagingBuffer.copyFromHost(vertices.data(), bufferSize);

    _vertexBuffer.create(_device, bufferSize);

    stagingBuffer.copyToBuffer(Vulkan::CommandBuffer{_commandPool}, _vertexBuffer, bufferSize);
  }

  void createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    Vulkan::StagingBuffer stagingBuffer(_device, bufferSize);
    stagingBuffer.copyFromHost(indices.data(), bufferSize);

    _indexBuffer.create(_device, bufferSize);

    stagingBuffer.copyToBuffer(Vulkan::CommandBuffer{_commandPool}, _indexBuffer, bufferSize);
  }

  void createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    _uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    _uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      _uniformBuffers[i].create(_device, bufferSize);
      _uniformBuffersMapped[i] = _uniformBuffers[i].map();
    }
  }

  void createDescriptorSets() {
    _descriptorPool.create(_pipeline.descriptorSetLayout(), MAX_FRAMES_IN_FLIGHT);

    _descriptorSets.reserve(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = _uniformBuffers[i];
      bufferInfo.offset = 0;
      bufferInfo.range = sizeof(UniformBufferObject);

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfo.imageView = _textureImageView;
      imageInfo.sampler = _textureSampler;

      std::vector<Vulkan::DescriptorSet::Binding> bindings = {&bufferInfo, &imageInfo};
      _descriptorSets.emplace_back(_descriptorPool, _pipeline.descriptorSetLayout(), bindings);
    }
  }

  void createCommandBuffers() {
    _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (auto &buffer : _commandBuffers) {
      buffer.allocate(_commandPool);
    }
  }

  void createSyncObjects() {
    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      _imageAvailableSemaphores[i].create(_device);
      _renderFinishedSemaphores[i].create(_device);
      _inFlightFences[i].create(_device, true);
    }
  }

  void updateUniformBuffer(uint32_t currentImage) {
    using namespace std;

    static auto startTime = chrono::high_resolution_clock::now();

    auto currentTime = chrono::high_resolution_clock::now();
    float time = chrono::duration<float, chrono::seconds::period>(currentTime - startTime).count();

    auto extent = _swapchain.imageExtent();

    using glm::vec3;
    using glm::mat4;
    using glm::radians;
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(mat4(1.0F), time * radians(90.0F), vec3(0.0F, 0.0F, 1.0F));
    ubo.view = glm::lookAt(vec3(2.0F, 2.0F, 2.0F), vec3(0.0F, 0.0F, 0.0F), vec3(0.0F, 0.0F, 1.0F));
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    ubo.proj = glm::perspective(radians(45.0F), aspect, 0.1F, 10.0F);
    ubo.proj[1][1] *= -1;

    memcpy(_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
  }

  void drawFrame() {
    _inFlightFences[currentFrame].wait();

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(_device,
                                            _swapchain,
                                            UINT64_MAX,
                                            _imageAvailableSemaphores[currentFrame],
                                            VK_NULL_HANDLE,
                                            &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapChain();
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer(currentFrame);

    _inFlightFences[currentFrame].reset();

    _commandBuffers[currentFrame].reset();
    _commandBuffers[currentFrame].executeCommand(
        [this, imageIndex](const Vulkan::CommandBuffer &commandBuffer) {
          const auto &framebuffer = _swapchain.framebuffer(imageIndex);
          auto extent = framebuffer.extent();

          VkRenderPassBeginInfo renderPassInfo{};
          renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
          renderPassInfo.renderPass = _renderPass;
          renderPassInfo.framebuffer = framebuffer;
          renderPassInfo.renderArea.offset = {0, 0};
          renderPassInfo.renderArea.extent = extent;

          VkClearValue clearColor = {{{0.0F, 0.0F, 0.0F, 1.0F}}};
          renderPassInfo.clearValueCount = 1;
          renderPassInfo.pClearValues = &clearColor;

          vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
          vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

          VkViewport viewport{};
          viewport.x = 0.0F;
          viewport.y = 0.0F;
          viewport.width = (float)extent.width;
          viewport.height = (float)extent.height;
          viewport.minDepth = 0.0F;
          viewport.maxDepth = 1.0F;
          vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

          VkRect2D scissor{};
          scissor.offset = {0, 0};
          scissor.extent = extent;
          vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

          VkBuffer vertexBuffers[] = {_vertexBuffer};
          VkDeviceSize offsets[] = {0};
          vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
          vkCmdBindIndexBuffer(commandBuffer, _indexBuffer, 0, VK_INDEX_TYPE_UINT16);
          vkCmdBindDescriptorSets(commandBuffer,
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  _pipeline.layout(),
                                  0,
                                  1,
                                  _descriptorSets[currentFrame],
                                  0,
                                  nullptr);

          vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

          vkCmdEndRenderPass(commandBuffer);
        },
        {&_imageAvailableSemaphores[currentFrame]},
        {&_renderFinishedSemaphores[currentFrame]},
        _inFlightFences[currentFrame]);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = _renderFinishedSemaphores[currentFrame];

    VkSwapchainKHR swapchains[] = {_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(_device.queue("present"), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
      framebufferResized = false;
      recreateSwapChain();
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("Failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

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

  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return availablePresentMode;
      }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    } else {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);

      VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

      actualExtent.width = std::clamp(
          actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
      actualExtent.height = std::clamp(actualExtent.height,
                                       capabilities.minImageExtent.height,
                                       capabilities.maxImageExtent.height);

      return actualExtent;
    }
  }

  bool isDeviceSuitable(VkPhysicalDevice device) {
    auto queueFamilies = Vulkan::PhysicalDevice::findQueueFamilies(device, _surface);
    bool isQueueFamiliesComplete = queueFamilies.graphics && queueFamilies.present;

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
      auto swapChainSupport = _surface.querySupports(device);
      swapChainAdequate =
          !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return isQueueFamiliesComplete && extensionsSupported && swapChainAdequate &&
           (supportedFeatures.samplerAnisotropy != 0U);
  }

  bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : availableExtensions) {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
  }
};

int main() {
  HelloTriangleApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
