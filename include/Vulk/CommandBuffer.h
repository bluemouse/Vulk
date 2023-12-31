#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>
#include <memory>

// Defined in CMakeLists.txt:GLM_FORCE_DEPTH_ZERO_TO_ONE, GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <Vulk/internal/base.h>

#include <Vulk/Fence.h>
#include <Vulk/Semaphore.h>

NAMESPACE_BEGIN(Vulk)

class Device;
class CommandPool;
class RenderPass;
class Framebuffer;
class Pipeline;
class VertexBuffer;
class IndexBuffer;
class DescriptorSet;

class CommandBuffer : public Sharable<CommandBuffer>, private NotCopyable {
 public:
  CommandBuffer() = default;
  CommandBuffer(const CommandPool& commandPool);
  ~CommandBuffer() override;

  void allocate(const CommandPool& commandPool);
  void free();

  using Recorder = std::function<void(const CommandBuffer& buffer)>;
  // Record the commands played by `recorder` into this command buffer
  void recordCommands(const Recorder& recorder) const { recordCommands(recorder, false); }
  // Execute the commands currently recorded in this command buffer
  void executeCommands(const std::vector<Semaphore*>& waits   = {},
                       const std::vector<Semaphore*>& signals = {},
                       const Fence& fence                     = {}) const;
  // Record the commands played by `recorder` into this command buffer and then execute them
  void executeCommands(const Recorder& recorder,
                       const std::vector<Semaphore*>& waits   = {},
                       const std::vector<Semaphore*>& signals = {},
                       const Fence& fence                     = {}) const;

  void recordSingleTimeCommand(const Recorder& recorder) const { recordCommands(recorder, true); }
  void executeSingleTimeCommand(const Recorder& recorder,
                                const std::vector<Semaphore*>& waits   = {},
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

  void waitIdle() const;

  void reset();

  operator VkCommandBuffer() const { return _buffer; }
  operator const VkCommandBuffer*() const { return &_buffer; }

  [[nodiscard]] bool isAllocated() const { return _buffer != VK_NULL_HANDLE; }

  [[nodiscard]] const CommandPool& pool() const { return *_pool.lock(); }

 private:
  void recordCommands(const Recorder& recorder, bool singleTime) const;

 private:
  VkCommandBuffer _buffer = VK_NULL_HANDLE;

  std::weak_ptr<const CommandPool> _pool;
};

NAMESPACE_END(Vulk)