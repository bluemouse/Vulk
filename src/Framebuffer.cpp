#include <Vulk/Framebuffer.h>

#include <Vulk/internal/debug.h>

#include <Vulk/Device.h>
#include <Vulk/Image.h>
#include <Vulk/ImageView.h>
#include <Vulk/RenderPass.h>

MI_NAMESPACE_BEGIN(Vulk)

Framebuffer::Framebuffer(const Device& device,
                         const RenderPass::shared_ptr& renderPass,
                         const ImageView::shared_ptr& colorAttachment,
                         const ImageView::shared_ptr& depthStencilAttachment) {
  create(device, renderPass, colorAttachment, depthStencilAttachment);
}

Framebuffer::~Framebuffer() {
  if (isCreated()) {
    destroy();
  }
}

void Framebuffer::create(const Device& device,
                         const RenderPass::shared_ptr& renderPass,
                         const ImageView::shared_ptr& colorAttachment,
                         const ImageView::shared_ptr& depthStencilAttachment) {
  _device = device.get_weak();

  _renderPass             = renderPass;
  _colorAttachment        = colorAttachment;
  _depthStencilAttachment = depthStencilAttachment;

  create();
}

void Framebuffer::create() {
  MI_VERIFY(!isCreated());

  std::vector<VkImageView> attachments = {colorAttachment()};
  if (_depthStencilAttachment) {
    attachments.push_back(depthStencilAttachment());
  }

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  // TODO use RenderPass to assert the matching of the attachments
  framebufferInfo.renderPass      = renderPass();
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
  return _colorAttachment->image();
}

MI_NAMESPACE_END(Vulk)
