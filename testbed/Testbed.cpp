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
  createUniformBuffer();
  createDescriptorSets();
}

void Testbed::cleanupVulkan() {
  _indexBuffer.destroy();
  _vertexBuffer.destroy();

  _textureSampler.destroy();
  _textureImageView.destroy();
  _textureImage.destroy();

  _uniformBuffers.clear();
  _descriptorSets.clear();
  _descriptorPool.destroy();

  Application::cleanupVulkan();
}

void Testbed::drawFrame() {
  _inFlightFences[_currentFrame].wait();

  uint32_t imageIndex = 0;
  VkResult result = vkAcquireNextImageKHR(_device,
                                          _swapchain,
                                          UINT64_MAX,
                                          _imageAvailableSemaphores[_currentFrame],
                                          VK_NULL_HANDLE,
                                          &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  updateUniformBuffer(_currentFrame);

  _inFlightFences[_currentFrame].reset();

  _commandBuffers[_currentFrame].reset();
  _commandBuffers[_currentFrame].executeCommand(
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
                                _descriptorSets[_currentFrame],
                                0,
                                nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
      },
      {&_imageAvailableSemaphores[_currentFrame]},
      {&_renderFinishedSemaphores[_currentFrame]},
      _inFlightFences[_currentFrame]);

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = _renderFinishedSemaphores[_currentFrame];

  VkSwapchainKHR swapchains[] = {_swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;

  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(_device.queue("present"), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isFramebufferResized()) {
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swap chain image!");
  }

  _currentFrame = (_currentFrame + 1) % _maxFrameInFlight;
}


void Testbed::createTextureImage() {
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

void Testbed::createUniformBuffer() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  _uniformBuffers.resize(_maxFrameInFlight);
  _uniformBuffersMapped.resize(_maxFrameInFlight);

  for (size_t i = 0; i < _maxFrameInFlight; i++) {
    _uniformBuffers[i].create(_device, bufferSize);
    _uniformBuffersMapped[i] = _uniformBuffers[i].map();
  }
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

Vulkan::FragmentShader Testbed::createFragmentShader(const Vulkan::Device& device) {
  Vulkan::FragmentShader fragShader{device, "main", "shaders/frag.spv"};
  fragShader.addDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  return fragShader;
}


void Testbed::updateUniformBuffer(uint32_t currentImage) {
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

void Testbed::createDescriptorSets() {
  _descriptorPool.create(_pipeline.descriptorSetLayout(), _maxFrameInFlight);

  _descriptorSets.reserve(_maxFrameInFlight);
  for (size_t i = 0; i < _maxFrameInFlight; i++) {
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
