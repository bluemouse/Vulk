#pragma once

#include <Vulk/Device.h>

#include <Vulk/CommandPool.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/DescriptorPool.h>
#include <Vulk/DescriptorSet.h>
#include <Vulk/DescriptorSetLayout.h>
#include <Vulk/Framebuffer.h>

#include <Vulk/Semaphore.h>
#include <Vulk/Fence.h>

#include <Vulk/engine/DeviceContext.h>

#include <tbb/concurrent_queue.h>

MI_NAMESPACE_BEGIN(Vulk)

class RenderTask;

// Manager of command buffers that can be cached and re-cycled
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

// Manager of descriptor sets that can be cached and re-cycled
// TODO support caching
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

// Manager of sync objects (semaphores and fences) that can be re-cycled
class SyncObjectManager : public Sharable<SyncObjectManager>, private NotCopyable {
 public:
  SyncObjectManager(const Device& device) : _device(device) {}
  ~SyncObjectManager() = default;

  Semaphore::shared_ptr acquireSemaphore();
  Fence::shared_ptr acquireFence();

  void reset();

 private:
  const Device& _device;

  tbb::concurrent_queue<Semaphore::shared_ptr> _availableSemaphores;
  tbb::concurrent_queue<Semaphore::shared_ptr> _acquiredSemaphores;

  tbb::concurrent_queue<Fence::shared_ptr> _availableFences;
  tbb::concurrent_queue<Fence::shared_ptr> _acquiredFences;
};

// Manager of framebuffers that when registered the framebuffer will be kept alive until the next
// call of `reset()`
class FramebufferKeeper : public Sharable<FramebufferKeeper>, private NotCopyable {
 public:
  FramebufferKeeper() {}
  ~FramebufferKeeper() = default;

  void registerFramebuffer(const Framebuffer::shared_ptr& framebuffer);

  void reset();

 private:
  tbb::concurrent_queue<Framebuffer::shared_ptr> _registeredFramebuffers;
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

  [[nodiscard]] Semaphore::shared_ptr acquireSemaphore();
  [[nodiscard]] Fence::shared_ptr acquireFence();

  void registerFramebuffer(const Framebuffer::shared_ptr& framebuffer);

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

  SyncObjectManager::shared_ptr _syncObjectManager;

  FramebufferKeeper::shared_ptr _framebufferManager;

  Fence::shared_ptr _frameRendered;
};

MI_NAMESPACE_END(Vulk)
