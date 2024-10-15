#include <Vulk/engine/FrameContext.h>

#include <Vulk/engine/RenderTask.h>
#include <Vulk/DescriptorSetLayout.h>

#include <Vulk/internal/debug.h>

MI_NAMESPACE_BEGIN(Vulk)

//
// CommandBufferManager
//
CommandBufferManager::CommandBufferManager(const Device& device,
                                           Device::QueueFamilyType queueFamily) {
  _commandPool = CommandPool::make_shared(device, queueFamily);
}

CommandBufferManager::~CommandBufferManager() {
  _availableCommandBuffers.clear();
  _acquiredCommandBuffers.clear();
  _commandPool = nullptr;
}

CommandBuffer::shared_ptr CommandBufferManager::acquireBuffer() {
  if (_availableCommandBuffers.empty()) {
    _availableCommandBuffers.push(CommandBuffer::make_shared(*_commandPool));
  }
  CommandBuffer::shared_ptr buffer;
  _availableCommandBuffers.try_pop(buffer);
  _acquiredCommandBuffers.push(buffer);
  return buffer;
}

void CommandBufferManager::reset() {
  _commandPool->reset();

  // Move all elements in _acquiredCommandBuffers to _availableCommandBuffers
  CommandBuffer::shared_ptr buffer;
  while (_acquiredCommandBuffers.try_pop(buffer)) {
    _availableCommandBuffers.push(buffer);
  }
}

//
// DescriptorSetManager
//
DescriptorSetManager::DescriptorSetManager(const Device& device,
                                           std::vector<DescriptorSetLayout::shared_ptr> layouts) {
  std::vector<VkDescriptorPoolSize> poolSizes;
  for (const auto& layout : layouts) {
    for (const auto& poolSize : layout->poolSizes()) {
      poolSizes.push_back(poolSize);
    }
  }
  _descriptorPool = DescriptorPool::make_shared(device, poolSizes, 1);
}

DescriptorSetManager::~DescriptorSetManager() {
}

DescriptorSet::shared_ptr DescriptorSetManager::acquireSet(const DescriptorSetLayout& layout) {
  auto set = DescriptorSet::make_shared(*_descriptorPool, layout);
  _acquiredDescriptorSets.push(set);
  return set;
}

void DescriptorSetManager::reset() {
  _acquiredDescriptorSets.clear();
  _descriptorPool->reset();
}

//
// SyncObjectManager
//
Semaphore::shared_ptr SyncObjectManager::acquireSemaphore() {
  if (_availableSemaphores.empty()) {
    _availableSemaphores.push(Semaphore::make_shared(_device));
  }

  Semaphore::shared_ptr semaphore;
  _availableSemaphores.try_pop(semaphore);
  _acquiredSemaphores.push(semaphore);
  return semaphore;
}

Fence::shared_ptr SyncObjectManager::acquireFence() {
  if (_availableFences.empty()) {
    _availableFences.push(Fence::make_shared(_device));
  }

  Fence::shared_ptr fence;
  _availableFences.try_pop(fence);
  _acquiredFences.push(fence);
  return fence;
}

void SyncObjectManager::reset() {
  Semaphore::shared_ptr semaphore;
  while (_acquiredSemaphores.try_pop(semaphore)) {
    _availableSemaphores.push(semaphore);
  }

  Fence::shared_ptr fence;
  while (_acquiredFences.try_pop(fence)) {
    fence->reset();
    _availableFences.push(fence);
  }
}

//
// FrameBufferKeeper
//
void FramebufferKeeper::registerFramebuffer(const Framebuffer::shared_ptr& framebuffer) {
  _registeredFramebuffers.push(framebuffer);
}

void FramebufferKeeper::reset() {
  Framebuffer::shared_ptr framebuffer;
  while (_registeredFramebuffers.try_pop(framebuffer)) {
    framebuffer.reset();
  }
}

//
// UniformBufferManager
//
UniformBufferManager::UniformBufferManager(const Device& device) : _device(device) {
}

UniformBufferManager::~UniformBufferManager() {
}

UniformBuffer::shared_ptr UniformBufferManager::acquireBuffer(uint32_t id, size_t size) {
  tbb::concurrent_hash_map<uint32_t,  UniformBuffer::shared_ptr>::const_accessor accessor;
  if (tbb::concurrent_hash_map<uint32_t, UniformBuffer::shared_ptr>::const_accessor accessor;
      _buffers.find(accessor, id)) {
    MI_ASSERT_MSG(size == accessor->second->size(),
                  "Uniform buffer with id does not match the requested size");
    return accessor->second;
  }

  auto buffer = UniformBuffer::make_shared(_device, size);
  _buffers.insert({id, buffer});
  return buffer;
}

void UniformBufferManager::reset() {
  _buffers.clear();
}

//
// FrameContext
//
FrameContext::FrameContext(std::shared_ptr<DeviceContext> deviceContext,
                           std::vector<RenderTask*> tasks)
    : _deviceContext(deviceContext) {
  const Device& device = deviceContext->device();

  // Initialize command buffer managers
  Device::QueueFamilyType queueFamilies[] = {Device::QueueFamilyType::Graphics,
                                             Device::QueueFamilyType::Compute,
                                             Device::QueueFamilyType::Transfer};
  for (auto family : queueFamilies) {
    if (deviceContext->isQueueFamilySupported(family)) {
      _commandBufferManagers[family] = std::make_shared<CommandBufferManager>(device, family);
    }
  }

  // Initialize descriptor set manager
  std::vector<DescriptorSetLayout::shared_ptr> descriptorSetLayouts;
  for (auto task : tasks) {
    descriptorSetLayouts.push_back(task->descriptorSetLayout());
  }
  _descriptorSetManager = std::make_shared<DescriptorSetManager>(device, descriptorSetLayouts);
  _syncObjectManager    = std::make_shared<SyncObjectManager>(device);
  _framebufferKeeper    = std::make_shared<FramebufferKeeper>();
  _uniformBufferManager = std::make_shared<UniformBufferManager>(device);

  // Initialize fence of finishing frame rendering
  _frameRendered = Fence::make_shared(device, true /*signaled*/);
}

CommandBuffer::shared_ptr FrameContext::acquireCommandBuffer(Device::QueueFamilyType queueFamily) {
  return _commandBufferManagers[static_cast<size_t>(queueFamily)]->acquireBuffer();
}

DescriptorSet::shared_ptr FrameContext::acquireDescriptorSet(const DescriptorSetLayout& layout) {
  return _descriptorSetManager->acquireSet(layout);
}

Semaphore::shared_ptr FrameContext::acquireSemaphore() {
  return _syncObjectManager->acquireSemaphore();
}

Fence::shared_ptr FrameContext::acquireFence() {
  return _syncObjectManager->acquireFence();
}

UniformBuffer::shared_ptr FrameContext::acquireUniformBuffer(uint32_t id, size_t size) {
  return _uniformBufferManager->acquireBuffer(id, size);
}

void FrameContext::registerFramebuffer(const Framebuffer::shared_ptr& framebuffer) {
  _framebufferKeeper->registerFramebuffer(framebuffer);
}

void FrameContext::reset() {
  for (auto& manager : _commandBufferManagers) {
    if (manager) {
      manager->reset();
    }
  }
  _descriptorSetManager->reset();
  _syncObjectManager->reset();

  _framebufferKeeper->reset();
}

MI_NAMESPACE_END(Vulk)
