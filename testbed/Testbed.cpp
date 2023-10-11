#include "Testbed.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <chrono>

namespace {
struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
  glm::vec2 texCoord;
};

struct UniformBufferObject {
  alignas(sizeof(glm::vec4)) glm::mat4 model;
  alignas(sizeof(glm::vec4)) glm::mat4 view;
  alignas(sizeof(glm::vec4)) glm::mat4 proj;
};

const std::vector<Vertex> vertices = {{{-0.5F, -0.5F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
                                      {{0.5F, -0.5F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
                                      {{0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
                                      {{-0.5F, 0.5F}, {1.0F, 1.0F, 1.0F}, {1.0F, 1.0F}}};

const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

} // namespace

void Testbed::initVulkan() {
  Application::initVulkan();

  createVertexBuffer();
  createIndexBuffer();
  createTextureImage();

  createFrameExts();
}

void Testbed::cleanupVulkan() {
  _indexBuffer.destroy();
  _vertexBuffer.destroy();

  _textureSampler.destroy();
  _textureImageView.destroy();
  _textureImage.destroy();

  _frameExts.clear();

  _descriptorPool.destroy();

  Application::cleanupVulkan();
}

void Testbed::drawFrame() {
  _currentFrame->fence.wait();

  auto imageIndex = _swapchain.acquireNextImage(_currentFrame->imageAvailableSemaphore);
  if (imageIndex == VK_ERROR_OUT_OF_DATE_KHR) {
    resizeSwapchain();
    return;
  }

  updateUniformBuffer();

  _currentFrame->fence.reset();

  _currentFrame->commandBuffer.reset();
  _currentFrame->commandBuffer.executeCommands(
      [this, imageIndex](const Vulkan::CommandBuffer& commandBuffer) {
        const auto& framebuffer = _swapchain.framebuffer(imageIndex);
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
                                _currentFrameExt->descriptorSet,
                                0,
                                nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
      },
      {&_currentFrame->imageAvailableSemaphore},
      {&_currentFrame->renderFinishedSemaphore},
      _currentFrame->fence);

  auto result = _swapchain.queuePreset(imageIndex, _currentFrame->renderFinishedSemaphore);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isFramebufferResized()) {
    resizeSwapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("Error: failed to present swap chain image!");
  }

  nextFrame();
}

void Testbed::createTextureImage() {
  int texWidth = 0;
  int texHeight = 0;
  int texChannels = 0;

  const char* texFile = "textures/texture.jpg";

  stbi_uc* pixels = stbi_load(texFile, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
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
  stagingBuffer.copyToImage(
      cmdBuffer, _textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  _textureImage.transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  _textureImageView.create(_device, _textureImage);
  _textureSampler.create(_device, {VK_FILTER_LINEAR}, {VK_SAMPLER_ADDRESS_MODE_REPEAT});
}

void Testbed::createVertexBuffer() {
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  Vulkan::StagingBuffer stagingBuffer(_device, bufferSize);
  stagingBuffer.copyFromHost(vertices.data(), bufferSize);

  _vertexBuffer.create(_device, bufferSize);

  stagingBuffer.copyToBuffer(Vulkan::CommandBuffer{_commandPool}, _vertexBuffer, bufferSize);
}

void Testbed::createIndexBuffer() {
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  Vulkan::StagingBuffer stagingBuffer(_device, bufferSize);
  stagingBuffer.copyFromHost(indices.data(), bufferSize);

  _indexBuffer.create(_device, bufferSize);

  stagingBuffer.copyToBuffer(Vulkan::CommandBuffer{_commandPool}, _indexBuffer, bufferSize);
}

void Testbed::createFrameExts() {
  _descriptorPool.create(_pipeline.descriptorSetLayout(), _maxFrameInFlight);

  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.offset = 0;
  bufferInfo.range = VK_WHOLE_SIZE;

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = _textureImageView;
  imageInfo.sampler = _textureSampler;

  _frameExts.resize(_frames.size());
  for (auto& frame : _frameExts) {
    frame.uniformBuffer.create(_device, bufferSize);
    frame.uniformBufferMapped = frame.uniformBuffer.map();

    bufferInfo.buffer = frame.uniformBuffer;

    std::vector<Vulkan::DescriptorSet::Binding> bindings = {&bufferInfo, &imageInfo};
    frame.descriptorSet.allocate(_descriptorPool, _pipeline.descriptorSetLayout(), bindings);
  }
}

Vulkan::FragmentShader Testbed::createFragmentShader(const Vulkan::Device& device) {
  Vulkan::FragmentShader fragShader{device, "main", "shaders/frag.spv"};
  fragShader.addDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  return fragShader;
}

Vulkan::VertexShader Testbed::createVertexShader(const Vulkan::Device& device) {
  Vulkan::VertexShader vertShader{device, "main", "shaders/vert.spv"};
  vertShader.addVertexInputBinding(0, sizeof(Vertex));
  vertShader.addVertexInputAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos));
  vertShader.addVertexInputAttribute(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));
  vertShader.addVertexInputAttribute(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord));
  vertShader.addDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

  return vertShader;
}

void Testbed::updateUniformBuffer() {
  using namespace std;

  static auto startTime = chrono::high_resolution_clock::now();

  auto currentTime = chrono::high_resolution_clock::now();
  float time = chrono::duration<float, chrono::seconds::period>(currentTime - startTime).count();

  auto extent = _swapchain.surfaceExtent();

  using glm::vec3;
  using glm::mat4;
  using glm::radians;
  UniformBufferObject ubo{};
  ubo.model = glm::rotate(mat4(1.0F), time * radians(90.0F), vec3(0.0F, 0.0F, 1.0F));
  ubo.view = glm::lookAt(vec3(2.0F, 2.0F, 2.0F), vec3(0.0F, 0.0F, 0.0F), vec3(0.0F, 0.0F, 1.0F));
  float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
  ubo.proj = glm::perspective(radians(45.0F), aspect, 0.1F, 10.0F);
  ubo.proj[1][1] *= -1;

  memcpy(_currentFrameExt->uniformBufferMapped, &ubo, sizeof(ubo));
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
  Application::nextFrame();

  _currentFrameExt = &_frameExts[_currentFrameIdx];
}