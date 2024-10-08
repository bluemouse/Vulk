#include <Vulk/Pipeline.h>

#include <utility>

#include <Vulk/internal/debug.h>

#include <Vulk/Device.h>
#include <Vulk/RenderPass.h>
#include <Vulk/ShaderModule.h>
#include <Vulk/VertexShader.h>
#include <Vulk/FragmentShader.h>
#include <Vulk/ComputeShader.h>

MI_NAMESPACE_BEGIN(Vulk)

Pipeline::Pipeline(const Device &device,
                   const RenderPass &renderPass,
                   const VertexShader &vertShader,
                   const FragmentShader &fragShader) {
  create(device, renderPass, vertShader, fragShader);
}

Pipeline::~Pipeline() {
  if (isCreated()) {
    destroy();
  }
}

void Pipeline::create(const Device &device,
                      const RenderPass &renderPass,
                      const VertexShader &vertShader,
                      const FragmentShader &fragShader) {
  MI_VERIFY(!isCreated());
  _device = device.get_weak();

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShader;
  vertShaderStageInfo.pName  = vertShader.entry();

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShader;
  fragShaderStageInfo.pName  = fragShader.entry();

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo,
                                                                 fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  _vertexInputBindings  = vertShader.vertexInputBindings();
  vertexInputInfo.vertexBindingDescriptionCount = _vertexInputBindings.size();
  vertexInputInfo.pVertexBindingDescriptions    = _vertexInputBindings.data();
  const auto &vertexAttributes                  = vertShader.vertexInputAttributes();
  std::vector<VkVertexInputAttributeDescription> attributesDescriptions{};
  for (const auto &attr : vertexAttributes) {
    attributesDescriptions.push_back(attr.vkDescription);
  }
  vertexInputInfo.vertexAttributeDescriptionCount = attributesDescriptions.size();
  vertexInputInfo.pVertexAttributeDescriptions    = attributesDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount  = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0F;
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable  = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable       = VK_TRUE;
  depthStencil.depthWriteEnable      = VK_TRUE;
  depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable     = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable     = VK_FALSE;
  colorBlending.logicOp           = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount   = 1;
  colorBlending.pAttachments      = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0F;
  colorBlending.blendConstants[1] = 0.0F;
  colorBlending.blendConstants[2] = 0.0F;
  colorBlending.blendConstants[3] = 0.0F;

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates    = dynamicStates.data();

  MI_VERIFY(!_descriptorSetLayout || !_descriptorSetLayout->isCreated());
  _descriptorSetLayout = DescriptorSetLayout::make_shared(device, vertShader, fragShader);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts    = *_descriptorSetLayout;

  MI_VERIFY_VK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_layout));

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount          = 2;
  pipelineInfo.pStages             = shaderStages.data();
  pipelineInfo.pVertexInputState   = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState      = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState   = &multisampling;
  if (renderPass.hasDepthStencilAttachment()) {
    pipelineInfo.pDepthStencilState = &depthStencil;
  }
  pipelineInfo.pColorBlendState   = &colorBlending;
  pipelineInfo.pDynamicState      = &dynamicState;
  pipelineInfo.layout             = _layout;
  pipelineInfo.renderPass         = renderPass;
  pipelineInfo.subpass            = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  MI_VERIFY_VK_RESULT(
      vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline));
}

void Pipeline::create(const Device &device, const ComputeShader &compShader) {
  MI_VERIFY(!isCreated());
  _device = device.get_weak();

  VkPipelineShaderStageCreateInfo compShaderStageInfo{};
  compShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  compShaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  compShaderStageInfo.module = compShader;
  compShaderStageInfo.pName  = compShader.entry();

   MI_VERIFY(!_descriptorSetLayout || !_descriptorSetLayout->isCreated());
   _descriptorSetLayout = DescriptorSetLayout::make_shared(device, compShader);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts    = *_descriptorSetLayout;

  MI_VERIFY_VK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_layout));

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage  = compShaderStageInfo;
  pipelineInfo.layout = _layout;

  MI_VERIFY_VK_RESULT(
      vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline));
}

void Pipeline::destroy() {
  MI_VERIFY(isCreated());
  vkDestroyPipeline(device(), _pipeline, nullptr);
  vkDestroyPipelineLayout(device(), _layout, nullptr);
  _descriptorSetLayout.reset();

  _pipeline = VK_NULL_HANDLE;
  _device.reset();
}

MI_NAMESPACE_END(Vulk)
