#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/helpers_vulkan.h>

NAMESPACE_BEGIN(Vulk)

class Device;
class RenderPass;
class ImageView;

class Framebuffer {
 public:
  Framebuffer() = default;
  Framebuffer(const Device& device, const RenderPass& renderPass, const ImageView& colorAttachment);
  Framebuffer(const Device& device,
              const RenderPass& renderPass,
              const ImageView& colorAttachment,
              const ImageView& depthStencilAttachment);
  ~Framebuffer();

  // Transfer the ownership from `rhs` to `this`
  Framebuffer(Framebuffer&& rhs) noexcept;
  Framebuffer& operator=(Framebuffer&& rhs) noexcept(false);

  void create(const Device& device, const RenderPass& renderPass, const ImageView& colorAttachment);
  void create(const Device& device,
              const RenderPass& renderPass,
              const ImageView& colorAttachment,
              const ImageView& depthStencilAttachment);
  void destroy();

  operator VkFramebuffer() const { return _buffer; }

  [[nodiscard]] VkExtent2D extent() const;

  [[nodiscard]] bool isCreated() const { return _buffer != VK_NULL_HANDLE; }

  [[nodiscard]] const RenderPass* renderPass() const { return _renderPass; }

 private:
  void moveFrom(Framebuffer& rhs);

  void create();

 private:
  VkFramebuffer _buffer = VK_NULL_HANDLE;

  const Device* _device                    = nullptr;
  const RenderPass* _renderPass            = nullptr;
  const ImageView* _colorAttachment        = nullptr;
  const ImageView* _depthStencilAttachment = nullptr;
};

NAMESPACE_END(Vulk)