#include <Vulk/RenderPass.h>

#include <Vulk/internal/debug.h>

#include <Vulk/Device.h>

MI_NAMESPACE_BEGIN(Vulk)

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

  std::vector<VkAttachmentDescription> attachments{};

  VkAttachmentDescription colorAttachment{};
  colorAttachment.format         = colorFormat;
  colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  if (colorAttachmentOverride) {
    colorAttachmentOverride(colorAttachment);
  }
  attachments.push_back(colorAttachment);
  constexpr uint32_t colorAttachmentIndex =
      0; // It is the location of this attachment in the `attachments` array

  VkAttachmentDescription depthStencilAttachment{};
  constexpr uint32_t depthStencilAttachmentIndex =
      1; // It is the location of this attachment in the `attachments` array
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

    attachments.push_back(depthStencilAttachment);
  }

  VkAttachmentReference colorAttachmentRef{colorAttachmentIndex,
                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depthStencilAttachmentRef{depthStencilAttachmentIndex,
                                                  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint =
      VK_PIPELINE_BIND_POINT_GRAPHICS; // TODO to support VK_PIPELINE_BIND_POINT_COMPUTE
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &colorAttachmentRef;
  subpass.pDepthStencilAttachment =
      depthStencilFormat != VK_FORMAT_UNDEFINED ? &depthStencilAttachmentRef : nullptr;
  if (subpassOverride) {
    subpassOverride(subpass);
  }

  VkSubpassDependency subpassDependency{};
  subpassDependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
  subpassDependency.dstSubpass    = 0;
  subpassDependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependency.srcAccessMask = 0;
  subpassDependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  if (depthStencilFormat != VK_FORMAT_UNDEFINED) {
    subpassDependency.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subpassDependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpassDependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  }
  if (dependencyOverride) {
    dependencyOverride(subpassDependency);
  }

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.subpassCount    = 1;
  renderPassInfo.pSubpasses      = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies   = &subpassDependency;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments    = attachments.data();

  MI_VERIFY_VK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &_renderPass));
}

void RenderPass::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyRenderPass(device(), _renderPass, nullptr);

  _renderPass = VK_NULL_HANDLE;
  _device.reset();
}

MI_NAMESPACE_END(Vulk)
