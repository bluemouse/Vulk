#include <Vulk/Framebuffer.h>

#include <Vulk/internal/debug.h>

#include <Vulk/Device.h>
#include <Vulk/Image.h>
#include <Vulk/ImageView.h>
#include <Vulk/RenderPass.h>

MI_NAMESPACE_BEGIN(Vulk)

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

void Framebuffer::create(const Device& device,
                         const RenderPass& renderPass,
                         const ImageView& colorAttachment) {
  _device          = device.get_weak();
  _renderPass      = renderPass.get_weak();
  _colorAttachment = colorAttachment.get_weak();
  _depthStencilAttachment.reset();

  create();
}

void Framebuffer::create(const Device& device,
                         const RenderPass& renderPass,
                         const ImageView& colorAttachment,
                         const ImageView& depthStencilAttachment) {
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
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass =
      renderPass(); // TODO use RenderPass to assert the matching of the attachments
  framebufferInfo.attachmentCount = attachments.size();
  framebufferInfo.pAttachments    = attachments.data();
  framebufferInfo.width           = colorBuffer().width();
  framebufferInfo.height          = colorBuffer().height();
  framebufferInfo.layers          = 1; // TODO should be a parameter

  MI_VERIFY_VK_RESULT(vkCreateFramebuffer(device(), &framebufferInfo, nullptr, &_buffer));
}

void Framebuffer::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyFramebuffer(device(), _buffer, nullptr);

  _buffer = VK_NULL_HANDLE;

  _device.reset();
  _renderPass.reset();
  _colorAttachment.reset();
  _depthStencilAttachment.reset();
}

VkExtent2D Framebuffer::extent() const {
  auto imageExtent = colorBuffer().extent();
  return {imageExtent.width, imageExtent.height};
}

const Image& Framebuffer::colorBuffer() const {
  return _colorAttachment.lock()->image();
}

MI_NAMESPACE_END(Vulk)
