#include <Vulk/Framebuffer.h>

#include <Vulk/Device.h>
#include <Vulk/Image.h>
#include <Vulk/ImageView.h>
#include <Vulk/RenderPass.h>

NAMESPACE_Vulk_BEGIN

Framebuffer::Framebuffer(const Device& device,
                         const RenderPass& renderPass,
                         const ImageView& imageView) {
  create(device, renderPass, imageView);
}

Framebuffer::~Framebuffer() {
  if (isCreated()) {
    destroy();
  }
}

Framebuffer::Framebuffer(Framebuffer&& rhs) noexcept {
  moveFrom(rhs);
}

void Framebuffer::create(const Device& device,
                         const RenderPass& renderPass,
                         const ImageView& imageView) {
  MI_VERIFY(!isCreated());
  _device = &device;
  _renderPass = &renderPass;
  _imageView = &imageView;

  VkImageView attachments[] = {imageView};

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = 1;
  framebufferInfo.pAttachments = attachments;
  framebufferInfo.width = imageView.image().width();
  framebufferInfo.height = imageView.image().height();
  framebufferInfo.layers = 1;

  MI_VERIFY_VKCMD(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &_buffer));
}

void Framebuffer::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyFramebuffer(*_device, _buffer, nullptr);

  _buffer = VK_NULL_HANDLE;
  _device = nullptr;
  _renderPass = nullptr;
  _imageView = nullptr;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

VkExtent2D Framebuffer::extent() const {
  auto imageExtent = _imageView->image().extent();
  return {imageExtent.width, imageExtent.height};
}

void Framebuffer::moveFrom(Framebuffer& rhs) {
  MI_VERIFY(_buffer == VK_NULL_HANDLE);
  _buffer = rhs._buffer;
  _device = rhs._device;
  _renderPass = rhs._renderPass;
  _imageView = rhs._imageView;

  rhs._buffer = VK_NULL_HANDLE;
  rhs._device = nullptr;
  rhs._renderPass = nullptr;
  rhs._imageView = nullptr;
}

NAMESPACE_Vulk_END
