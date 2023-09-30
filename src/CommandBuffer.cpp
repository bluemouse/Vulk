#include <Vulk/CommandBuffer.h>

#include <Vulk/CommandPool.h>
#include <Vulk/Device.h>
#include <Vulk/Pipeline.h>
#include <Vulk/RenderPass.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_VULKAN_BEGIN

CommandBuffer::CommandBuffer(const CommandPool& commandPool) {
  allocate(commandPool);
}

CommandBuffer::~CommandBuffer() {
  if (isAllocated()) {
    free();
  }
}

CommandBuffer::CommandBuffer(CommandBuffer&& rhs) noexcept {
  moveFrom(rhs);
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void CommandBuffer::moveFrom(CommandBuffer& rhs) {
  MI_VERIFY(!isAllocated());
  _buffer = rhs._buffer;
  _pool = rhs._pool;

  rhs._buffer = VK_NULL_HANDLE;
  rhs._pool = nullptr;
}

void CommandBuffer::allocate(const CommandPool& commandPool) {
  MI_VERIFY(!isAllocated());
  _pool = &commandPool;

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  MI_VERIFY_VKCMD(vkAllocateCommandBuffers(commandPool.device(), &allocInfo, &_buffer));
}

void CommandBuffer::reset() {
  vkResetCommandBuffer(_buffer, VkCommandBufferResetFlagBits(0));
}

void CommandBuffer::free() {
  MI_VERIFY(isAllocated());

  vkFreeCommandBuffers(_pool->device(), *_pool, 1, &_buffer);

  _buffer = VK_NULL_HANDLE;
  _pool = nullptr;
}

void CommandBuffer::executeCommand(const Recorder& recorder,
                                   const std::vector<Semaphore*>& waits,
                                   const std::vector<Semaphore*>& signals,
                                   const Fence& fence) const {
  recordCommand(recorder);
  executeCommand(waits, signals, fence);
}

void CommandBuffer::executeSingleTimeCommand(const Recorder& recorder,
                                             const std::vector<Semaphore*>& waits,
                                             const std::vector<Semaphore*>& signals,
                                             const Fence& fence) const {
  recordSingleTimeCommand(recorder);
  executeCommand(waits, signals, fence);
}

void CommandBuffer::waitIdle() const {
  vkQueueWaitIdle(_pool->queue());
}

void CommandBuffer::recordCommand(const Recorder& recorder, bool singleTime) const {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (singleTime) {
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  }

  MI_VERIFY_VKCMD(vkBeginCommandBuffer(_buffer, &beginInfo));

  recorder(*this);

  MI_VERIFY_VKCMD(vkEndCommandBuffer(_buffer));
}

void CommandBuffer::executeCommand(const std::vector<Semaphore*>& waits,
                                   const std::vector<Semaphore*>& signals,
                                   const Fence& fence) const {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &_buffer;

  std::vector<VkSemaphore> waitSemaphores;
  if (!waits.empty()) {
    waitSemaphores.reserve(waits.size());
    for (const auto& wait : waits) {
      waitSemaphores.push_back(*wait);
    }

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = static_cast<VkPipelineStageFlags*>(waitStages);
  }
  std::vector<VkSemaphore> signalSemaphores;
  if (!signals.empty()) {
    signalSemaphores.reserve(signals.size());
    for (const auto& signal : signals) {
      signalSemaphores.push_back(*signal);
    }

    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();
  }
  VkQueue queue = _pool->queue();
  MI_VERIFY_VKCMD(vkQueueSubmit(queue, 1, &submitInfo, fence));
}

NAMESPACE_VULKAN_END