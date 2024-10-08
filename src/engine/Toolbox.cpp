#include <Vulk/engine/Toolbox.h>

#include <thread>

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

namespace {
void releaseOnDone(CommandBuffer::shared_ptr commandBuffer,
                   Fence::shared_ptr fence,
                   StagingBuffer::shared_ptr stagingBuffer) {
  // The commandBuffer now should be in the pending state
  MI_ASSERT(commandBuffer->state() == CommandBuffer::State::Pending);
  // Start a new thread to wait for the fence to be signaled and then release the staging buffer
  auto releaser = [](CommandBuffer::shared_ptr commandBuffer,
                     Fence::shared_ptr fence,
                     StagingBuffer::shared_ptr stagingBuffer) {
    fence->wait();

    stagingBuffer.reset();
    fence.reset();
    commandBuffer.reset();
  };
  std::thread{releaser, commandBuffer, fence, stagingBuffer}.detach();
}
} // namespace

Image2D::shared_ptr Toolbox::createImage2D(const char* imageFile) const {
  const auto& device = _context.device();

  auto [stagingBuffer, width, height] = createStagingBuffer(imageFile);
  auto image                          = Image2D::make_shared(
      device, VK_FORMAT_R8G8B8A8_SRGB, VkExtent2D{width, height}, Image2D::Usage::TRANSFER_DST);

  image->allocate();

  const auto& commandPool                 = device.commandPool(Device::QueueFamilyType::Transfer);
  CommandBuffer::shared_ptr commandBuffer = CommandBuffer::make_shared(commandPool);
  Fence::shared_ptr fence                 = Fence::make_shared(device);

  commandBuffer->beginRecording(CommandBuffer::Usage::OneTimeSubmit);
  {
    image->copyFrom(*commandBuffer, *stagingBuffer);
    image->transitToNewLayout(*commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
  commandBuffer->endRecording();
  commandBuffer->submitCommands(*fence);

  releaseOnDone(commandBuffer, fence, stagingBuffer);

  return image;
}

Texture2D::shared_ptr Toolbox::createTexture2D(const char* textureFile) const {
  const auto& device = _context.device();

  auto [stagingBuffer, width, height] = createStagingBuffer(textureFile);
  auto texture                        = Texture2D::make_shared(_context.device(),
                                        VK_FORMAT_R8G8B8A8_SRGB,
                                        VkExtent2D{width, height},
                                        Image2D::Usage::TRANSFER_DST);

  const auto& commandPool                 = device.commandPool(Device::QueueFamilyType::Transfer);
  CommandBuffer::shared_ptr commandBuffer = CommandBuffer::make_shared(commandPool);
  Fence::shared_ptr fence                 = Fence::make_shared(device);

  texture->copyFrom(*commandBuffer, *stagingBuffer, *fence);

  releaseOnDone(commandBuffer, fence, stagingBuffer);

  return texture;
}

Texture2D::shared_ptr Toolbox::createTexture2D(TextureFormat format,
                                               const uint8_t* data,
                                               uint32_t width,
                                               uint32_t height) const {
  const auto& device = _context.device();

  const uint32_t size = width * height * (format == TextureFormat::RGBA ? 4 : 3);
  auto stagingBuffer  = createStagingBuffer(data, size);

  const auto vkFormat =
      format == TextureFormat::RGBA ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8_SRGB;
  auto texture = Texture2D::make_shared(
      _context.device(), vkFormat, VkExtent2D{width, height}, Image2D::Usage::TRANSFER_DST);

  const auto& commandPool                 = device.commandPool(Device::QueueFamilyType::Transfer);
  CommandBuffer::shared_ptr commandBuffer = CommandBuffer::make_shared(commandPool);
  Fence::shared_ptr fence                 = Fence::make_shared(device);

  texture->copyFrom(*commandBuffer, *stagingBuffer, *fence);

  releaseOnDone(commandBuffer, fence, stagingBuffer);

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
