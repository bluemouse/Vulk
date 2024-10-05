#include <Vulk/engine/Texture2D.h>

#include <Vulk/Device.h>
#include <Vulk/Queue.h>
#include <Vulk/CommandBuffer.h>

MI_NAMESPACE_BEGIN(Vulk)

Texture2D::Texture2D(const Device& device,
                     VkFormat format,
                     VkExtent2D extent,
                     Image2D::Usage usage,
                     Filter filter,
                     AddressMode addressMode) {
  create(device, format, extent, usage, filter, addressMode);
}

void Texture2D::create(const Device& device,
                       VkFormat format,
                       VkExtent2D extent,
                       Image2D::Usage usage,
                       Filter filter,
                       AddressMode addressMode) {
  _image = Image2D::make_shared(device, format, extent, usage);
  _image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  _view    = ImageView::make_shared(device, image());
  _sampler = Sampler::make_shared(device, filter, addressMode);
}

void Texture2D::destroy() {
  _sampler.reset();
  _view.reset();
  _image.reset();
}

void Texture2D::copyFrom(const CommandBuffer& commandBuffer,
                         const StagingBuffer& stagingBuffer,
                         const std::vector<Semaphore*>& waits,
                         const std::vector<Semaphore*>& signals,
                         const Fence& fence) {
  commandBuffer.beginRecording(CommandBuffer::Usage::OneTimeSubmit);
  {
    _image->copyFrom(commandBuffer, stagingBuffer);
    _image->transitToNewLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
  commandBuffer.endRecording();
  commandBuffer.submitCommands(waits, signals, fence);
}

void Texture2D::copyFrom(const CommandBuffer& commandBuffer,
                         const Image2D& srcImage,
                         const std::vector<Semaphore*>& waits,
                         const std::vector<Semaphore*>& signals,
                         const Fence& fence) {
  commandBuffer.beginRecording(CommandBuffer::Usage::OneTimeSubmit);
  {
    _image->copyFrom(commandBuffer, srcImage);
    _image->transitToNewLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
  commandBuffer.endRecording();
  commandBuffer.submitCommands(waits, signals, fence);
}

void Texture2D::copyFrom(const CommandBuffer& commandBuffer,
                         const Texture2D& srcTexture,
                         const std::vector<Semaphore*>& waits,
                         const std::vector<Semaphore*>& signals,
                         const Fence& fence) {
  copyFrom(commandBuffer, srcTexture.image(), waits, signals, fence);
}

void Texture2D::blitFrom(const CommandBuffer& commandBuffer,
                         const Image2D& srcImage,
                         const std::vector<Semaphore*>& waits,
                         const std::vector<Semaphore*>& signals,
                         const Fence& fence) {
  commandBuffer.beginRecording(CommandBuffer::Usage::OneTimeSubmit);
  {
    _image->blitFrom(commandBuffer, srcImage);
    _image->transitToNewLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
  commandBuffer.endRecording();
  commandBuffer.submitCommands(waits, signals, fence);
}

void Texture2D::blitFrom(const CommandBuffer& commandBuffer, const Texture2D& srcTexture,
                         const std::vector<Semaphore*>& waits,
                         const std::vector<Semaphore*>& signals,
                         const Fence& fence) {
  blitFrom(commandBuffer, srcTexture.image(), waits, signals, fence);
}

MI_NAMESPACE_END(Vulk)
