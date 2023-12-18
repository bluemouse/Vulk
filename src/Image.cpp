#include <Vulk/Image.h>

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/StagingBuffer.h>
#include <Vulk/Utility.h>

#include <Vulk/helpers_vulkan.h>

#include <set>

namespace {
VkImageAspectFlags selectAspectMask(VkImageLayout layout) {
  struct DepthStencilAspect {
    VkImageLayout layout;
    VkImageAspectFlags mask{VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT};
  };

  const DepthStencilAspect depthStencilAspects[] = {
      {VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT},
      {VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT},
      {VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_STENCIL_BIT},
      {VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_STENCIL_BIT},
      {VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
      {VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL},
      {VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL},
      {VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL}};

  for (auto aspect : depthStencilAspects) {
    if (aspect.layout == layout) {
      return aspect.mask;
    }
  }

  return VK_IMAGE_ASPECT_COLOR_BIT;
}
bool isLayoutReadOnly(VkImageLayout layout) {
  const std::set<VkImageLayout> readOnlyLayouts = {
      VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
      // VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
      // VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL};

  return readOnlyLayouts.find(layout) != std::end(readOnlyLayouts);
}

void selectStageAccess(VkImageLayout layout, VkPipelineStageFlags& stage, VkAccessFlags& access) {
  switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
      stage  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      access = VK_ACCESS_NONE;
      break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      stage  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
      stage  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      if (!isLayoutReadOnly(layout)) {
        access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      }
      break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      access = VK_ACCESS_SHADER_READ_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
      access = VK_ACCESS_TRANSFER_READ_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
      access = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
      stage  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      access = VK_ACCESS_MEMORY_READ_BIT;
      break;
    default: throw std::invalid_argument("Unsupported image layout!");
  }
}
} // namespace

NAMESPACE_BEGIN(Vulk)

Image::~Image() noexcept(false) {
  if (isAllocated()) {
    free();
  }
}

Image::Image(Image&& rhs) noexcept {
  moveFrom(rhs);
}

Image& Image::operator=(Image&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void Image::moveFrom(Image& rhs) {
  MI_VERIFY(!isCreated());
  _image  = rhs._image;
  _memory = rhs._memory;
  _format = rhs._format;
  _extent = rhs._extent;
  _layout = rhs._layout;
  _device = rhs._device;

  rhs._image  = VK_NULL_HANDLE;
  rhs._memory = VK_NULL_HANDLE;
  rhs._format = VK_FORMAT_UNDEFINED;
  rhs._extent = {0, 0, 0};
  rhs._layout = VK_IMAGE_LAYOUT_UNDEFINED;
  rhs._device = nullptr;
}

void Image::create(const Device& device, const VkImageCreateInfo& imageInfo) {
  MI_VERIFY(!isCreated());
  _device = &device;

  MI_VERIFY_VKCMD(vkCreateImage(device, &imageInfo, nullptr, &_image));

  _type   = imageInfo.imageType;
  _format = imageInfo.format;
  _extent = imageInfo.extent;
  _layout = imageInfo.initialLayout;
}

void Image::destroy() {
  MI_VERIFY(isCreated());

  if (isAllocated()) {
    free();
  }
  vkDestroyImage(*_device, _image, nullptr);

  _image  = VK_NULL_HANDLE;
  _format = VK_FORMAT_UNDEFINED;
  _extent = {0, 0, 0};
  _layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _device = nullptr;
}

void Image::allocate(VkMemoryPropertyFlags properties) {
  MI_VERIFY(!isAllocated());
  _memory = DeviceMemory::make();

  VkMemoryRequirements requirements;
  vkGetImageMemoryRequirements(*_device, _image, &requirements);
  _memory->allocate(*_device, properties, requirements);

  vkBindImageMemory(*_device, _image, *_memory.get(), 0);
}

void Image::free() {
  MI_VERIFY(isAllocated());
  _memory = nullptr;
}

void Image::bind(const DeviceMemory::Ptr& memory, VkDeviceSize offset) {
  MI_VERIFY(isCreated());
  MI_VERIFY(memory != _memory);
  if (isAllocated()) {
    free();
  }
  _memory = memory;
  vkBindImageMemory(*_device, _image, *_memory.get(), offset);
}

void* Image::map() {
  MI_VERIFY(isAllocated());
  return _memory->map();
}

void* Image::map(VkDeviceSize offset, VkDeviceSize size) {
  MI_VERIFY(isAllocated());
  return _memory->map(offset, size);
}

void Image::unmap() {
  MI_VERIFY(isAllocated());
  _memory->unmap();
}

void Image::copyFrom(const CommandBuffer& cmdBuffer, const StagingBuffer& stagingBuffer) {
  transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  stagingBuffer.copyToImage(cmdBuffer, *this, width(), height());
}

// copy the image data from `srcImage` to this image
void Image::copyFrom(const CommandBuffer& cmdBuffer, const Image& srcImage) {
  // TODO should we execute transit and copy in one command buffer execution?

  auto& dstImage = *this;
  MI_VERIFY(srcImage.extent() == dstImage.extent());

  dstImage.transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  auto prevSrcLayout = srcImage._layout;
  srcImage.transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  cmdBuffer.executeSingleTimeCommand([&srcImage, &dstImage](const CommandBuffer& cmdBuffer) {
    const auto& srcLayout = srcImage._layout;
    const auto& dstLayout = dstImage._layout;

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource.aspectMask     = selectAspectMask(srcLayout);
    copyRegion.srcSubresource.mipLevel       = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount     = 1;

    copyRegion.dstSubresource.aspectMask     = selectAspectMask(dstLayout);
    copyRegion.dstSubresource.mipLevel       = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount     = 1;

    copyRegion.srcOffset = {0, 0, 0};
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent    = {srcImage.width(), srcImage.height(), srcImage.depth()};

    vkCmdCopyImage(cmdBuffer, srcImage, srcLayout, dstImage, dstLayout, 1, &copyRegion);
  });

  cmdBuffer.waitIdle();

  srcImage.transitToNewLayout(cmdBuffer, prevSrcLayout);
}

// blit the image data from `srcImage` to this image
void Image::blitFrom(const CommandBuffer& cmdBuffer, const Image& srcImage) {
  // TODO should we execute transit and blit in one command buffer execution?

  auto& dstImage = *this;

  dstImage.transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  auto prevSrcLayout = srcImage._layout;
  srcImage.transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  cmdBuffer.executeSingleTimeCommand([&srcImage, &dstImage](const CommandBuffer& cmdBuffer) {
    const auto& srcLayout = srcImage._layout;
    const auto& dstLayout = dstImage._layout;

    const int32_t srcW = static_cast<int32_t>(srcImage.width());
    const int32_t srcH = static_cast<int32_t>(srcImage.height());
    const int32_t srcD = static_cast<int32_t>(srcImage.depth());

    const int32_t dstW = static_cast<int32_t>(dstImage.width());
    const int32_t dstH = static_cast<int32_t>(dstImage.height());
    const int32_t dstD = static_cast<int32_t>(dstImage.depth());

    VkImageBlit blit{};
    blit.srcSubresource.aspectMask = selectAspectMask(srcLayout);
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[0]             = {0, 0, 0};
    blit.srcOffsets[1]             = {srcW, srcH, srcD};

    blit.dstSubresource.aspectMask = selectAspectMask(dstLayout);
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[0]             = {0, 0, 0};
    blit.dstOffsets[1]             = {dstW, dstH, dstD};

    vkCmdBlitImage(cmdBuffer, srcImage, srcLayout, dstImage, dstLayout, 1, &blit, VK_FILTER_LINEAR);
  });

  cmdBuffer.waitIdle();

  srcImage.transitToNewLayout(cmdBuffer, prevSrcLayout);
}

void Image::transitToNewLayout(const CommandBuffer& commandBuffer,
                               VkImageLayout newLayout,
                               bool waitForFinish) const {
  if (_layout == newLayout) {
    return;
  }

  commandBuffer.executeSingleTimeCommand([this, newLayout](const CommandBuffer& buffer) {
    auto oldLayout = _layout;

    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = *this;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.aspectMask     = selectAspectMask(newLayout);

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_NONE;
    selectStageAccess(oldLayout, srcStage, barrier.srcAccessMask);
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_NONE;
    selectStageAccess(newLayout, dstStage, barrier.dstAccessMask);

    vkCmdPipelineBarrier(buffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  });

  if (waitForFinish) {
    commandBuffer.waitIdle();
  }

  // TODO there could be sync issue here. We should update the layout after the command buffer is
  // executed.
  _layout = newLayout;
}

VkImageViewType Image::imageViewType() const {
  switch (_type) {
    case VK_IMAGE_TYPE_1D: return VK_IMAGE_VIEW_TYPE_1D;
    case VK_IMAGE_TYPE_2D: return VK_IMAGE_VIEW_TYPE_2D;
    case VK_IMAGE_TYPE_3D: return VK_IMAGE_VIEW_TYPE_3D;
    default: MI_ASSERT(!"Invalid image type (VkImageType)"); return VK_IMAGE_VIEW_TYPE_2D;
  }
}

void Image::makeShaderReadable(const CommandBuffer& cmdBuffer) const {
  transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

NAMESPACE_END(Vulk)
