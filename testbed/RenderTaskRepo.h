#pragma once

#include <Vulk/engine/RenderTask.h>
#include <Vulk/engine/Texture2D.h>

#include <Vulk/RenderPass.h>
#include <Vulk/Pipeline.h>
#include <Vulk/DescriptorPool.h>
#include <Vulk/UniformBuffer.h>
#include <Vulk/DescriptorSet.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/Framebuffer.h>
#include <Vulk/Semaphore.h>
#include <Vulk/Fence.h>
#include <Vulk/DepthImage.h>
#include <Vulk/Image2D.h>
#include <Vulk/VertexBuffer.h>
#include <Vulk/IndexBuffer.h>

MI_NAMESPACE_BEGIN(Vulk)

//
//
//
class TextureMappingTask : public RenderTask {
 public:
  struct Uniforms {
    alignas(sizeof(glm::vec4)) glm::mat4 model;
    alignas(sizeof(glm::vec4)) glm::mat4 view;
    alignas(sizeof(glm::vec4)) glm::mat4 proj;

    static size_t size() {
      return sizeof(Uniforms);
    }
    static UniformBuffer::shared_ptr allocateBuffer(const Device& device) {
      return UniformBuffer::make_shared(device, size());
    }
    static VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(uint32_t binding) {
      return {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
    }
  };

 public:
  explicit TextureMappingTask(const DeviceContext& deviceContext);
  ~TextureMappingTask() override;

  void prepareGeometry(const VertexBuffer& vertexBuffer,
                       const IndexBuffer& indexBuffer,
                       size_t numIndices);
  void prepareUniforms(const glm::mat4& model2world,
                       const glm::mat4& world2view,
                       const glm::mat4& project);
  void prepareInputs(const Texture2D& texture);
  void prepareOutputs(const Image2D& colorBuffer, const DepthImage& depthStencilBuffer);
  void prepareSynchronization(const std::vector<Semaphore::shared_ptr>& waits = {});

  std::pair<Semaphore::shared_ptr, Fence::shared_ptr> run() override;
  DescriptorSetLayout::shared_ptr descriptorSetLayout() override;

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(TextureMappingTask, RenderTask);

 private:
  RenderPass::shared_ptr _renderPass;
  Pipeline::shared_ptr _pipeline;

  uint32_t _vertexBufferBinding = 0U;

  // Geometry
  VertexBuffer::shared_ptr_const _vertexBuffer;
  IndexBuffer::shared_ptr_const _indexBuffer;
  size_t _numIndices;

  // Inputs
  Texture2D::shared_ptr_const _texture;

  // Uniforms
  UniformBuffer::shared_ptr _uniformBuffer;

  // Outputs
  Image2D::shared_ptr_const _colorBuffer;
  DepthImage::shared_ptr_const _depthStencilBuffer; // TODO this could be internal managed as well.

  // Synchronizations
  std::vector<Semaphore::shared_ptr> _waits;
};

//
//
//
class PresentTask : public RenderTask {
 public:
  explicit PresentTask(const DeviceContext& deviceContext);
  ~PresentTask() override;

  void prepareInput(const Image2D& frame);
  void prepareSynchronization(const std::vector<Semaphore::shared_ptr>& waits = {});

  std::pair<Semaphore::shared_ptr, Fence::shared_ptr> run() override;
  DescriptorSetLayout::shared_ptr descriptorSetLayout() override { return nullptr; }

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(PresentTask, RenderTask);

 private:
  // Input
  Image2D::shared_ptr_const _frame;

  // Synchronizations
  std::vector<Semaphore::shared_ptr> _waits;
};

MI_NAMESPACE_END(Vulk)