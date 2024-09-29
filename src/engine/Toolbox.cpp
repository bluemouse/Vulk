#include <Vulk/engine/Toolbox.h>

#include <Vulk/CommandBuffer.h>
#include <Vulk/CommandPool.h>
#include <Vulk/Device.h>
#include <Vulk/StagingBuffer.h>
#include <Vulk/internal/debug.h>

#include <Vulk/engine/Context.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

MI_NAMESPACE_BEGIN(Vulk)

Toolbox::Toolbox(const Context& context) : _context(context) {
}

Image2D::shared_ptr Toolbox::createImage2D(const char* imageFile) const {
  auto [stagingBuffer, width, height] = createStagingBuffer(imageFile);

  auto image = Image2D::make_shared(_context.device(),
                                    VK_FORMAT_R8G8B8A8_SRGB,
                                    VkExtent2D{width, height},
                                    Image2D::Usage::TRANSFER_DST);

  image->allocate();
  CommandBuffer cmdBuffer{_context.commandPool(Device::QueueFamilyType::Graphics)};
  const auto& queue = _context.queue(Device::QueueFamilyType::Graphics);
  image->copyFrom(queue, cmdBuffer, *stagingBuffer);
  image->makeShaderReadable(queue, cmdBuffer);

  return image;
}

Texture2D::shared_ptr Toolbox::createTexture2D(const char* textureFile) const {
  auto [stagingBuffer, width, height] = createStagingBuffer(textureFile);

  auto texture = Texture2D::make_shared(_context.device(),
                                        VK_FORMAT_R8G8B8A8_SRGB,
                                        VkExtent2D{width, height},
                                        Image2D::Usage::TRANSFER_DST);

  CommandBuffer cmdBuffer{_context.commandPool(Device::QueueFamilyType::Graphics)};
  const auto& queue = _context.queue(Device::QueueFamilyType::Graphics);
  texture->copyFrom(queue, cmdBuffer, *stagingBuffer);

  return texture;
}

Texture2D::shared_ptr Toolbox::createTexture2D(TextureFormat format,
                                               const uint8_t* data,
                                               uint32_t width,
                                               uint32_t height) const {
  uint32_t size      = width * height * (format == TextureFormat::RGBA ? 4 : 3);
  auto stagingBuffer = createStagingBuffer(data, size);

  const auto vkFormat =
      format == TextureFormat::RGBA ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8_SRGB;
  auto texture = Texture2D::make_shared(
      _context.device(), vkFormat, VkExtent2D{width, height}, Image2D::Usage::TRANSFER_DST);

  CommandBuffer cmdBuffer{_context.commandPool(Device::QueueFamilyType::Graphics)};
  const auto& queue = _context.queue(Device::QueueFamilyType::Graphics);
  texture->copyFrom(queue, cmdBuffer, *stagingBuffer);

  return texture;
}

auto Toolbox::createStagingBuffer(const char* imageFile) const
    -> std::tuple<StagingBuffer::shared_ptr, width_t, height_t> {
  int texWidth    = 0;
  int texHeight   = 0;
  int texChannels = 0;

  stbi_uc* pixels = stbi_load(imageFile, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  MI_VERIFY(pixels != nullptr);

  auto imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * 4);

  auto stagingBuffer = StagingBuffer::make_shared(_context.device(), imageSize, pixels);

  stbi_image_free(pixels);

  return {stagingBuffer, texWidth, texHeight};
}

StagingBuffer::shared_ptr Toolbox::createStagingBuffer(const uint8_t* data, uint32_t size) const {
  return StagingBuffer::make_shared(_context.device(), size, data);
}

MI_NAMESPACE_END(Vulk)
