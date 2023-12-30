#include <Vulk/engine/Toolbox.h>

#include <Vulk/CommandBuffer.h>
#include <Vulk/CommandPool.h>
#include <Vulk/engine/Context.h>
#include <Vulk/Device.h>
#include <Vulk/StagingBuffer.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

NAMESPACE_BEGIN(Vulk)

Toolbox::Toolbox(const Context& context) : _context(context) {
}

Image2D::shared_ptr Toolbox::createImage2D(const char* imageFile) const {
  auto [stagingBuffer, width, height] = createStagingBuffer(imageFile);

  const auto usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  auto image       = Image2D::make_shared(
      _context.device(), VK_FORMAT_R8G8B8A8_SRGB, VkExtent2D{width, height}, usage);

  image->allocate();
  CommandBuffer cmdBuffer{_context.commandPool()};
  image->copyFrom(cmdBuffer, stagingBuffer);
  image->makeShaderReadable(cmdBuffer);

  return image;
}

Texture2D::shared_ptr Toolbox::createTexture2D(const char* textureFile) const {
  auto [stagingBuffer, width, height] = createStagingBuffer(textureFile);

  const auto usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  auto texture     = Texture2D::make_shared(
      _context.device(), VK_FORMAT_R8G8B8A8_SRGB, VkExtent2D{width, height}, usage);
  texture->copyFrom(CommandBuffer{_context.commandPool()}, stagingBuffer);

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
  const auto usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  auto texture =
      Texture2D::make_shared(_context.device(), vkFormat, VkExtent2D{width, height}, usage);
  texture->copyFrom(CommandBuffer{_context.commandPool()}, stagingBuffer);

  return texture;
}

auto Toolbox::createStagingBuffer(const char* imageFile) const
    -> std::tuple<StagingBuffer, width_t, height_t> {
  int texWidth    = 0;
  int texHeight   = 0;
  int texChannels = 0;

  stbi_uc* pixels = stbi_load(imageFile, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  MI_VERIFY(pixels != nullptr);

  auto imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * 4);

  StagingBuffer stagingBuffer{_context.device(), imageSize, pixels};

  stbi_image_free(pixels);

  return {std::move(stagingBuffer), texWidth, texHeight};
}

StagingBuffer Toolbox::createStagingBuffer(const uint8_t* data, uint32_t size) const {
  return StagingBuffer{_context.device(), size, data};
}

NAMESPACE_END(Vulk)
