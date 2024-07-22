#pragma once

#include <Vulk/engine/Context.h>
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

NAMESPACE_BEGIN(Vulk)

//
//
//
class RenderTask : public Sharable<RenderTask>, private NotCopyable {
 public:
  explicit RenderTask(const Vulk::Context& context);
  virtual ~RenderTask(){};

  virtual void render() = 0;

 protected:
  const Vulk::Context& _context;
  Vulk::CommandBuffer::shared_ptr _commandBuffer;
};

//
//
//
class TextureMappingTask : public RenderTask {
 public:
  TextureMappingTask(const Vulk::Context& context);
  ~TextureMappingTask() override;

  void prepareGeometry(const Vulk::VertexBuffer& vertexBuffer,
                       const Vulk::IndexBuffer& indexBuffer, size_t numIndices);
  void prepareUniforms(const glm::mat4& model2world,
                       const glm::mat4& world2view,
                       const glm::mat4& project,
                       bool newFrame = true);
  void prepareInputs(const Vulk::Texture2D& texture);
  void prepareOutputs(const Vulk::Image2D& colorBuffer, const Vulk::DepthImage& depthStencilBuffer);
  void prepareSynchronization(const std::vector<Vulk::Semaphore*> waits,
                              const std::vector<Vulk::Semaphore*> signals,
                              const Vulk::Fence& fence = {});

  Vulk::DescriptorSet::shared_ptr createDescriptorSet();

  void render() override;

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(TextureMappingTask, RenderTask);

 private:
  Vulk::RenderPass::shared_ptr _renderPass;
  Vulk::Pipeline::shared_ptr _pipeline;
  Vulk::DescriptorPool::shared_ptr _descriptorPool;
  uint32_t _vertexBufferBinding = 0U;

  // Geometry
  Vulk::VertexBuffer::shared_ptr_const _vertexBuffer;
  Vulk::IndexBuffer::shared_ptr_const _indexBuffer;
  size_t _numIndices;

  // Inputs
  Vulk::Texture2D::shared_ptr_const _texture;

  // Uniforms: the buffers and mapped memory are internal managed
  std::vector<Vulk::UniformBuffer::shared_ptr> _uniformBuffers;
  std::vector<void*> _uniformBufferMapped;
  size_t _currentUniformBufferIdx = 0;

  // Outputs
  Vulk::Image2D::shared_ptr_const _colorBuffer;
  Vulk::DepthImage::shared_ptr_const _depthStencilBuffer; // TODO this could be internal managed as well.

  // Synchronizations
  std::vector<Vulk::Semaphore*> _waits;
  std::vector<Vulk::Semaphore*> _signals;
  Vulk::Fence::shared_ptr_const _fence;
};

//
//
//
class PresentTask : public RenderTask {
 public:
  PresentTask(const Vulk::Context& context);
  ~PresentTask() override;

  void prepareInput(const Vulk::Image2D& frame);
  void prepareSynchronization(const std::vector<Vulk::Semaphore*> waits);

  void render() override;

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(PresentTask, RenderTask);

 private:
  // Input
  Vulk::Image2D::shared_ptr_const _frame;

  // Synchronizations
  std::vector<Vulk::Semaphore*> _waits;
};


NAMESPACE_END(Vulk)