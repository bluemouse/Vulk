#include <Vulk/CommandBuffer.h>

#include <Vulk/CommandPool.h>
#include <Vulk/Device.h>
#include <Vulk/Pipeline.h>
#include <Vulk/RenderPass.h>
#include <Vulk/Framebuffer.h>
#include <Vulk/VertexBuffer.h>
#include <Vulk/IndexBuffer.h>
#include <Vulk/DescriptorSet.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_Vulk_BEGIN

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

void CommandBuffer::executeCommands(const Recorder& recorder,
                                    const std::vector<Semaphore*>& waits,
                                    const std::vector<Semaphore*>& signals,
                                    const Fence& fence) const {
  recordCommands(recorder);
  executeCommands(waits, signals, fence);
}

void CommandBuffer::executeSingleTimeCommand(const Recorder& recorder,
                                             const std::vector<Semaphore*>& waits,
                                             const std::vector<Semaphore*>& signals,
                                             const Fence& fence) const {
  recordSingleTimeCommand(recorder);
  executeCommands(waits, signals, fence);
}

void CommandBuffer::waitIdle() const {
  vkQueueWaitIdle(_pool->queue());
}

void CommandBuffer::recordCommands(const Recorder& recorder, bool singleTime) const {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (singleTime) {
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  }

  MI_VERIFY_VKCMD(vkBeginCommandBuffer(_buffer, &beginInfo));

  recorder(*this);

  MI_VERIFY_VKCMD(vkEndCommandBuffer(_buffer));
}

void CommandBuffer::executeCommands(const std::vector<Semaphore*>& waits,
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

void CommandBuffer::beginRenderPass(const RenderPass& renderPass,
                                    const Framebuffer& framebuffer,
                                    const glm::vec4& clearColor,
                                    float /*clearDepth*/,
                                    uint32_t /*clearStencil*/) const {
  VkClearValue clearValue{};
  clearValue.color = {{clearColor.r, clearColor.g, clearColor.b, clearColor.a}};

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = framebuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = framebuffer.extent();
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearValue;

  vkCmdBeginRenderPass(_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::endRenderpass() const {
  vkCmdEndRenderPass(_buffer);
}

void CommandBuffer::bindPipeline(const Pipeline& pipeline) const {
  vkCmdBindPipeline(_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void CommandBuffer::setViewport(const glm::vec2& upperLeft,
                                const glm::vec2& extent,
                                const glm::vec2& depthRange) const {
  VkViewport viewport{};
  viewport.x = upperLeft.x;
  viewport.y = upperLeft.y;
  viewport.width = extent[0];
  viewport.height = extent[1];
  viewport.minDepth = depthRange.x;
  viewport.maxDepth = depthRange.y;
  vkCmdSetViewport(_buffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {static_cast<int32_t>(upperLeft.x), static_cast<int32_t>(upperLeft.y)};
  scissor.extent = {static_cast<uint32_t>(extent[0]), static_cast<uint32_t>(extent[1])};
  vkCmdSetScissor(_buffer, 0, 1, &scissor);
}

void CommandBuffer::bindVertexBuffer(const VertexBuffer& buffer,
                                     uint32_t binding,
                                     uint64_t offset) const {
  VkBuffer vertexBuffers[] = {buffer};
  VkDeviceSize offsets[] = {offset};
  vkCmdBindVertexBuffers(_buffer, binding, 1, vertexBuffers, offsets);
}

void CommandBuffer::bindIndexBuffer(const IndexBuffer& buffer, uint64_t offset) const {
  vkCmdBindIndexBuffer(_buffer, buffer, offset, VK_INDEX_TYPE_UINT16); // TODO choose index type
}

void CommandBuffer::bindDescriptorSet(const Pipeline& pipeline,
                                      const DescriptorSet& descriptorSet) const {
  vkCmdBindDescriptorSets(_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.layout(),
                          0,
                          1,
                          descriptorSet,
                          0,
                          nullptr);
}

void CommandBuffer::drawIndexed(uint32_t indexCount) const {
  vkCmdDrawIndexed(_buffer, indexCount, 1, 0, 0, 0);
}

NAMESPACE_Vulk_END