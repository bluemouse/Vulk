#include <Vulk/DepthImage.h>

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/StagingBuffer.h>

NAMESPACE_Vulk_BEGIN

DepthImage::DepthImage(const Device& device,
                       VkFormat format,
                       VkExtent2D extent,
                       const ImageCreateInfoOverride& override) {
  create(device, format, extent, override);
}

DepthImage::DepthImage(const Device& device,
                       VkFormat format,
                       VkExtent2D extent,
                       VkMemoryPropertyFlags properties,
                       const ImageCreateInfoOverride& override)
    : DepthImage(device, format, extent, override) {
  allocate(properties);
}

DepthImage::DepthImage(DepthImage&& rhs) noexcept : Image(std::move(rhs)) {
}

DepthImage& DepthImage::operator=(DepthImage&& rhs) noexcept(false) {
  Image::operator=(std::move(rhs));
  return *this;
}

void DepthImage::create(const Device& device,
                        VkFormat format,
                        VkExtent2D extent,
                        const ImageCreateInfoOverride& override) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent = {extent.width, extent.height, 1};
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  // The Vulkan spec states: initialLayout must be VK_IMAGE_LAYOUT_UNDEFINED or
  // VK_IMAGE_LAYOUT_PREINITIALIZED
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (override) {
    override(&imageInfo);
  }

  Image::create(device, imageInfo);
}

void DepthImage::promoteLayout(const CommandBuffer& cmdBuffer, bool waitForFinish) {
  transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, waitForFinish);
}


NAMESPACE_Vulk_END
