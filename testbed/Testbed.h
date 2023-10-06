#pragma once

#include "Application.h"

class Testbed : public Application {
 protected:
  void drawFrame() override;

  void initVulkan() override;
  void cleanupVulkan() override;

  Vulkan::VertexShader createVertexShader(const Vulkan::Device& device) override;
  Vulkan::FragmentShader createFragmentShader(const Vulkan::Device& device) override;

  void createTextureImage();
  void createVertexBuffer();
  void createIndexBuffer();
  void createUniformBuffer();
  void createDescriptorSets();
  void updateUniformBuffer(uint32_t currentImage);

 private:
  Vulkan::VertexBuffer _vertexBuffer;
  Vulkan::IndexBuffer _indexBuffer;

  Vulkan::Image _textureImage;
  Vulkan::ImageView _textureImageView;
  Vulkan::Sampler _textureSampler;

  std::vector<Vulkan::UniformBuffer> _uniformBuffers;
  std::vector<void*> _uniformBuffersMapped;

  Vulkan::DescriptorPool _descriptorPool;
  std::vector<Vulkan::DescriptorSet> _descriptorSets;
};
