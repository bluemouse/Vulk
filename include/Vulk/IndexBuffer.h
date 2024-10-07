#pragma once

#include <volk/volk.h>

#include <Vulk/internal/base.h>

#include <Vulk/Device.h>
#include <Vulk/Queue.h>
#include <Vulk/Buffer.h>
#include <Vulk/engine/TypeTraits.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;
class CommandBuffer;

class IndexBuffer : public Buffer {
 public:
  IndexBuffer(const Device& device, VkDeviceSize size, bool hostVisible = false);
  template <typename Index>
  IndexBuffer(const Device& device, const std::vector<Index>& indices, bool hostVisible = false) {
    create(device, indices, hostVisible);
  }

  // Buffer will be device local and can only be loaded using a staging buffer
  void create(const Device& device, VkDeviceSize size, bool hostVisible = false);
  // Buffer will be device local only and the data will be copied from host to buffer using a
  // staging buffer
  template <typename Index>
  void create(const Device& device, const std::vector<Index>& indices, bool hostVisible = false);

  VkIndexType indexType() const { return _indexType; }

  //
  // Override the sharable types and functions
  //
  MI_DEFINE_SHARED_PTR(IndexBuffer, Buffer);

 private:
  VkIndexType _indexType = VK_INDEX_TYPE_NONE_KHR;
};

template <typename Index>
inline void IndexBuffer::create(const Device& device,
                                const std::vector<Index>& indices,
                                bool hostVisible) {
  _indexType        = IndexTrait<Index>::type;
  VkDeviceSize size = sizeof(Index) * indices.size();
  create(device, size, hostVisible);
  load(indices.data(), size, 0, !hostVisible);
}

MI_NAMESPACE_END(Vulk)