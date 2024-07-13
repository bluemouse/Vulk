#pragma once

#include <volk/volk.h>

#include <functional>
#include <memory>

#include <Vulk/internal/base.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class DeviceMemory : public Sharable<DeviceMemory>, private NotCopyable {
 public:
  DeviceMemory() = default;
  DeviceMemory(const Device& device,
               VkMemoryPropertyFlags properties,
               const VkMemoryRequirements& requirements);
  virtual ~DeviceMemory();

  void allocate(const Device& device,
                VkMemoryPropertyFlags properties,
                const VkMemoryRequirements& requirements);
  void free();

  void* map() { return map(0, _size); }
  void* map(VkDeviceSize offset, VkDeviceSize size);
  void unmap();

  operator VkDeviceMemory() const { return _memory; }

  [[nodiscard]] VkDeviceSize size() const { return _size; }
  [[nodiscard]] bool isAllocated() const { return _memory != VK_NULL_HANDLE; }
  [[nodiscard]] bool isMapped() const { return _mappedMemory != nullptr; }
  [[nodiscard]] bool isHostVisible() const { return _hostVisible; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

 private:
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

 private:
  VkDeviceMemory _memory = VK_NULL_HANDLE;

  VkDeviceSize _size = 0; // in bytes

  bool _hostVisible   = false;
  void* _mappedMemory = nullptr;

  std::weak_ptr<const Device> _device;
};

NAMESPACE_END(Vulk)