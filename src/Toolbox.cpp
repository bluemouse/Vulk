#include <Vulk/Toolbox.h>

#include <Vulk/CommandBuffer.h>
#include <Vulk/CommandPool.h>
#include <Vulk/Context.h>
#include <Vulk/Device.h>
#include <Vulk/StagingBuffer.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

NAMESPACE_Vulk_BEGIN

Toolbox::Toolbox(const Context& context)
    : _context(context) {
}

Image Toolbox::createImage(const char* imageFile) const {
  int texWidth = 0;
  int texHeight = 0;
  int texChannels = 0;

  stbi_uc* pixels = stbi_load(imageFile, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  MI_VERIFY(pixels != nullptr);

  auto imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * 4);

  StagingBuffer stagingBuffer(_context.device(), imageSize);
  stagingBuffer.copyFromHost(pixels, imageSize);

  stbi_image_free(pixels);

  Image image;
  image.create(_context.device(),
                       VK_FORMAT_R8G8B8A8_SRGB,
                       {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)});
  image.allocate();

  CommandBuffer cmdBuffer{_context.commandPool()};
  image.transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  stagingBuffer.copyToImage(
      cmdBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  image.transitToNewLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  return image;
}

NAMESPACE_Vulk_END
