#pragma once

#include <Vulk/engine/DeviceContext.h>
#include <Vulk/engine/FrameContext.h>

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
  RenderTask(const DeviceContext& deviceContext, Type type);
  virtual ~RenderTask(){};

  const Device& device() const { return _deviceContext.device(); }

  virtual std::pair<Semaphore::shared_ptr, Fence::shared_ptr> run() = 0;
  [[nodiscard]] virtual DescriptorSetLayout::shared_ptr descriptorSetLayout() = 0;

  // Before each frame pass, you need to call this function to set the active frame context.
  void setFrameContext(FrameContext& frameContext);

  [[nodiscard]] uint32_t id() const { return _id; }

 protected:
  const DeviceContext& _deviceContext;
  FrameContext* _frameContext;

  Type _type;
  uint32_t _id = 0; // Unique ID for each render task. 0 indicates invalid ID.
  static uint32_t _nextId; // The next id to assign to the next render task.

  CommandBuffer::shared_ptr _commandBuffer;
};


MI_NAMESPACE_END(Vulk)