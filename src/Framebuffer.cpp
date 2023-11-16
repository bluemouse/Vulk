#include <Vulk/Framebuffer.h>

#include <Vulk/Device.h>
#include <Vulk/Image.h>
#include <Vulk/ImageView.h>
#include <Vulk/RenderPass.h>

NAMESPACE_BEGIN(Vulk)

Framebuffer::Framebuffer(const Device& device,
                         const RenderPass& renderPass,
                         const ImageView& colorAttachment) {
  create(device, renderPass, colorAttachment);
}

Framebuffer::Framebuffer(const Device& device,
                         const RenderPass& renderPass,
                         const ImageView& colorAttachment,
                         const ImageView& depthStencilAttachment) {
  create(device, renderPass, colorAttachment, depthStencilAttachment);
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
                         const ImageView& colorAttachment) {
  _device                 = &device;
  _renderPass             = &renderPass;
  _colorAttachment        = &colorAttachment;
  _depthStencilAttachment = nullptr;

  create();
}

void Framebuffer::create(const Device& device,
                         const RenderPass& renderPass,
                         const ImageView& colorAttachment,
                         const ImageView& depthStencilAttachment) {
  _device                 = &device;
  _renderPass             = &renderPass;
  _colorAttachment        = &colorAttachment;
  _depthStencilAttachment = &depthStencilAttachment;

  create();
}

void Framebuffer::create() {
  MI_VERIFY(!isCreated());

  std::vector<VkImageView> attachments = {*_colorAttachment};
  if (_depthStencilAttachment != nullptr) {
    attachments.push_back(*_depthStencilAttachment);
  }

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass      = *_renderPass;
  framebufferInfo.attachmentCount = attachments.size();
  framebufferInfo.pAttachments    = attachments.data();
  framebufferInfo.width           = _colorAttachment->image().width();
  framebufferInfo.height          = _colorAttachment->image().height();
  framebufferInfo.layers          = 1;

  MI_VERIFY_VKCMD(vkCreateFramebuffer(*_device, &framebufferInfo, nullptr, &_buffer));
}

void Framebuffer::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyFramebuffer(*_device, _buffer, nullptr);

  _buffer                 = VK_NULL_HANDLE;
  _device                 = nullptr;
  _renderPass             = nullptr;
  _colorAttachment        = nullptr;
  _depthStencilAttachment = nullptr;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

VkExtent2D Framebuffer::extent() const {
  auto imageExtent = _colorAttachment->image().extent();
  return {imageExtent.width, imageExtent.height};
}

void Framebuffer::moveFrom(Framebuffer& rhs) {
  MI_VERIFY(_buffer == VK_NULL_HANDLE);
  _buffer                 = rhs._buffer;
  _device                 = rhs._device;
  _renderPass             = rhs._renderPass;
  _colorAttachment        = rhs._colorAttachment;
  _depthStencilAttachment = rhs._depthStencilAttachment;

  rhs._buffer                 = VK_NULL_HANDLE;
  rhs._device                 = nullptr;
  rhs._renderPass             = nullptr;
  rhs._colorAttachment        = nullptr;
  rhs._depthStencilAttachment = nullptr;
}

NAMESPACE_END(Vulk)
