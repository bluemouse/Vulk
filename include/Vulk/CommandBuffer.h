#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>

#include <Vulk/Fence.h>
#include <Vulk/Semaphore.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_VULKAN_BEGIN

class Device;
class CommandPool;

class CommandBuffer {
 public:
  CommandBuffer() = default;
  CommandBuffer(const CommandPool& commandPool);
  ~CommandBuffer();

  // Transfer the ownership from `rhs` to `this`
  CommandBuffer(CommandBuffer&& rhs) noexcept;
  CommandBuffer& operator=(CommandBuffer&& rhs) noexcept(false);

  void allocate(const CommandPool& commandPool);
  void free();

  using Recorder = std::function<void(const CommandBuffer& buffer)>;
  // Record the commands played by `recorder` into this command buffer
  void recordCommands(const Recorder& recorder) const { recordCommands(recorder, false); }
  // Execute the commands currently recorded in this command buffer
  void executeCommands(const std::vector<Semaphore*>& waits = {},
                       const std::vector<Semaphore*>& signals = {},
                       const Fence& fence = {}) const;
  // Record the commands played by `recorder` into this command buffer and then execute them
  void executeCommands(const Recorder& recorder,
                       const std::vector<Semaphore*>& waits = {},
                       const std::vector<Semaphore*>& signals = {},
                       const Fence& fence = {}) const;

  void recordSingleTimeCommand(const Recorder& recorder) const { recordCommands(recorder, true); }
  void executeSingleTimeCommand(const Recorder& recorder,
                                const std::vector<Semaphore*>& waits = {},
                                const std::vector<Semaphore*>& signals = {},
                                const Fence& fence = {}) const;

  void waitIdle() const;

  void reset();

  operator VkCommandBuffer() const { return _buffer; }
  operator const VkCommandBuffer*() const { return &_buffer; }

  [[nodiscard]] bool isAllocated() const { return _buffer != VK_NULL_HANDLE; }

 private:
  void recordCommands(const Recorder& recorder, bool singleTime) const;
  void moveFrom(CommandBuffer& rhs);

 private:
  VkCommandBuffer _buffer = VK_NULL_HANDLE;

  const CommandPool* _pool = nullptr;
};

NAMESPACE_VULKAN_END