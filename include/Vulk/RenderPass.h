#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <functional>
#include <memory>

#include <Vulk/internal/base.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class RenderPass : public Sharable<RenderPass>, private NotCopyable {
 public:
  using AttachmentDescriptionOverride = std::function<void(VkAttachmentDescription&)>;
  using SubpassDescriptionOverride    = std::function<void(VkSubpassDescription&)>;
  using SubpassDependencyOverride     = std::function<void(VkSubpassDependency&)>;

 public:
  RenderPass(const Device& device,
             VkFormat colorFormat,
             VkFormat depthStencilFormat                                  = VK_FORMAT_UNDEFINED,
             const AttachmentDescriptionOverride& colorAttachmentOverride = {},
             const AttachmentDescriptionOverride& depthStencilAttachmentOverride = {},
             const SubpassDescriptionOverride& subpassOverride                   = {},
             const SubpassDependencyOverride& dependencyOverride                 = {});
  ~RenderPass() override;

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

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

 private:
  VkRenderPass _renderPass = VK_NULL_HANDLE;

  VkFormat _colorFormat        = VK_FORMAT_UNDEFINED;
  VkFormat _depthStencilFormat = VK_FORMAT_UNDEFINED;

  std::weak_ptr<const Device> _device;
};

NAMESPACE_END(Vulk)