#include <Vulk/Image.h>

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>

NAMESPACE_Vulk_BEGIN

Image::Image(const Device& device,
             VkFormat format,
             VkExtent2D extent,
             const ImageCreateInfoOverride& override) {
  create(device, format, extent, override);
}

Image::Image(const Device& device,
             VkFormat format,
             VkExtent2D extent,
             VkMemoryPropertyFlags properties,
             const ImageCreateInfoOverride& override)
    : Image(device, format, extent, override) {
  allocate(properties);
}

Image::~Image() noexcept(false) {
  if (isAllocated()) {
    free();
  }
}

Image::Image(VkImage image, VkFormat format, VkExtent2D extent)
    : _image(image), _format(format), _extent{extent.width, extent.height, 1}, _external(true) {
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
  _image = rhs._image;
  _memory = rhs._memory;
  _format = rhs._format;
  _extent = rhs._extent;
  _layout = rhs._layout;
  _device = rhs._device;
  _external = rhs._external;

  rhs._image = VK_NULL_HANDLE;
  rhs._memory = VK_NULL_HANDLE;
  rhs._format = VK_FORMAT_UNDEFINED;
  rhs._extent = {0, 0, 0};
  rhs._layout = VK_IMAGE_LAYOUT_UNDEFINED;
  rhs._device = nullptr;
  rhs._external = false;
}

void Image::create(const Device& device,
                   VkFormat format,
                   VkExtent2D extent,
                   const ImageCreateInfoOverride& override) {
  MI_VERIFY(!isExternal());
  MI_VERIFY(!isCreated());
  _device = &device;

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D; // Implied by extent
  imageInfo.extent.width = extent.width;
  imageInfo.extent.height = extent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  // The Vulkan spec states: initialLayout must be VK_IMAGE_LAYOUT_UNDEFINED or
  // VK_IMAGE_LAYOUT_PREINITIALIZED
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (override) {
    override(&imageInfo);
  }

  MI_VERIFY_VKCMD(vkCreateImage(device, &imageInfo, nullptr, &_image));

  _type = imageInfo.imageType;
  _format = imageInfo.format;
  _extent = imageInfo.extent;
  _layout = imageInfo.initialLayout;
}

void Image::destroy() {
  MI_VERIFY(!isExternal());
  MI_VERIFY(isCreated());

  if (isAllocated()) {
    free();
  }
  vkDestroyImage(*_device, _image, nullptr);

  _image = VK_NULL_HANDLE;
  _format = VK_FORMAT_UNDEFINED;
  _extent = {0, 0, 0};
  _layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _device = nullptr;
}

void Image::allocate(VkMemoryPropertyFlags properties) {
  MI_VERIFY(!isExternal());
  MI_VERIFY(!isAllocated());
  _memory = DeviceMemory::make();

  VkMemoryRequirements requirements;
  vkGetImageMemoryRequirements(*_device, _image, &requirements);
  _memory->allocate(*_device, properties, requirements);

  vkBindImageMemory(*_device, _image, *_memory.get(), 0);
}

void Image::free() {
  MI_VERIFY(!isExternal());
  MI_VERIFY(isAllocated());
  _memory = nullptr;
}

void Image::bind(const DeviceMemory::Ptr& memory, VkDeviceSize offset) {
  MI_VERIFY(!isExternal());
  MI_VERIFY(isCreated());
  MI_VERIFY(memory != _memory);
  if (isAllocated()) {
    free();
  }
  _memory = memory;
  vkBindImageMemory(*_device, _image, *_memory.get(), offset);
}

void* Image::map() {
  MI_VERIFY(!isExternal());
  MI_VERIFY(isAllocated());
  return _memory->map();
}

void* Image::map(VkDeviceSize offset, VkDeviceSize size) {
  MI_VERIFY(!isExternal());
  MI_VERIFY(isAllocated());
  return _memory->map(offset, size);
}

void Image::unmap() {
  MI_VERIFY(!isExternal());
  MI_VERIFY(isAllocated());
  _memory->unmap();
}

void Image::transitToNewLayout(const CommandBuffer& commandBuffer,
                               VkImageLayout newLayout,
                               bool waitForFinish) {
  commandBuffer.executeSingleTimeCommand([this, newLayout](const CommandBuffer& buffer) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = _layout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = *this;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage = 0;
    VkPipelineStageFlags dstStage = 0;

    if (_layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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
    case VK_IMAGE_TYPE_1D:
      return VK_IMAGE_VIEW_TYPE_1D;
    case VK_IMAGE_TYPE_2D:
      return VK_IMAGE_VIEW_TYPE_2D;
    case VK_IMAGE_TYPE_3D:
      return VK_IMAGE_VIEW_TYPE_3D;
    default:
      MI_ASSERT(!"Invalid image type (VkImageType)");
      return VK_IMAGE_VIEW_TYPE_2D;
  }
}

NAMESPACE_Vulk_END
