#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/base.h>
#include <Vulk/internal/vulkan_debug.h>

#include <vector>
#include <functional>

NAMESPACE_BEGIN(Vulk)

class Device;

class RenderPass {
 public:
  using AttachmentDescriptionOverride = std::function<void(VkAttachmentDescription&)>;
  using SubpassDescriptionOverride    = std::function<void(VkSubpassDescription&)>;
  using SubpassDependencyOverride     = std::function<void(VkSubpassDependency&)>;

 public:
  RenderPass() = default;
  RenderPass(const Device& device,
             VkFormat colorFormat,
             VkFormat depthStencilFormat                                  = VK_FORMAT_UNDEFINED,
             const AttachmentDescriptionOverride& colorAttachmentOverride = {},
             const AttachmentDescriptionOverride& depthStencilAttachmentOverride = {},
             const SubpassDescriptionOverride& subpassOverride                   = {},
             const SubpassDependencyOverride& dependencyOverride                 = {});
  ~RenderPass();

  // Transfer the ownership from `rhs` to `this`
  RenderPass(RenderPass&& rhs) noexcept;
  RenderPass& operator=(RenderPass&& rhs) noexcept(false);

  void create(const Device& device,
              VkFormat colorFormat,
              VkFormat depthStencilFormat                                  = VK_FORMAT_UNDEFINED,
              const AttachmentDescriptionOverride& colorAttachmentOverride = {},
              const AttachmentDescriptionOverride& depthStencilAttachmentOverride = {},
              const SubpassDescriptionOverride& subpassOverride                   = {},
              const SubpassDependencyOverride& dependencyOverride                 = {});

  void destroy();

  operator VkRenderPass() const { return _renderPass; }

  [[nodiscard]] bool isCreated() const { return _renderPass != VK_NULL_HANDLE; }

  [[nodiscard]] bool hasDepthStencilAttachment() const {
    return _depthStencilFormat != VK_FORMAT_UNDEFINED;
  }

  VkFormat colorFormat() const { return _colorFormat; }
  VkFormat depthStencilFormat() const { return _depthStencilFormat; }

 private:
  void moveFrom(RenderPass& rhs);

 private:
  VkRenderPass _renderPass = VK_NULL_HANDLE;

  VkFormat _colorFormat        = VK_FORMAT_UNDEFINED;
  VkFormat _depthStencilFormat = VK_FORMAT_UNDEFINED;

  const Device* _device = nullptr;
};

NAMESPACE_END(Vulk)