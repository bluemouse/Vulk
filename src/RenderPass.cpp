#include <Vulk/RenderPass.h>

#include <Vulk/Device.h>

NAMESPACE_VULKAN_BEGIN

RenderPass::RenderPass(const Device& device,
                       VkFormat format,
                       const AttachmentDescriptionOverride& attachmentOverride,
                       const SubpassDescriptionOverride& subpassOverride,
                       const SubpassDependencyOverride& dependencyOverride) {
  create(device, format, attachmentOverride, subpassOverride, dependencyOverride);
}

RenderPass::~RenderPass() {
  if (isCreated()) {
    destroy();
  }
}

RenderPass::RenderPass(RenderPass&& rhs) noexcept {
  moveFrom(rhs);
}

RenderPass& RenderPass::operator=(RenderPass&& rhs) noexcept(false) {
  if (this != &rhs) {
    moveFrom(rhs);
  }
  return *this;
}

void RenderPass::moveFrom(RenderPass& rhs) {
  MI_VERIFY(!isCreated());
  _renderPass = rhs._renderPass;
  _device = rhs._device;

  rhs._renderPass = VK_NULL_HANDLE;
  rhs._device = nullptr;
}

void RenderPass::create(const Device& device,
                        VkFormat format,
                        const AttachmentDescriptionOverride& attachmentOverride,
                        const SubpassDescriptionOverride& subpassOverride,
                        const SubpassDependencyOverride& dependencyOverride) {
  MI_VERIFY(!isCreated());
  _device = &device;

  std::vector<VkAttachmentDescription> attachments{1};
  attachments[0].format = format;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  if (attachmentOverride) {
    attachmentOverride(attachments);
  }

  std::vector<VkAttachmentReference> attachmentReferences{1};
  attachmentReferences[0].attachment = 0;
  attachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  std::vector<VkSubpassDescription> subpasses{1};
  subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpasses[0].colorAttachmentCount = attachmentReferences.size();
  subpasses[0].pColorAttachments = attachmentReferences.data();

  if (subpassOverride) {
    subpassOverride(subpasses);
  }

  std::vector<VkSubpassDependency> dependencies{1};
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = 0;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  if (dependencyOverride) {
    dependencyOverride(dependencies);
  }

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = attachments.size();
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = subpasses.size();
  renderPassInfo.pSubpasses = subpasses.data();
  renderPassInfo.dependencyCount = dependencies.size();
  renderPassInfo.pDependencies = dependencies.data();

  MI_VERIFY_VKCMD(vkCreateRenderPass(device, &renderPassInfo, nullptr, &_renderPass));
}

void RenderPass::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyRenderPass(*_device, _renderPass, nullptr);

  _renderPass = VK_NULL_HANDLE;
  _device = nullptr;
}

NAMESPACE_VULKAN_END
