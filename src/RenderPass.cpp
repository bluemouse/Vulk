#include <Vulk/RenderPass.h>

#include <Vulk/Device.h>

NAMESPACE_BEGIN(Vulk)

RenderPass::RenderPass(const Device& device,
                       VkFormat colorFormat,
                       VkFormat depthStencilFormat,
                       const AttachmentDescriptionOverride& colorAttachmentOverride,
                       const AttachmentDescriptionOverride& depthStencilAttachmentOverride,
                       const SubpassDescriptionOverride& subpassOverride,
                       const SubpassDependencyOverride& dependencyOverride) {
  create(device,
         colorFormat,
         depthStencilFormat,
         colorAttachmentOverride,
         depthStencilAttachmentOverride,
         subpassOverride,
         dependencyOverride);
}

RenderPass::~RenderPass() {
  if (isCreated()) {
    destroy();
  }
}

void RenderPass::create(const Device& device,
                        VkFormat colorFormat,
                        VkFormat depthStencilFormat,
                        const AttachmentDescriptionOverride& colorAttachmentOverride,
                        const AttachmentDescriptionOverride& depthStencilAttachmentOverride,
                        const SubpassDescriptionOverride& subpassOverride,
                        const SubpassDependencyOverride& dependencyOverride) {
  MI_VERIFY(!isCreated());
  _device             = device.get_weak();
  _colorFormat        = colorFormat;
  _depthStencilFormat = depthStencilFormat;

  VkAttachmentDescription colorAttachment{};
  colorAttachment.format         = colorFormat;
  colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  if (colorAttachmentOverride) {
    colorAttachmentOverride(colorAttachment);
  }

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &colorAttachmentRef;

  VkAttachmentDescription depthStencilAttachment{};
  VkAttachmentReference depthStencilAttachmentRef{};
  if (depthStencilFormat != VK_FORMAT_UNDEFINED) {
    depthStencilAttachment.format         = depthStencilFormat;
    depthStencilAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthStencilAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthStencilAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthStencilAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthStencilAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthStencilAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthStencilAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if (depthStencilAttachmentOverride) {
      depthStencilAttachmentOverride(depthStencilAttachment);
    }

    depthStencilAttachmentRef.attachment = 1;
    depthStencilAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    subpass.pDepthStencilAttachment = &depthStencilAttachmentRef;
  }

  if (subpassOverride) {
    subpassOverride(subpass);
  }

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;

  if (depthStencilFormat == VK_FORMAT_UNDEFINED) {
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  } else {
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  }
  if (dependencyOverride) {
    dependencyOverride(dependency);
  }

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.subpassCount    = 1;
  renderPassInfo.pSubpasses      = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies   = &dependency;

  std::vector<VkAttachmentDescription> attachments{{colorAttachment}};
  if (depthStencilFormat != VK_FORMAT_UNDEFINED) {
    attachments.push_back(depthStencilAttachment);
  }
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments    = attachments.data();

  MI_VERIFY_VKCMD(vkCreateRenderPass(device, &renderPassInfo, nullptr, &_renderPass));
}

void RenderPass::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyRenderPass(device(), _renderPass, nullptr);

  _renderPass = VK_NULL_HANDLE;
  _device.reset();
}

NAMESPACE_END(Vulk)
