#pragma once

#include "Application.h"

class Testbed : public Application {
 protected:
  void drawFrame() override;

  void initVulkan() override;
  void cleanupVulkan() override;

  Vulkan::VertexShader createVertexShader(const Vulkan::Device& device) override;
  Vulkan::FragmentShader createFragmentShader(const Vulkan::Device& device) override;

  VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats) override;
  VkPresentModeKHR chooseSwapchainPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes) override;

  void nextFrame() override;

private:
  void createTextureImage();
  void createVertexBuffer();
  void createIndexBuffer();
  void createFrameExts();

  void updateUniformBuffer();

 private:
  Vulkan::VertexBuffer _vertexBuffer;
  Vulkan::IndexBuffer _indexBuffer;

  Vulkan::Image _textureImage;
  Vulkan::ImageView _textureImageView;
  Vulkan::Sampler _textureSampler;

  Vulkan::DescriptorPool _descriptorPool;

  struct FrameExt {
    Vulkan::UniformBuffer uniformBuffer;
    void* uniformBufferMapped;

    Vulkan::DescriptorSet descriptorSet;
  };
  std::vector<FrameExt> _frameExts;
  FrameExt* _currentFrameExt = nullptr;
};
