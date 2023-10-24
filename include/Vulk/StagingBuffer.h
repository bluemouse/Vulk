#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/Buffer.h>

NAMESPACE_Vulk_BEGIN

class Device;
class CommandBuffer;
class Image;

class StagingBuffer : public Buffer {
 public:
  StagingBuffer() = default;
  StagingBuffer(const Device& device, VkDeviceSize size);

  // Transfer the ownership from `rhs` to `this`
  StagingBuffer(StagingBuffer&& rhs) = default;
  StagingBuffer& operator=(StagingBuffer&& rhs) noexcept(false) = default;

  void create(const Device& device, VkDeviceSize size);

  void copyFromHost(const void* src, VkDeviceSize size) { copyFromHost(src, 0, size); }
  void copyFromHost(const void* src, VkDeviceSize offset, VkDeviceSize size);

  void copyToBuffer(const CommandBuffer& commandBuffer,
                    Buffer& dst,
                    const VkBufferCopy& roi,
                    bool waitForFinish = true) const;
  void copyToBuffer(const CommandBuffer& commandBuffer,
                    Buffer& dst,
                    VkDeviceSize size,
                    bool waitForFinish = true) const;

  void copyToImage(const CommandBuffer& commandBuffer,
                   Image& dst,
                   const VkBufferImageCopy& roi,
                   bool waitForFinish = true) const;
  void copyToImage(const CommandBuffer& commandBuffer,
                   Image& dst,
                   uint32_t width,
                   uint32_t height,
                   bool waitForFinish = true) const;
};

NAMESPACE_Vulk_END