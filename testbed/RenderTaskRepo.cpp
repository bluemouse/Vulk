#include "RenderTaskRepo.h"

#include <Vulk/Device.h>
#include <Vulk/CommandPool.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/VertexShader.h>
#include <Vulk/FragmentShader.h>
#include <Vulk/Exception.h>

#include <Vulk/internal/debug.h>

#include <filesystem>
#include <cstring>
#include <thread>

namespace {
#if defined(__linux__)
std::filesystem::path executablePath() {
  return std::filesystem::canonical("/proc/self/exe").parent_path();
}
#else
std::filesystem::path executablePath() {
  return std::filesystem::path{};
}
#endif
} // namespace

MI_NAMESPACE_BEGIN(Vulk)

//
//
//
namespace {
struct Uniforms {
  alignas(sizeof(glm::vec4)) glm::mat4 model;
  alignas(sizeof(glm::vec4)) glm::mat4 view;
  alignas(sizeof(glm::vec4)) glm::mat4 proj;

  static UniformBuffer::shared_ptr allocateBuffer(const Device& device) {
    return UniformBuffer::make_shared(device, sizeof(Uniforms));
  }

  static VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(uint32_t binding) {
    return {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
  }
};
} // namespace

TextureMappingTask::TextureMappingTask(const DeviceContext::shared_ptr& deviceContext)
    : RenderTask(deviceContext, Type::Graphics) {
  const auto& device = _deviceContext->device();

  // Create render pass
  const VkFormat colorFormat        = VK_FORMAT_B8G8R8A8_SRGB;
  const VkFormat depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
  _renderPass = RenderPass::make_shared(device, colorFormat, depthStencilFormat);

  // Create pipeline
  auto vertShaderFile = executablePath() / "shaders/vert.spv";
  VertexShader vertShader{device, vertShaderFile.string().c_str()};
  auto fragShaderFile = executablePath() / "shaders/frag.spv";
  FragmentShader fragShader{device, fragShaderFile.string().c_str()};
  _pipeline = Pipeline::make_shared(device, *_renderPass, vertShader, fragShader);

  // TODO uniform buffers should be managed by FrameContext
  // Create descriptor pool
  constexpr int maxFramesInFlight = 3;
  for (int i = 0; i < maxFramesInFlight; ++i) {
    auto uniforms = Uniforms::allocateBuffer(device);
    _uniformBuffers.push_back(uniforms);
    _uniformBufferMapped.push_back(uniforms->map());
  }
  // For now, we only have one vertex buffer binding (hence, 0 indexed). Check
  // `ShaderModule::reflectVertexInputs` to see how `_vertexInputBindings` are reflected.
  _vertexBufferBinding = 0U;
}

TextureMappingTask::~TextureMappingTask() {
}

void TextureMappingTask::prepareGeometry(const VertexBuffer& vertexBuffer,
                                         const IndexBuffer& indexBuffer,
                                         size_t numIndices) {
  _vertexBuffer = vertexBuffer.get_shared();
  _indexBuffer  = indexBuffer.get_shared();
  _numIndices   = numIndices;
}

void TextureMappingTask::prepareUniforms(const glm::mat4& model2world,
                                         const glm::mat4& world2view,
                                         const glm::mat4& projection,
                                         bool newFrame) {
  Uniforms uniforms{};

  uniforms.model = model2world;
  uniforms.view  = world2view;
  uniforms.proj  = projection;

  if (newFrame) {
    _currentUniformBufferIdx = (_currentUniformBufferIdx + 1) % _uniformBuffers.size();
  }
  memcpy(_uniformBufferMapped[_currentUniformBufferIdx], &uniforms, sizeof(uniforms));
}

void TextureMappingTask::prepareInputs(const Texture2D& texture) {
  _texture = texture.get_shared();
}

void TextureMappingTask::prepareOutputs(const Image2D& colorBuffer,
                                        const DepthImage& depthStencilBuffer) {
  _colorBuffer        = colorBuffer.get_shared();
  _depthStencilBuffer = depthStencilBuffer.get_shared();
}

void TextureMappingTask::prepareSynchronization(const std::vector<Semaphore::shared_ptr>& waits) {
  _waits = waits;
}

std::pair<Semaphore::shared_ptr, Fence::shared_ptr> TextureMappingTask::run() {
  const auto& device = _deviceContext->device();

  auto fence  = Fence::make_shared(device);
  auto signal = Semaphore::make_shared(device);

  std::vector<Semaphore*> waits;
  waits.reserve(_waits.size());
  for (const auto& semaphore : _waits) {
    waits.push_back(semaphore.get());
  }

  auto label = _commandBuffer->queue().scopedLabel("TextureMappingTask::run()");

  VkDescriptorImageInfo textureImageInfo{};
  textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  textureImageInfo.imageView   = _texture->view();
  textureImageInfo.sampler     = _texture->sampler();

  VkDescriptorBufferInfo transformationBufferInfo{};
  transformationBufferInfo.offset = 0;
  transformationBufferInfo.range  = VK_WHOLE_SIZE;
  transformationBufferInfo.buffer = *_uniformBuffers[_currentUniformBufferIdx];

  // TODO We should cache and reuse descriptor sets. For now, we create a new one for each frame and
  //      reset the pool at the end of the frame (see `_descriptorPool->reset()`).
  auto descriptorSet = _frameContext->acquireDescriptorSet(*_pipeline->descriptorSetLayout());

  // The order of bindings must match the order of bindings in shaders. The name and the type need
  // to match them in the shader as well.
  std::vector<DescriptorSet::Binding> bindings = {
      {"xform", "Transformation", &transformationBufferInfo},
      {"texSampler", "sampler2D", &textureImageInfo}};


  descriptorSet->bind(bindings);

  auto colorAttachment        = ImageView::make_shared(device, *_colorBuffer);
  auto depthStencilAttachment = ImageView::make_shared(device, *_depthStencilBuffer);

  // TODO We should cache and reuse framebuffers. For nolayoutw, we create a new one for each frame.
  auto framebuffer =
      Framebuffer::make_shared(device, *_renderPass, *colorAttachment, *depthStencilAttachment);

  _commandBuffer->beginRecording();
  {
    auto label = _commandBuffer->scopedLabel("Frame");

    _commandBuffer->beginRenderPass(*_renderPass, *framebuffer);

    _commandBuffer->bindPipeline(*_pipeline);

    auto extent = framebuffer->extent();
    _commandBuffer->setViewport({0.0F, 0.0F}, {extent.width, extent.height});

    _commandBuffer->bindVertexBuffer(*_vertexBuffer, _vertexBufferBinding);
    _commandBuffer->bindIndexBuffer(*_indexBuffer);
    _commandBuffer->bindDescriptorSet(*_pipeline, *descriptorSet);

    _commandBuffer->drawIndexed(_numIndices);

    _commandBuffer->endRenderpass();
  }
  _commandBuffer->endRecording();
  _commandBuffer->submitCommands(waits, {signal.get()}, *fence);

  // Start a new thread to wait for the fence to be signaled and then release the resource
  auto releaser = [](Fence::shared_ptr_const fence,
                     Semaphore::shared_ptr signal,
                     std::vector<Semaphore::shared_ptr> waits,
                     Framebuffer::shared_ptr framebuffer,
                     ImageView::shared_ptr colorAttachment,
                     ImageView::shared_ptr depthStencilAttachment) {
    fence->wait();

    fence.reset();
    signal.reset();
    waits.clear();

    framebuffer.reset();
    colorAttachment.reset();
    depthStencilAttachment.reset();
  };
  std::thread{releaser,
              fence,
              signal,
              _waits,
              framebuffer,
              colorAttachment,
              depthStencilAttachment}
      .detach();

  return {signal, fence};
}

DescriptorSetLayout::shared_ptr TextureMappingTask::descriptorSetLayout() {
  return _pipeline->descriptorSetLayout();
}

//
//
//
PresentTask::PresentTask(const DeviceContext::shared_ptr& deviceContext)
    : RenderTask(deviceContext, Type::Transfer) {
  _signals.resize(deviceContext->swapchain().images().size());
  for (auto& signal : _signals) {
    signal = Semaphore::make_shared(deviceContext->device());
  }
}

PresentTask::~PresentTask() {
}

void PresentTask::prepareInput(const Image2D& frame) {
  _frame = frame.get_shared();
}

void PresentTask::prepareSynchronization(const std::vector<Semaphore::shared_ptr>& waits) {
  _waits = waits;
}

std::pair<Semaphore::shared_ptr, Fence::shared_ptr> PresentTask::run() {
  const auto& swapchain = _deviceContext->swapchain();

  auto label = _commandBuffer->queue().scopedLabel("PresentTask::run()");

  Semaphore::shared_ptr swapchainImageReady = Semaphore::make_shared(device());
  _deviceContext->swapchain().acquireNextImage(*swapchainImageReady);

  // Create a vector<Semaphore*> from _waits and swapchainImageReady
  std::vector<Semaphore*> waits;
  waits.reserve(_waits.size() + 1);
  for (const auto& semaphore : _waits) {
    waits.push_back(semaphore.get());
  }
  waits.push_back(swapchainImageReady.get());

  auto fence = Fence::make_shared(device());

  // Signal must've been un-signaled since we can acquire this swapchain image.
  auto signal = _signals[swapchain.activeImageIndex()];

  _commandBuffer->beginRecording();
  {
    auto& swapchainFrame = const_cast<Image&>(swapchain.activeImage());
    swapchainFrame.blitFrom(*_commandBuffer, *_frame);
    swapchainFrame.transitToNewLayout(*_commandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  }
  _commandBuffer->endRecording();
  _commandBuffer->submitCommands(waits, {signal.get()}, *fence);

  swapchain.present({signal.get()});

  // Start a new thread to wait for the fence to be signaled and then release the resource
  auto releaser = [](Fence::shared_ptr fence,
                     std::vector<Semaphore::shared_ptr> waits,
                     Semaphore::shared_ptr swapchainImageReady) {
    fence->wait();

    fence.reset();
    waits.clear();
    swapchainImageReady.reset();
  };
  std::thread{releaser, fence, _waits, swapchainImageReady}.detach();

  return {signal, fence};
}

MI_NAMESPACE_END(Vulk)