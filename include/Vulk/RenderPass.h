#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/helpers_vulkan.h>

#include <vector>
#include <functional>

NAMESPACE_VULKAN_BEGIN

class Device;

class RenderPass {
 public:
  using AttachmentDescriptionOverride = std::function<void(std::vector<VkAttachmentDescription>&)>;
  using SubpassDescriptionOverride = std::function<void(std::vector<VkSubpassDescription>&)>;
  using SubpassDependencyOverride = std::function<void(std::vector<VkSubpassDependency>&)>;

 public:
  RenderPass() = default;
  RenderPass(const Device& device,
             VkFormat format,
             const AttachmentDescriptionOverride& attachmentOverride = {},
             const SubpassDescriptionOverride& subpassOverride = {},
             const SubpassDependencyOverride& dependencyOverride = {});
  ~RenderPass();

  // Transfer the ownership from `rhs` to `this`
  RenderPass(RenderPass&& rhs) noexcept;
  RenderPass& operator=(RenderPass&& rhs) noexcept(false);

  void create(const Device& device,
              VkFormat format,
              const AttachmentDescriptionOverride& attachmentOverride = {},
              const SubpassDescriptionOverride& subpassOverride = {},
              const SubpassDependencyOverride& dependencyOverride = {});

  void destroy();

  operator VkRenderPass() const { return _renderPass; }

  [[nodiscard]] bool isCreated() const { return _renderPass != VK_NULL_HANDLE; }

 private:
  void moveFrom(RenderPass& rhs);

 private:
  VkRenderPass _renderPass = VK_NULL_HANDLE;

  const Device* _device = nullptr;
};

NAMESPACE_VULKAN_END