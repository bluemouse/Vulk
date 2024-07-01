#include <Vulk/engine/Texture2D.h>

#include <Vulk/Device.h>
#include <Vulk/Queue.h>

NAMESPACE_BEGIN(Vulk)

Texture2D::Texture2D(const Device& device,
                     VkFormat format,
                     VkExtent2D extent,
                     VkImageUsageFlags usage,
                     Filter filter,
                     AddressMode addressMode) {
  create(device, format, extent, usage, filter, addressMode);
}

void Texture2D::create(const Device& device,
                       VkFormat format,
                       VkExtent2D extent,
                       VkImageUsageFlags usage,
                       Filter filter,
                       AddressMode addressMode) {
  _image = Image2D::make_shared(device, format, extent, usage);
  _image->allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  _view = ImageView::make_shared(device, image());
  _sampler = Sampler::make_shared(device, filter, addressMode);
}

void Texture2D::destroy() {
  _sampler.reset();
  _view.reset();
  _image.reset();
}

void Texture2D::copyFrom(const Queue& queue,
                         const CommandBuffer& cmdBuffer, const StagingBuffer& stagingBuffer) {
  _image->copyFrom(queue, cmdBuffer, stagingBuffer);
  _image->makeShaderReadable(queue, cmdBuffer);
}

void Texture2D::copyFrom(const Queue& queue,
                         const CommandBuffer& cmdBuffer, const Image2D& srcImage) {
  _image->copyFrom(queue, cmdBuffer, srcImage);
  _image->makeShaderReadable(queue, cmdBuffer);
}

void Texture2D::copyFrom(const Queue& queue,
                         const CommandBuffer& cmdBuffer, const Texture2D& srcTexture) {
  copyFrom(queue, cmdBuffer, srcTexture.image());
}

void Texture2D::blitFrom(const Queue& queue,
                         const CommandBuffer& cmdBuffer, const Image2D& srcImage) {
  _image->blitFrom(queue, cmdBuffer, srcImage);
  _image->makeShaderReadable(queue, cmdBuffer);
}

void Texture2D::blitFrom(const Queue& queue,
                         const CommandBuffer& cmdBuffer, const Texture2D& srcTexture) {
  blitFrom(queue, cmdBuffer, srcTexture.image());
}

NAMESPACE_END(Vulk)
