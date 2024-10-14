#pragma once

#include <Vulk/engine/Context.h>

#include <Vulk/DescriptorSetLayout.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/Semaphore.h>
#include <Vulk/Fence.h>


MI_NAMESPACE_BEGIN(Vulk)

//
//
//
class RenderTask : public Sharable<RenderTask>, private NotCopyable {
 public:
  enum class Type { Graphics, Compute, Transfer };

 public:
  RenderTask(const DeviceContext::shared_ptr& deviceContext, Type type);
  virtual ~RenderTask(){};

  const Device& device() const { return _deviceContext->device(); }

  virtual std::pair<Semaphore::shared_ptr, Fence::shared_ptr> run() = 0;
  virtual DescriptorSetLayout::shared_ptr descriptorSetLayout() = 0;

  // Before each frame pass, you need to call this function to set the active frame context.
  void setFrameContext(const FrameContext::shared_ptr& frameContext);

 protected:
  const DeviceContext::shared_ptr _deviceContext;
  FrameContext::shared_ptr _frameContext;

  Type _type;

  CommandBuffer::shared_ptr _commandBuffer;
};


MI_NAMESPACE_END(Vulk)