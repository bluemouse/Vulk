#include <Vulk/Image2D.h>

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/StagingBuffer.h>

#include <Vulk/internal/debug.h>

MI_NAMESPACE_BEGIN(Vulk)

Image2D::Image2D(const Device& device,
                 VkFormat format,
                 VkExtent2D extent,
                 VkImageUsageFlags usage,
                 const ImageCreateInfoOverride& override) {
  create(device, format, extent, usage, override);
}

Image2D::Image2D(const Device& device,
                 VkFormat format,
                 VkExtent2D extent,
                 VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties,
                 const ImageCreateInfoOverride& override)
    : Image2D(device, format, extent, usage, override) {
  allocate(properties);
}

Image2D::Image2D(VkImage image, VkFormat format, VkExtent2D extent) : _external{true} {
  _image  = image;
  _format = format;
  _extent = {extent.width, extent.height, 1};
}

void Image2D::create(const Device& device,
                     VkFormat format,
                     VkExtent2D extent,
                     VkImageUsageFlags usage,
                     const ImageCreateInfoOverride& override) {
  MI_ASSERT(extent.width > 0 && extent.height > 0);

  VkImageCreateInfo imageInfo{};
  imageInfo.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType   = VK_IMAGE_TYPE_2D;
  imageInfo.extent      = {extent.width, extent.height, 1};
  imageInfo.mipLevels   = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format      = format;
  imageInfo.tiling      = VK_IMAGE_TILING_OPTIMAL;
  // The Vulkan spec states: initialLayout must be VK_IMAGE_LAYOUT_UNDEFINED or
  // VK_IMAGE_LAYOUT_PREINITIALIZED
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  imageInfo.usage = usage;

  // imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
  //                   VK_IMAGE_USAGE_SAMPLED_BIT;

  if (override) {
    override(&imageInfo);
  }

  Image::create(device, imageInfo);
}

Image2D::~Image2D() {
  if (isExternal()) {
    // We need to reset _image to VK_NULL_HANDLE so that Image::destroy() in ~Image() won't try to
    // destroy it.
    _image  = VK_NULL_HANDLE;
    _format = VK_FORMAT_UNDEFINED;
    _extent = {0, 0, 0};
  }
}

void Image2D::destroy() {
  MI_VERIFY(!isExternal());
  Image::destroy();
}

void Image2D::allocate(VkMemoryPropertyFlags properties) {
  MI_VERIFY(!isExternal());
  Image::allocate(properties);
}

void Image2D::free() {
  MI_VERIFY(!isExternal());
  Image::free();
}

MI_NAMESPACE_END(Vulk)
