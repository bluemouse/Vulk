#include <Vulk/Buffer.h>

#include <cstring>

#include <Vulk/Device.h>
#include <Vulk/DeviceMemory.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/Queue.h>
#include <Vulk/StagingBuffer.h>
#include <Vulk/internal/debug.h>

MI_NAMESPACE_BEGIN(Vulk)

Buffer::Buffer(const Device& device,
               VkDeviceSize size,
               VkBufferUsageFlags usage,
               const BufferCreateInfoOverride& override) {
  create(device, size, usage, override);
}

Buffer::Buffer(const Device& device,
               VkDeviceSize size,
               VkBufferUsageFlags usage,
               VkMemoryPropertyFlags properties,
               const BufferCreateInfoOverride& override)
    : Buffer(device, size, usage, override) {
  allocate(properties);
}

Buffer::~Buffer() {
  if (isAllocated()) {
    destroy();
  }
}

void Buffer::create(const Device& device,
                    VkDeviceSize size,
                    VkBufferUsageFlags usage,
                    const BufferCreateInfoOverride& override) {
  MI_VERIFY(!isCreated());
  _device = device.get_weak();
  _size   = size;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  // bufferInfo.pNext =
  // bufferInfo.flags =
  bufferInfo.size        = size;
  bufferInfo.usage       = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // bufferInfo.queueFamilyIndexCount =
  // bufferInfo.pQueueFamilyIndices =

  if (override) {
    override(&bufferInfo);
  }

  MI_VERIFY_VK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &_buffer));
}

void Buffer::destroy() {
  MI_VERIFY(isCreated());

  if (isAllocated()) {
    free();
  }
  vkDestroyBuffer(device(), _buffer, nullptr);

  _buffer = VK_NULL_HANDLE;
  _size   = 0;

  _device.reset();
}

void Buffer::allocate(VkMemoryPropertyFlags properties) {
  MI_VERIFY(!isAllocated());

  auto& device = this->device();

  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(device, _buffer, &requirements);

  _memory = DeviceMemory::make_shared(device, properties, requirements);

  vkBindBufferMemory(device, _buffer, *_memory, 0);
}

void Buffer::load(const void* data, VkDeviceSize size, VkDeviceSize offset, bool staging) {
  MI_VERIFY(isAllocated());
  MI_VERIFY(offset + size <= _size);

  if (staging) {
    const auto& commandPool = device().commandPool(Device::QueueFamilyType::Transfer);

    CommandBuffer::shared_ptr commandBuffer = CommandBuffer::make_shared(commandPool);
    StagingBuffer::shared_ptr stagingBuffer = StagingBuffer::make_shared(device(), size);
    Fence::shared_ptr fence                 = Fence::make_shared(device());

    stagingBuffer->copyFromHost(data, size);
    stagingBuffer->copyToBuffer(*commandBuffer, *this, {0, offset, size}, *fence);

    // The commandBuffer now should be submitted and in the pending state
    MI_ASSERT(commandBuffer->state() == CommandBuffer::State::Pending);
    fence->wait();
  } else {
    std::memcpy(map(offset, size), data, size);
    unmap();
  }
}

void Buffer::free() {
  MI_VERIFY(isAllocated());
  _memory = nullptr;
}

void Buffer::bind(DeviceMemory& memory, VkDeviceSize offset) {
  MI_VERIFY(isCreated());
  MI_VERIFY(&memory != _memory.get());
  if (isAllocated()) {
    free();
  }
  _memory = memory.get_shared();
  vkBindBufferMemory(device(), _buffer, memory, offset);
}

void* Buffer::map() {
  MI_VERIFY(isAllocated());
  return _memory->map();
}
void* Buffer::map(VkDeviceSize offset, VkDeviceSize size) {
  MI_VERIFY(isAllocated());
  return _memory->map(offset, size);
}

void Buffer::unmap() {
  MI_VERIFY(isAllocated());
  _memory->unmap();
}

bool Buffer::isAllocated() const {
  return isCreated() && (_memory && _memory->isAllocated());
}
bool Buffer::isMapped() const {
  return isAllocated() && _memory->isMapped();
}

MI_NAMESPACE_END(Vulk)