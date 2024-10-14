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
  MI_ASSERT(_acquiredCommandBuffers.empty());
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

  // Initialize fence of finishing frame rendering
  _frameRendered = Fence::make_shared(device, true /*signaled*/);
}

CommandBuffer::shared_ptr FrameContext::acquireCommandBuffer(Device::QueueFamilyType queueFamily) {
  return _commandBufferManagers[static_cast<size_t>(queueFamily)]->acquireBuffer();
}

DescriptorSet::shared_ptr FrameContext::acquireDescriptorSet(const DescriptorSetLayout& layout) {
  return _descriptorSetManager->acquireSet(layout);
}

void FrameContext::reset() {
  for (auto& manager : _commandBufferManagers) {
    if (manager) {
      manager->reset();
    }
  }
  _descriptorSetManager->reset();
}

MI_NAMESPACE_END(Vulk)
