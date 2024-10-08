#pragma once

#include <volk/volk.h>

#include <memory>

#include <Vulk/internal/base.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;
class RenderPass;
class ImageView;
class Image;

class Framebuffer : public Sharable<Framebuffer>, private NotCopyable {
 public:
  Framebuffer(const Device& device, const RenderPass& renderPass, const ImageView& colorAttachment);
  Framebuffer(const Device& device,
              const RenderPass& renderPass,
              const ImageView& colorAttachment,
              const ImageView& depthStencilAttachment);
  ~Framebuffer();

  void create(const Device& device, const RenderPass& renderPass, const ImageView& colorAttachment);
  void create(const Device& device,
              const RenderPass& renderPass,
              const ImageView& colorAttachment,
              const ImageView& depthStencilAttachment);
  void destroy();

  operator VkFramebuffer() const { return _buffer; }

  [[nodiscard]] VkExtent2D extent() const;

  [[nodiscard]] bool isCreated() const { return _buffer != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }
  [[nodiscard]] const RenderPass& renderPass() const { return *_renderPass.lock(); }

  [[nodiscard]] const Image& colorBuffer() const;

 private:
  void create();

  [[nodiscard]] const ImageView& colorAttachment() const { return *_colorAttachment.lock(); }
  [[nodiscard]] const ImageView& depthStencilAttachment() const {
    return *_depthStencilAttachment.lock();
  }

 private:
  VkFramebuffer _buffer = VK_NULL_HANDLE;

  std::weak_ptr<const Device> _device;
  std::weak_ptr<const RenderPass> _renderPass;
  std::weak_ptr<const ImageView> _colorAttachment;
  std::weak_ptr<const ImageView> _depthStencilAttachment;
};

MI_NAMESPACE_END(Vulk)