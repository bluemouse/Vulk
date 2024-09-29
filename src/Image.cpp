#include <Vulk/Image.h>

#include <set>

#include <Vulk/internal/debug.h>
#include <Vulk/internal/helpers.h>

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/Queue.h>
#include <Vulk/StagingBuffer.h>

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

MI_NAMESPACE_BEGIN(Vulk)

Image::~Image() {
  if (isCreated()) {
    destroy();
  }
}

void Image::create(const Device& device, const VkImageCreateInfo& imageInfo) {
  MI_VERIFY(!isCreated());
  _device = device.get_weak();

  MI_VERIFY_VK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &_image));

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
  vkDestroyImage(device(), _image, nullptr);

  _image  = VK_NULL_HANDLE;
  _format = VK_FORMAT_UNDEFINED;
  _extent = {0, 0, 0};
  _layout = VK_IMAGE_LAYOUT_UNDEFINED;
  _device.reset();
}

void Image::allocate(VkMemoryPropertyFlags properties) {
  MI_VERIFY(!isAllocated());

  const Device& device = this->device();

  VkMemoryRequirements requirements;
  vkGetImageMemoryRequirements(device, _image, &requirements);

  _memory = DeviceMemory::make_shared(device, properties, requirements);

  vkBindImageMemory(device, _image, *_memory, 0);
}

void Image::free() {
  MI_VERIFY(isAllocated());
  _memory = nullptr;
}

void Image::bind(DeviceMemory& memory, VkDeviceSize offset) {
  MI_VERIFY(isCreated());
  MI_VERIFY(&memory != _memory.get());
  if (isAllocated()) {
    free();
  }
  _memory = memory.get_shared();
  vkBindImageMemory(device(), _image, memory, offset);
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

void Image::copyFrom(const Queue& queue,
                     const CommandBuffer& commandBuffer,
                     const StagingBuffer& stagingBuffer) {
  transitToNewLayout(queue, commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  stagingBuffer.copyToImage(queue, commandBuffer, *this, width(), height());
}

// copy the image data from `srcImage` to this image
void Image::copyFrom(const Queue& queue,
                     const CommandBuffer& commandBuffer,
                     const Image& srcImage) {
  // TODO should we execute transit and copy in one command buffer execution?

  auto& dstImage = *this;
  MI_VERIFY(srcImage.extent() == dstImage.extent());

  dstImage.transitToNewLayout(queue, commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  auto prevSrcLayout = srcImage._layout;
  srcImage.transitToNewLayout(queue, commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  auto commands = [&srcImage, &dstImage](const CommandBuffer& commandBuffer) {
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

    vkCmdCopyImage(commandBuffer, srcImage, srcLayout, dstImage, dstLayout, 1, &copyRegion);
  };
  commandBuffer.recordCommands(commands, CommandBuffer::Usage::OneTimeSubmit);

  queue.submitCommands(commandBuffer);

  queue.waitIdle();

  srcImage.transitToNewLayout(queue, commandBuffer, prevSrcLayout);
}

// blit the image data from `srcImage` to this image
void Image::blitFrom(const Queue& queue,
                     const CommandBuffer& commandBuffer,
                     const Image& srcImage) {
  // TODO should we execute transit and blit in one command buffer execution?

  auto& dstImage = *this;

  dstImage.transitToNewLayout(queue, commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  auto prevSrcLayout = srcImage._layout;
  srcImage.transitToNewLayout(queue, commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  auto commands = [&srcImage, &dstImage](const CommandBuffer& commandBuffer) {
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

    vkCmdBlitImage(
        commandBuffer, srcImage, srcLayout, dstImage, dstLayout, 1, &blit, VK_FILTER_LINEAR);
  };
  commandBuffer.recordCommands(commands, CommandBuffer::Usage::OneTimeSubmit);

  queue.submitCommands(commandBuffer);

  queue.waitIdle();

  srcImage.transitToNewLayout(queue, commandBuffer, prevSrcLayout);
}

void Image::transitToNewLayout(const Queue& queue,
                               const CommandBuffer& commandBuffer,
                               VkImageLayout newLayout,
                               bool waitForFinish) const {
  if (_layout == newLayout) {
    return;
  }

  auto commands = [this, newLayout](const CommandBuffer& buffer) {
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
  };
  commandBuffer.recordCommands(commands, CommandBuffer::Usage::OneTimeSubmit);

  queue.submitCommands(commandBuffer);

  if (waitForFinish) {
    queue.waitIdle();
  }

  // TODO there could be sync issue here. We should update the layout after the command buffer is
  // executed.
  _layout = newLayout;
}

void Image::makeShaderReadable(const Queue& queue, const CommandBuffer& commandBuffer) const {
  transitToNewLayout(queue, commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

VkImageViewType Image::imageViewType() const {
  switch (_type) {
    case VK_IMAGE_TYPE_1D: return VK_IMAGE_VIEW_TYPE_1D;
    case VK_IMAGE_TYPE_2D: return VK_IMAGE_VIEW_TYPE_2D;
    case VK_IMAGE_TYPE_3D: return VK_IMAGE_VIEW_TYPE_3D;
    default: MI_ASSERT(!"Invalid image type (VkImageType)"); return VK_IMAGE_VIEW_TYPE_2D;
  }
}

MI_NAMESPACE_END(Vulk)
