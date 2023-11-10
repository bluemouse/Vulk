#include <Vulk/Texture2D.h>

#include <Vulk/Device.h>

NAMESPACE_Vulk_BEGIN

Texture2D::Texture2D(const Device& device,
                     VkFormat format,
                     VkExtent2D extent,
                     Filter filter,
                     AddressMode addressMode) {
  create(device, format, extent, filter, addressMode);
}

void Texture2D::create(const Device& device,
                       VkFormat format,
                       VkExtent2D extent,
                       Filter filter,
                       AddressMode addressMode) {
  _image.create(device, format, extent);
  _image.allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  _view.create(device, _image);
  _sampler.create(device, filter, addressMode);
}

void Texture2D::destroy() {
  _image.destroy();
  _view.destroy();
  _sampler.destroy();
}

void Texture2D::copyFrom(const CommandBuffer& cmdBuffer, const StagingBuffer& stagingBuffer) {
  _image.copyFrom(cmdBuffer, stagingBuffer);
}

NAMESPACE_Vulk_END
