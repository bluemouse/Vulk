#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/base.h>

#include <Vulk/Buffer.h>

NAMESPACE_BEGIN(Vulk)

class Device;
class CommandBuffer;
class Queue;
class Image;

class StagingBuffer : public Buffer {
 public:
  StagingBuffer(const Device& device, VkDeviceSize size, const void* data = nullptr);

  void create(const Device& device, VkDeviceSize size);

  void copyFromHost(const void* src, VkDeviceSize size) { copyFromHost(src, 0, size); }
  void copyFromHost(const void* src, VkDeviceSize offset, VkDeviceSize size);

  void copyToBuffer(const Queue& queue,
                    const CommandBuffer& commandBuffer,
                    Buffer& dst,
                    const VkBufferCopy& roi,
                    bool waitForFinish = true) const;
  void copyToBuffer(const Queue& queue,
                    const CommandBuffer& commandBuffer,
                    Buffer& dst,
                    VkDeviceSize size,
                    bool waitForFinish = true) const;

  void copyToImage(const Queue& queue,
                   const CommandBuffer& commandBuffer,
                   Image& dst,
                   const VkBufferImageCopy& roi,
                   bool waitForFinish = true) const;
  void copyToImage(const Queue& queue,
                   const CommandBuffer& commandBuffer,
                   Image& dst,
                   uint32_t width,
                   uint32_t height,
                   bool waitForFinish = true) const;

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(StagingBuffer, Buffer);
};

NAMESPACE_END(Vulk)