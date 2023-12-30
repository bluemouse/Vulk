#include <Vulk/Framebuffer.h>

#include <Vulk/internal/vulkan_debug.h>

#include <Vulk/Device.h>
#include <Vulk/Image.h>
#include <Vulk/ImageView.h>
#include <Vulk/RenderPass.h>

NAMESPACE_BEGIN(Vulk)

Framebuffer::Framebuffer(const Device& device,
                         const RenderPass& renderPass,
                         ImageView& colorAttachment) {
  create(device, renderPass, colorAttachment);
}

Framebuffer::Framebuffer(const Device& device,
                         const RenderPass& renderPass,
                         ImageView& colorAttachment,
                         ImageView& depthStencilAttachment) {
  create(device, renderPass, colorAttachment, depthStencilAttachment);
}

Framebuffer::~Framebuffer() {
  if (isCreated()) {
    destroy();
  }
}

void Framebuffer::create(const Device& device,
                         const RenderPass& renderPass,
                         ImageView& colorAttachment) {
  _device          = device.get_weak();
  _renderPass      = renderPass.get_weak();
  _colorAttachment = colorAttachment.get_weak();
  _depthStencilAttachment.reset();

  create();
}

void Framebuffer::create(const Device& device,
                         const RenderPass& renderPass,
                         ImageView& colorAttachment,
                         ImageView& depthStencilAttachment) {
  _device                 = device.get_weak();
  _renderPass             = renderPass.get_weak();
  _colorAttachment        = colorAttachment.get_weak();
  _depthStencilAttachment = depthStencilAttachment.get_weak();

  create();
}

void Framebuffer::create() {
  MI_VERIFY(!isCreated());

  std::vector<VkImageView> attachments = {colorAttachment()};
  if (_depthStencilAttachment.lock()) {
    attachments.push_back(depthStencilAttachment());
  }

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass      = renderPass();
  framebufferInfo.attachmentCount = attachments.size();
  framebufferInfo.pAttachments    = attachments.data();
  framebufferInfo.width           = image().width();
  framebufferInfo.height          = image().height();
  framebufferInfo.layers          = 1;

  MI_VERIFY_VKCMD(vkCreateFramebuffer(device(), &framebufferInfo, nullptr, &_buffer));
}

void Framebuffer::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyFramebuffer(device(), _buffer, nullptr);

  _buffer                 = VK_NULL_HANDLE;

  _device.reset();
  _renderPass.reset();
  _colorAttachment.reset();
  _depthStencilAttachment.reset();
}

VkExtent2D Framebuffer::extent() const {
  auto imageExtent = image().extent();
  return {imageExtent.width, imageExtent.height};
}

Image& Framebuffer::image() {
  return _colorAttachment.lock()->image();
}

const Image& Framebuffer::image() const {
  return _colorAttachment.lock()->image();
}

NAMESPACE_END(Vulk)
