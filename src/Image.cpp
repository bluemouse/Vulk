#include <Vulk/Image.h>

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/StagingBuffer.h>

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
  transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Image::transitToNewLayout(const CommandBuffer& commandBuffer,
                               VkImageLayout newLayout,
                               bool waitForFinish) const {
  commandBuffer.executeSingleTimeCommand([this, newLayout](const CommandBuffer& buffer) {
    VkImageMemoryBarrier barrier{};

    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = _layout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = *this;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_NONE_KHR;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags srcStage = 0;
    VkPipelineStageFlags dstStage = 0;

    const VkImageLayout depthLayouts[] = {VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
    // VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
    // VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
    // VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    // VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL
    for (auto layout : depthLayouts) {
      if (layout == newLayout) {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
      }
    }
    const VkImageLayout stencilLayouts[] = {VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                            VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL};
    // VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
    // VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
    // VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    // VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL
    for (auto layout : stencilLayouts) {
      if (layout == newLayout) {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
      }
    }

    const bool isNewLayoutDepthStencil =
        barrier.subresourceRange.aspectMask != VK_IMAGE_ASPECT_NONE_KHR;

    if (!isNewLayoutDepthStencil) {
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (_layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_NONE;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (_layout == VK_IMAGE_LAYOUT_UNDEFINED && isNewLayoutDepthStencil) {
      barrier.srcAccessMask = VK_ACCESS_NONE;
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      // TODO This is probably not correct for ready-only depth and stencil layout

      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
      throw std::invalid_argument("Unsupported layout transition!");
    }

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

NAMESPACE_END(Vulk)
