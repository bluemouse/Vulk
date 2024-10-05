#pragma once

#include <volk/volk.h>

#include <functional>
#include <vector>
#include <memory>

// Defined in CMakeLists.txt:GLM_FORCE_DEPTH_ZERO_TO_ONE, GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <Vulk/internal/base.h>

#include <Vulk/Fence.h>
#include <Vulk/Semaphore.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;
class CommandPool;
class RenderPass;
class Framebuffer;
class Pipeline;
class VertexBuffer;
class IndexBuffer;
class DescriptorSet;
class Queue;

/// @brief
/// CommandBuffer is a wrapper around VkCommandBuffer. It is not thread-safe so it should be used
/// only in the thread where it was created.
class CommandBuffer : public Sharable<CommandBuffer>, private NotCopyable {
 public:
  enum class Level {
    Primary   = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    Secondary = VK_COMMAND_BUFFER_LEVEL_SECONDARY
  };

  enum class Usage {
    OneTimeSubmit      = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    RenderPassContinue = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
    SimultaneousUse    = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    Default            = 0
  };

 public:
  CommandBuffer() = default;
  CommandBuffer(const CommandPool& commandPool, Level level = Level::Primary);
  ~CommandBuffer() override;

  void allocate(const CommandPool& commandPool, Level level = Level::Primary);
  void free();

  using Recorder = std::function<void(const CommandBuffer& buffer)>;
  // Record the commands played by `recorder` into this command buffer
  void recordCommands(const Recorder& recorder, Usage usage = Usage::Default) const;

  void beginRecording(Usage usage = Usage::Default) const;
  void endRecording() const;

  void submitCommands(const std::vector<Semaphore*>& waits   = {},
                      const std::vector<Semaphore*>& signals = {},
                      const Fence& fence                     = {}) const;

  void beginRenderPass(const RenderPass& renderPass,
                       const Framebuffer& framebuffer,
                       const glm::vec4& clearColor = {0.0F, 0.0F, 0.0F, 1.0F},
                       float clearDepth            = 1.0F,
                       uint32_t clearStencil       = 0) const;
  void endRenderpass() const;

  void setViewport(const glm::vec2& upperLeft,
                   const glm::vec2& extent,
                   const glm::vec2& depthRange = {0.0F, 1.0F}) const;

  void bindPipeline(const Pipeline& pipeline) const;

  void bindVertexBuffer(const VertexBuffer& buffer, uint32_t binding, uint64_t offset = 0) const;
  void bindIndexBuffer(const IndexBuffer& buffer, uint64_t offset = 0) const;
  void bindDescriptorSet(const Pipeline& pipeline, const DescriptorSet& descriptorSet) const;

  void drawIndexed(uint32_t indexCount) const;

  void reset();

  operator VkCommandBuffer() const { return _buffer; }
  operator const VkCommandBuffer*() const { return &_buffer; }

  [[nodiscard]] bool isAllocated() const { return _buffer != VK_NULL_HANDLE; }

  [[nodiscard]] const CommandPool& pool() const { return *_pool.lock(); }
  [[nodiscard]] const Queue& queue() const;

  void beginLabel(const char* label, const glm::vec4& color = {0.2F, 0.8F, 0.2F, 1.0F}) const;
  void insertLabel(const char* label, const glm::vec4& color = {0.2F, 0.8F, 0.2F, 1.0F}) const;
  void endLabel() const;

  struct ScopedLabel {
    ScopedLabel(const CommandBuffer& commandBuffer, const char* label, const glm::vec4& color)
        : _commandBuffer(commandBuffer) {
      _commandBuffer.beginLabel(label, color);
    }
    ~ScopedLabel() { _commandBuffer.endLabel(); }

   private:
    const CommandBuffer& _commandBuffer;
  };
  [[nodiscard]] std::unique_ptr<ScopedLabel> scopedLabel(
      const char* label,
      const glm::vec4& color = {0.2F, 0.8F, 0.2F, 1.0F}) const;

 private:
  VkCommandBuffer _buffer = VK_NULL_HANDLE;

  // When _recordingStack > 0 (i.e. there are outer begin/end recording), beginRecording(),
  // endRecording() and submitCommands() are no-op.
  mutable uint32_t _recordingStack = 0;

  std::weak_ptr<const CommandPool> _pool;
};

MI_NAMESPACE_END(Vulk)
