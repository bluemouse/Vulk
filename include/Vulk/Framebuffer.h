#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/helpers_vulkan.h>

NAMESPACE_BEGIN(Vulk)

class Device;
class RenderPass;
class ImageView;
class Image;

class Framebuffer {
 public:
  Framebuffer() = default;
  Framebuffer(const Device& device, const RenderPass& renderPass, ImageView& colorAttachment);
  Framebuffer(const Device& device,
              const RenderPass& renderPass,
              ImageView& colorAttachment,
              ImageView& depthStencilAttachment);
  ~Framebuffer();

  // Transfer the ownership from `rhs` to `this`
  Framebuffer(Framebuffer&& rhs) noexcept;
  Framebuffer& operator=(Framebuffer&& rhs) noexcept(false);

  void create(const Device& device, const RenderPass& renderPass, ImageView& colorAttachment);
  void create(const Device& device,
              const RenderPass& renderPass,
              ImageView& colorAttachment,
              ImageView& depthStencilAttachment);
  void destroy();

  operator VkFramebuffer() const { return _buffer; }

  [[nodiscard]] VkExtent2D extent() const;

  [[nodiscard]] bool isCreated() const { return _buffer != VK_NULL_HANDLE; }

  [[nodiscard]] const RenderPass& renderPass() const { return *_renderPass; }

  [[nodiscard]] Image& image();
  [[nodiscard]] const Image& image() const;

 private:
  void moveFrom(Framebuffer& rhs);

  void create();

 private:
  VkFramebuffer _buffer = VK_NULL_HANDLE;

  const Device* _device              = nullptr;
  const RenderPass* _renderPass      = nullptr;
  ImageView* _colorAttachment        = nullptr;
  ImageView* _depthStencilAttachment = nullptr;
};

NAMESPACE_END(Vulk)