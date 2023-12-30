#include <Vulk/engine/Texture2D.h>

#include <Vulk/Device.h>

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

  _view.create(device, image());
  _sampler = Sampler::make_shared(device, filter, addressMode);
}

void Texture2D::destroy() {
  _sampler.reset();
  _view.destroy();
  _image.reset();
}

void Texture2D::copyFrom(const CommandBuffer& cmdBuffer, const StagingBuffer& stagingBuffer) {
  _image->copyFrom(cmdBuffer, stagingBuffer);
  _image->makeShaderReadable(cmdBuffer);
}

void Texture2D::copyFrom(const CommandBuffer& cmdBuffer, const Image2D& srcImage) {
  _image->copyFrom(cmdBuffer, srcImage);
  _image->makeShaderReadable(cmdBuffer);
}

void Texture2D::copyFrom(const CommandBuffer& cmdBuffer, const Texture2D& srcTexture) {
  copyFrom(cmdBuffer, srcTexture.image());
}

void Texture2D::blitFrom(const CommandBuffer& cmdBuffer, const Image2D& srcImage) {
  _image->blitFrom(cmdBuffer, srcImage);
  _image->makeShaderReadable(cmdBuffer);
}

void Texture2D::blitFrom(const CommandBuffer& cmdBuffer, const Texture2D& srcTexture) {
  blitFrom(cmdBuffer, srcTexture.image());
}

NAMESPACE_END(Vulk)
