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

MI_NAMESPACE_BEGIN(Vulk)

//
//
//
class RenderTask : public Sharable<RenderTask>, private NotCopyable {
  public:
   enum class Type {
    Graphics,
    Compute,
    Transfer
  };

  public:
   explicit RenderTask(const Context& context, Type type);
   virtual ~RenderTask() {};

   virtual std::pair<Semaphore::shared_ptr, Fence::shared_ptr> run() = 0;

   const Device& device() const { return _context.device(); }

  protected:
   const Context& _context;

   CommandBuffer::shared_ptr _commandBuffer;
};

//
//
//
class TextureMappingTask : public RenderTask {
 public:
  explicit TextureMappingTask(const Context& context);
  ~TextureMappingTask() override;

  void prepareGeometry(const VertexBuffer& vertexBuffer,
                       const IndexBuffer& indexBuffer,
                       size_t numIndices);
  void prepareUniforms(const glm::mat4& model2world,
                       const glm::mat4& world2view,
                       const glm::mat4& project,
                       bool newFrame = true);
  void prepareInputs(const Texture2D& texture);
  void prepareOutputs(const Image2D& colorBuffer, const DepthImage& depthStencilBuffer);
  void prepareSynchronization(const std::vector<Semaphore::shared_ptr>& waits = {});

  DescriptorSet::shared_ptr createDescriptorSet();

  std::pair<Semaphore::shared_ptr, Fence::shared_ptr> run() override;

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(TextureMappingTask, RenderTask);

 private:
  RenderPass::shared_ptr _renderPass;
  Pipeline::shared_ptr _pipeline;
  DescriptorPool::shared_ptr _descriptorPool;
  uint32_t _vertexBufferBinding = 0U;

  // Geometry
  VertexBuffer::shared_ptr_const _vertexBuffer;
  IndexBuffer::shared_ptr_const _indexBuffer;
  size_t _numIndices;

  // Inputs
  Texture2D::shared_ptr_const _texture;

  // Uniforms: the buffers and mapped memory are internal managed
  std::vector<UniformBuffer::shared_ptr> _uniformBuffers;
  std::vector<void*> _uniformBufferMapped;
  size_t _currentUniformBufferIdx = 0;

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
  explicit PresentTask(const Context& context);
  ~PresentTask() override;

  void prepareInput(const Image2D& frame);
  void prepareSynchronization(const std::vector<Semaphore::shared_ptr>& waits = {});

  std::pair<Semaphore::shared_ptr, Fence::shared_ptr> run() override;

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(PresentTask, RenderTask);

 private:
  // Input
  Image2D::shared_ptr_const _frame;

  // Synchronizations
  std::vector<Semaphore::shared_ptr> _waits;

  // Store the signals for the each swapchain image. Index of the signal is the same as the index of
  // the swapchain image being presented. We have to store (i.e hold) the signal semaphore since
  // there is no way to sync on finishing presenting the current swapchain image.
  std::vector<Semaphore::shared_ptr> _signals;
};

MI_NAMESPACE_END(Vulk)