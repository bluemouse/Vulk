#include <Vulk/CommandBuffer.h>

#include <Vulk/internal/helpers.h>
#include <Vulk/internal/debug.h>

#include <Vulk/CommandPool.h>
#include <Vulk/Device.h>
#include <Vulk/Pipeline.h>
#include <Vulk/RenderPass.h>
#include <Vulk/Framebuffer.h>
#include <Vulk/VertexBuffer.h>
#include <Vulk/IndexBuffer.h>
#include <Vulk/DescriptorSet.h>

NAMESPACE_BEGIN(Vulk)

CommandBuffer::CommandBuffer(const CommandPool& commandPool, Level level) {
  allocate(commandPool, level);
}

CommandBuffer::~CommandBuffer() {
  if (isAllocated()) {
    free();
  }
}

const Queue& CommandBuffer::queue() const {
  return pool().queue();
}

void CommandBuffer::allocate(const CommandPool& commandPool, Level level) {
  MI_VERIFY(!isAllocated());
  _pool = commandPool.get_weak();

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = commandPool;
  allocInfo.level              = static_cast<VkCommandBufferLevel>(level);
  allocInfo.commandBufferCount = 1;

  MI_VERIFY_VKCMD(vkAllocateCommandBuffers(commandPool.device(), &allocInfo, &_buffer));
}

void CommandBuffer::reset() {
  vkResetCommandBuffer(_buffer, VkCommandBufferResetFlagBits(0));
}

void CommandBuffer::free() {
  MI_VERIFY(isAllocated());

  const auto& pool = this->pool();
  vkFreeCommandBuffers(pool.device(), pool, 1, &_buffer);

  _buffer = VK_NULL_HANDLE;
  _pool.reset();
}

void CommandBuffer::recordCommands(const Recorder& recorder, Usage usage) const {
  beginRecording(usage);
  recorder(*this);
  endRecording();
}

void CommandBuffer::beginRecording(Usage usage) const {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = static_cast<VkCommandBufferUsageFlags>(usage);

  MI_VERIFY_VKCMD(vkBeginCommandBuffer(_buffer, &beginInfo));
}

void CommandBuffer::endRecording() const {
  MI_VERIFY_VKCMD(vkEndCommandBuffer(_buffer));
}

void CommandBuffer::beginRenderPass(const RenderPass& renderPass,
                                    const Framebuffer& framebuffer,
                                    const glm::vec4& clearColor,
                                    float clearDepth,
                                    uint32_t clearStencil) const {
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass        = renderPass;
  renderPassInfo.framebuffer       = framebuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = framebuffer.extent();

  // Note that the order of the clear values should match the order of the attachments in the
  // framebuffer.
  std::vector<VkClearValue> clearValues{1};
  clearValues[0].color = {{clearColor.r, clearColor.g, clearColor.b, clearColor.a}};
  if (renderPass.hasDepthStencilAttachment()) {
    clearValues.push_back(VkClearValue{});
    clearValues[1].depthStencil = {clearDepth, clearStencil};
  }

  renderPassInfo.clearValueCount = clearValues.size();
  renderPassInfo.pClearValues    = clearValues.data();

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
  viewport.x        = upperLeft.x;
  viewport.y        = upperLeft.y;
  viewport.width    = extent[0];
  viewport.height   = extent[1];
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
  VkDeviceSize offsets[]   = {offset};
  vkCmdBindVertexBuffers(_buffer, binding, 1, vertexBuffers, offsets);
}

void CommandBuffer::bindIndexBuffer(const IndexBuffer& buffer, uint64_t offset) const {
  vkCmdBindIndexBuffer(_buffer, buffer, offset, buffer.indexType());
}

void CommandBuffer::bindDescriptorSet(const Pipeline& pipeline,
                                      const DescriptorSet& descriptorSet) const {
  vkCmdBindDescriptorSets(
      _buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout(), 0, 1, descriptorSet, 0, nullptr);
}

void CommandBuffer::drawIndexed(uint32_t indexCount) const {
  vkCmdDrawIndexed(_buffer, indexCount, 1, 0, 0, 0);
}

#if defined(ENABLE_VULKAN_DEBUG_UTILS)

void CommandBuffer::beginLabel(const char* label, const glm::vec4& color) const {
  VkDebugUtilsLabelEXT labelInfo{};
  labelInfo.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  labelInfo.pLabelName = label;
  labelInfo.color[0]   = color.r;
  labelInfo.color[1]   = color.g;
  labelInfo.color[2]   = color.b;
  labelInfo.color[3]   = color.a;
  vkCmdBeginDebugUtilsLabelEXT(_buffer, &labelInfo);
}
void CommandBuffer::insertLabel(const char* label, const glm::vec4& color) const {
  VkDebugUtilsLabelEXT labelInfo{};
  labelInfo.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  labelInfo.pLabelName = label;
  labelInfo.color[0]   = color.r;
  labelInfo.color[1]   = color.g;
  labelInfo.color[2]   = color.b;
  labelInfo.color[3]   = color.a;
  vkCmdInsertDebugUtilsLabelEXT(_buffer, &labelInfo);
}
void CommandBuffer::endLabel() const {
  vkCmdEndDebugUtilsLabelEXT(_buffer);
}
auto CommandBuffer::scopedLabel(const char* label, const glm::vec4& color) const
    -> std::unique_ptr<ScopedLabel> {
  return std::make_unique<ScopedLabel>(*this, label, color);
}

#else

void CommandBuffer::beginLabel(const char*, const glm::vec4&) const {
}
void CommandBuffer::insertLabel(const char*, const glm::vec4&) const {
}
void CommandBuffer::endLabel() const {
}
auto CommandBuffer::scopedLabel(const char*, const glm::vec4&) const
    -> std::unique_ptr<ScopedLabel> {
  return nullptr;
}

#endif // defined(ENABLE_VULKAN_DEBUG_UTILS)

NAMESPACE_END(Vulk)
