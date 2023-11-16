#include <Vulk/Toolbox.h>

#include <Vulk/CommandBuffer.h>
#include <Vulk/CommandPool.h>
#include <Vulk/Context.h>
#include <Vulk/Device.h>
#include <Vulk/StagingBuffer.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

NAMESPACE_BEGIN(Vulk)

Toolbox::Toolbox(const Context& context) : _context(context) {
}

Image2D Toolbox::createImage2D(const char* imageFile) const {
  auto [stagingBuffer, width, height] = createStagingBuffer(imageFile);

  Image2D image{_context.device(), VK_FORMAT_R8G8B8A8_SRGB, {width, height}};
  image.allocate();
  image.copyFrom(CommandBuffer{_context.commandPool()}, stagingBuffer);

  return image;
}

Texture2D Toolbox::createTexture(const char* textureFile) const {
  auto [stagingBuffer, width, height] = createStagingBuffer(textureFile);

  Texture2D texture{_context.device(), VK_FORMAT_R8G8B8A8_SRGB, {width, height}};
  texture.copyFrom(CommandBuffer{_context.commandPool()}, stagingBuffer);

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

  StagingBuffer stagingBuffer(_context.device(), imageSize);
  stagingBuffer.copyFromHost(pixels, imageSize);

  stbi_image_free(pixels);

  return {std::move(stagingBuffer), texWidth, texHeight};
}

NAMESPACE_END(Vulk)
