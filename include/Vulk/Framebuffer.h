#pragma once

#include <volk/volk.h>

#include <memory>

#include <Vulk/internal/base.h>
#include <Vulk/ImageView.h>
#include <Vulk/RenderPass.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;
class Image;

class Framebuffer : public Sharable<Framebuffer>, private NotCopyable {
 public:
  Framebuffer(const Device& device,
              const RenderPass::shared_ptr& renderPass,
              const ImageView::shared_ptr& colorAttachment,
              const ImageView::shared_ptr& depthStencilAttachment = nullptr);
  ~Framebuffer();

  void create(const Device& device,
              const RenderPass::shared_ptr& renderPass,
              const ImageView::shared_ptr& colorAttachment,
              const ImageView::shared_ptr& depthStencilAttachment = nullptr);
  void destroy();

  operator VkFramebuffer() const { return _buffer; }

  [[nodiscard]] VkExtent2D extent() const;

  [[nodiscard]] bool isCreated() const { return _buffer != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }
  [[nodiscard]] const RenderPass& renderPass() const { return *_renderPass; }

  [[nodiscard]] const Image& colorBuffer() const;

 private:
  void create();

  [[nodiscard]] const ImageView& colorAttachment() const { return *_colorAttachment; }
  [[nodiscard]] const ImageView& depthStencilAttachment() const { return *_depthStencilAttachment; }

 private:
  VkFramebuffer _buffer = VK_NULL_HANDLE;

  std::weak_ptr<const Device> _device;

  RenderPass::shared_ptr _renderPass;

  ImageView::shared_ptr _colorAttachment;
  ImageView::shared_ptr _depthStencilAttachment;
};

MI_NAMESPACE_END(Vulk)