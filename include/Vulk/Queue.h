#pragma once

#include <volk/volk.h>

#include <vector>

// Defined in CMakeLists.txt:GLM_FORCE_DEPTH_ZERO_TO_ONE, GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <Vulk/internal/base.h>
#include <Vulk/Device.h>
#include <Vulk/Fence.h>
#include <Vulk/Semaphore.h>

NAMESPACE_BEGIN(Vulk)

class CommandBuffer;

class Queue : public Sharable<Queue>, private NotCopyable {
 public:
  Queue(const Device& device, Device::QueueFamilyType queueFamily, uint32_t queueFamilyIndex);
  ~Queue() override;

  operator VkQueue() const { return _queue; }

  Device::QueueFamilyType queueFamily() const { return _queueFamily; }
  uint32_t queueFamilyIndex() const { return _queueFamilyIndex; }
  uint32_t queueIndex() const { return _queueIndex; }

  void submitCommands(const CommandBuffer& commandBuffer,
                      const std::vector<Semaphore*>& waits = {},
                      const std::vector<Semaphore*>& signals = {},
                      const Fence& fence = {}) const;
  void waitIdle() const;

  const Device& device() const { return *_device.lock(); }

  void beginLabel(const char* label, const glm::vec4& color = {0.8F, 0.2F, 0.1F, 1.0F}) const;
  void insertLabel(const char* label, const glm::vec4& color = {0.8F, 0.2F, 0.1F, 1.0F}) const;
  void endLabel() const;

  struct ScopedLabel {
    ScopedLabel(const Queue& queue,
                const char* label,
                const glm::vec4& color) : _queue(queue) {
      _queue.beginLabel(label, color);
    }
    ~ScopedLabel() { _queue.endLabel(); }

   private:
    const Queue& _queue;
  };
  [[nodiscard]] std::unique_ptr<ScopedLabel> scopedLabel(const char* label,
                                                         const glm::vec4& color = {0.8F, 0.2F, 0.1F, 1.0F}) const;

 private:
  VkQueue _queue      = VK_NULL_HANDLE;

  Device::QueueFamilyType _queueFamily;
  uint32_t _queueFamilyIndex;
  uint32_t _queueIndex;

  std::weak_ptr<const Device> _device;
};

NAMESPACE_END(Vulk)