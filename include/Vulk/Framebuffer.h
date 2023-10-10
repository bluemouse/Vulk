#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/helpers_vulkan.h>

NAMESPACE_VULKAN_BEGIN

class Device;
class RenderPass;
class ImageView;

class Framebuffer {
 public:
  Framebuffer(const Device& device, const RenderPass& renderPass, const ImageView& imageView);
  ~Framebuffer();

  // Transfer the ownership from `rhs` to `this`
  Framebuffer(Framebuffer&& rhs) noexcept;
  Framebuffer& operator=(Framebuffer&& rhs) noexcept(false);

  void create(const Device& device, const RenderPass& renderPass, const ImageView& imageView);
  void destroy();

  operator VkFramebuffer() const { return _buffer; }

  [[nodiscard]] VkExtent2D extent() const;

  [[nodiscard]] bool isCreated() const { return _buffer != VK_NULL_HANDLE; }

  [[nodiscard]] const RenderPass* renderPass() const { return _renderPass; }

 private:
  void moveFrom(Framebuffer& rhs);

 private:
  VkFramebuffer _buffer = VK_NULL_HANDLE;

  const Device* _device = nullptr;
  const RenderPass* _renderPass = nullptr;
  const ImageView* _imageView = nullptr;
};

NAMESPACE_VULKAN_END