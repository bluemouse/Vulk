#pragma once

#include <Vulk/Device.h>

#include <Vulk/CommandPool.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/DescriptorPool.h>
#include <Vulk/DescriptorSet.h>
#include <Vulk/DescriptorSetLayout.h>

#include <Vulk/engine/DeviceContext.h>

#include <tbb/concurrent_queue.h>

MI_NAMESPACE_BEGIN(Vulk)

class RenderTask;

class CommandBufferManager : public Sharable<CommandBufferManager>, private NotCopyable {
 public:
  CommandBufferManager(const Device& device, Device::QueueFamilyType queueFamily);
  ~CommandBufferManager();

  CommandBuffer::shared_ptr acquireBuffer();

  void reset();

 private:
  CommandPool::shared_ptr _commandPool;
  tbb::concurrent_queue<CommandBuffer::shared_ptr> _availableCommandBuffers;
  tbb::concurrent_queue<CommandBuffer::shared_ptr> _acquiredCommandBuffers;
};

class DescriptorSetManager : public Sharable<DescriptorSetManager>, private NotCopyable {
 public:
  DescriptorSetManager(const Device& device, std::vector<DescriptorSetLayout::shared_ptr> layouts);
  ~DescriptorSetManager();

  DescriptorSet::shared_ptr acquireSet(const DescriptorSetLayout& layout);

  void reset();

 private:
  DescriptorPool::shared_ptr _descriptorPool;
  tbb::concurrent_queue<DescriptorSet::shared_ptr> _acquiredDescriptorSets;
};

//
// Per frame context to manage the necessities (command buffers, descriptors, ..etc) of rendering a
// frame
//
class FrameContext : public Sharable<FrameContext>, private NotCopyable {
 public:
  FrameContext(DeviceContext::shared_ptr deviceContext, std::vector<RenderTask*> tasks);
  virtual ~FrameContext() = default;

  [[nodiscard]] CommandBuffer::shared_ptr acquireCommandBuffer(Device::QueueFamilyType queueFamily);
  [[nodiscard]] DescriptorSet::shared_ptr acquireDescriptorSet(const DescriptorSetLayout& layout);

  void setFrameRendered(const Fence::shared_ptr& fence) { _frameRendered = fence; }
  void waitFrameRendered() const { _frameRendered->wait(); }

  // Need to be called before each frame rendering to release the previous used resource such as
  // command buffers and descriptor sets.
  void reset();

 private:
  DeviceContext::shared_ptr _deviceContext;

  std::vector<CommandBufferManager::shared_ptr> _commandBufferManagers{
      Device::QueueFamilyType::NUM_QUEUE_FAMILY_TYPES};

  DescriptorSetManager::shared_ptr _descriptorSetManager;

  Fence::shared_ptr _frameRendered;
};

MI_NAMESPACE_END(Vulk)
