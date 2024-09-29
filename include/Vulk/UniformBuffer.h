#pragma once

#include <volk/volk.h>

#include <Vulk/internal/base.h>

#include <Vulk/DeviceMemory.h>
#include <Vulk/Buffer.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;

class UniformBuffer : public Buffer {
 public:
  UniformBuffer(const Device& device, VkDeviceSize size);

  void create(const Device& device, VkDeviceSize size);

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(UniformBuffer, Buffer);
};

MI_NAMESPACE_END(Vulk)