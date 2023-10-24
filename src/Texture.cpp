#include <Vulk/Texture.h>

#include <Vulk/Device.h>

NAMESPACE_Vulk_BEGIN

Texture::Texture(const Device& device,
                 VkFormat format,
                 VkExtent2D extent,
                 Filter filter,
                 AddressMode addressMode) {
  create(device, format, extent, filter, addressMode);
}

void Texture::create(const Device& device,
                     VkFormat format,
                     VkExtent2D extent,
                     Filter filter,
                     AddressMode addressMode) {
  _image.create(device, format, extent);
  _image.allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  _view.create(device, _image);
  _sampler.create(device, filter, addressMode);
}

void Texture::destroy() {
  _image.destroy();
  _view.destroy();
  _sampler.destroy();
}

void Texture::copyFrom(const CommandBuffer& cmdBuffer, const StagingBuffer& stagingBuffer) {
  _image.copyFrom(cmdBuffer, stagingBuffer);
}

NAMESPACE_Vulk_END
