#include <Vulk/DepthImage.h>

#include <Vulk/Device.h>
#include <Vulk/PhysicalDevice.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/StagingBuffer.h>

#include <Vulk/internal/debug.h>

MI_NAMESPACE_BEGIN(Vulk)

DepthImage::DepthImage(const Device& device,
                       VkExtent2D extent,
                       uint32_t depthBits,
                       uint32_t stencilBits,
                       const ImageCreateInfoOverride& override) {
  create(device, extent, depthBits, stencilBits, override);
}

DepthImage::DepthImage(const Device& device,
                       VkExtent2D extent,
                       VkFormat format,
                       const ImageCreateInfoOverride& override) {
  create(device, extent, format, override);
}

void DepthImage::create(const Device& device,
                        VkExtent2D extent,
                        uint32_t depthBits,
                        uint32_t stencilBits,
                        const ImageCreateInfoOverride& override) {
  create(device, extent, findFormat(depthBits, stencilBits), override);
}

void DepthImage::create(const Device& device,
                        VkExtent2D extent,
                        VkFormat format,
                        const ImageCreateInfoOverride& override) {
  _format = format;

  constexpr auto tiling  = VK_IMAGE_TILING_OPTIMAL;
  constexpr auto feature = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  constexpr auto usage   = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  MI_VERIFY(_format != VK_FORMAT_UNDEFINED);
  MI_VERIFY(device.physicalDevice().isFormatSupported(_format, tiling, feature));

  VkImageCreateInfo imageInfo{};
  imageInfo.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType   = VK_IMAGE_TYPE_2D;
  imageInfo.extent      = {extent.width, extent.height, 1};
  imageInfo.mipLevels   = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format      = _format;
  imageInfo.tiling      = tiling;
  imageInfo.usage       = usage;
  imageInfo.samples     = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // The Vulkan spec states: initialLayout must be VK_IMAGE_LAYOUT_UNDEFINED or
  // VK_IMAGE_LAYOUT_PREINITIALIZED
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  if (override) {
    override(&imageInfo);
  }

  Image::create(device, imageInfo);
}

void DepthImage::promoteLayout(const CommandBuffer& cmdBuffer) {
  transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkFormat DepthImage::findFormat(uint32_t depthBits, uint32_t stencilBits) {
  struct FormatProps {
    VkFormat format;
    uint32_t depthBits;
    uint32_t stencilBits;
  };
  constexpr std::array<FormatProps, 7> availableFormats = {{{VK_FORMAT_D16_UNORM, 16, 0},
                                                            {VK_FORMAT_X8_D24_UNORM_PACK32, 24, 0},
                                                            {VK_FORMAT_D32_SFLOAT, 32, 0},
                                                            {VK_FORMAT_S8_UINT, 0, 8},
                                                            {VK_FORMAT_D16_UNORM_S8_UINT, 16, 8},
                                                            {VK_FORMAT_D24_UNORM_S8_UINT, 24, 8},
                                                            {VK_FORMAT_D32_SFLOAT_S8_UINT, 32, 8}}};
  for (const auto& prop : availableFormats) {
    if (prop.depthBits == depthBits && prop.stencilBits == stencilBits) {
      return prop.format;
    }
  }
  return VK_FORMAT_UNDEFINED;
}

bool DepthImage::hasDepthBits() const {
  return _format != VK_FORMAT_S8_UINT;
}

bool DepthImage::hasStencilBits() const {
  return _format == VK_FORMAT_S8_UINT || _format == VK_FORMAT_D16_UNORM_S8_UINT ||
         _format == VK_FORMAT_D24_UNORM_S8_UINT || _format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

MI_NAMESPACE_END(Vulk)
