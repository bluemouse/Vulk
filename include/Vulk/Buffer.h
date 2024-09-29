#pragma once

#include <volk/volk.h>

#include <functional>
#include <memory>

#include <Vulk/internal/base.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;
class CommandBuffer;
class Queue;
class DeviceMemory;

class Buffer : public Sharable<Buffer>, private NotCopyable {
 public:
  using BufferCreateInfoOverride = std::function<void(VkBufferCreateInfo*)>;

 public:
  Buffer() = default;
  Buffer(const Device& device,
         VkDeviceSize size,
         VkBufferUsageFlags usage,
         const BufferCreateInfoOverride& override = {});
  Buffer(const Device& device,
         VkDeviceSize size,
         VkBufferUsageFlags usage,
         VkMemoryPropertyFlags properties,
         const BufferCreateInfoOverride& override = {});

  virtual ~Buffer() override;

  void create(const Device& device,
              VkDeviceSize size,
              VkBufferUsageFlags usage,
              const BufferCreateInfoOverride& override = {});
  void destroy();

  void allocate(VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  void free();

  void load(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
  void load(const Queue& queue,
            const CommandBuffer& stagingCommandBuffer,
            const void* data,
            VkDeviceSize size,
            VkDeviceSize offset = 0);

  void bind(DeviceMemory& memory, VkDeviceSize offset = 0);

  void* map();
  void* map(VkDeviceSize offset, VkDeviceSize size);
  void unmap();

  operator VkBuffer() const { return _buffer; }
  [[nodiscard]] const DeviceMemory& memory() const { return *_memory; }
  [[nodiscard]] VkDeviceSize size() const { return _size; }

  [[nodiscard]] bool isCreated() const { return _buffer != VK_NULL_HANDLE; }
  [[nodiscard]] bool isAllocated() const;
  [[nodiscard]] bool isMapped() const;

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

 protected:
  VkBuffer _buffer = VK_NULL_HANDLE;

  VkDeviceSize _size = 0; // in bytes
  std::shared_ptr<DeviceMemory> _memory;

  std::weak_ptr<const Device> _device;
};

MI_NAMESPACE_END(Vulk)