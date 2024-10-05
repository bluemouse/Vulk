#include "RenderTask.h"

#include <Vulk/Device.h>
#include <Vulk/CommandPool.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/VertexShader.h>
#include <Vulk/FragmentShader.h>
#include <Vulk/Exception.h>

#include <Vulk/internal/debug.h>

#include <filesystem>
#include <cstring>

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

RenderTask::RenderTask(const Vulk::Context& context) : _context(context) {
}

//
//
//
namespace {
struct Uniforms {
  alignas(sizeof(glm::vec4)) glm::mat4 model;
  alignas(sizeof(glm::vec4)) glm::mat4 view;
  alignas(sizeof(glm::vec4)) glm::mat4 proj;

  static Vulk::UniformBuffer::shared_ptr allocateBuffer(const Vulk::Device& device) {
    return Vulk::UniformBuffer::make_shared(device, sizeof(Uniforms));
  }

  static VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(uint32_t binding) {
    return {binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
  }
};
} // namespace

TextureMappingTask::TextureMappingTask(const Vulk::Context& context) : RenderTask(context) {
  const auto& device = _context.device();

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

  // Create descriptor pool
  constexpr int maxFramesInFlight = 3;
  _descriptorPool =
      DescriptorPool::make_shared(_pipeline->descriptorSetLayout(), maxFramesInFlight);

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

void TextureMappingTask::prepareGeometry(const Vulk::VertexBuffer& vertexBuffer,
                                         const Vulk::IndexBuffer& indexBuffer,
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

void TextureMappingTask::prepareInputs(const Vulk::Texture2D& texture) {
  _texture = texture.get_shared();
}

void TextureMappingTask::prepareOutputs(const Vulk::Image2D& colorBuffer,
                                        const Vulk::DepthImage& depthStencilBuffer) {
  _colorBuffer        = colorBuffer.get_shared();
  _depthStencilBuffer = depthStencilBuffer.get_shared();
}

void TextureMappingTask::prepareSynchronization(const std::vector<Vulk::Semaphore*> waits,
                                                const std::vector<Vulk::Semaphore*> signals,
                                                const Vulk::Fence& fence) {
  _waits   = waits;
  _signals = signals;
  if (fence.isCreated()) {
    _fence = fence.get_shared();
  } else {
    _fence = Fence::make_shared();
  }
}

void TextureMappingTask::run(CommandBuffer& commandBuffer) {
  const auto& device = _context.device();

  auto label = commandBuffer.queue().scopedLabel("TextureMappingTask::run()");

  VkDescriptorImageInfo textureImageInfo{};
  textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  textureImageInfo.imageView   = _texture->view();
  textureImageInfo.sampler     = _texture->sampler();

  VkDescriptorBufferInfo transformationBufferInfo{};
  transformationBufferInfo.offset = 0;
  transformationBufferInfo.range  = VK_WHOLE_SIZE;
  transformationBufferInfo.buffer = *_uniformBuffers[_currentUniformBufferIdx];

  // The order of bindings must match the order of bindings in shaders. The name and the type need
  // to match them in the shader as well.
  std::vector<Vulk::DescriptorSet::Binding> bindings = {
      {"xform", "Transformation", &transformationBufferInfo},
      {"texSampler", "sampler2D", &textureImageInfo}};

  // TODO We should cache and reuse descriptor sets. For now, we create a new one for each frame and
  //      reset the pool at the edn of the frame (see `_descriptorPool->reset()`).
  auto descriptorSet =
      DescriptorSet::make_shared(*_descriptorPool, _pipeline->descriptorSetLayout());

  descriptorSet->bind(bindings);

  auto colorAttachment        = Vulk::ImageView::make_shared(device, *_colorBuffer);
  auto depthStencilAttachment = Vulk::ImageView::make_shared(device, *_depthStencilBuffer);

  // TODO We should cache and reuse framebuffers. For now, we create a new one for each frame.
  //      We need to call `queue.waitIdle()` at the end of the frame to ensure that the framebuffer
  //      is not in use anymore before we we can release it.
  Vulk::Framebuffer framebuffer{device, *_renderPass, *colorAttachment, *depthStencilAttachment};

  commandBuffer.reset();

  commandBuffer.beginRecording();
  {
    auto label = commandBuffer.scopedLabel("Frame");

    commandBuffer.beginRenderPass(*_renderPass, framebuffer);

    commandBuffer.bindPipeline(*_pipeline);

    auto extent = framebuffer.extent();
    commandBuffer.setViewport({0.0F, 0.0F}, {extent.width, extent.height});

    commandBuffer.bindVertexBuffer(*_vertexBuffer, _vertexBufferBinding);
    commandBuffer.bindIndexBuffer(*_indexBuffer);
    commandBuffer.bindDescriptorSet(*_pipeline, *descriptorSet);

    commandBuffer.drawIndexed(_numIndices);

    commandBuffer.endRenderpass();
  }
  commandBuffer.endRecording();

  commandBuffer.submitCommands(_waits, _signals, *_fence);

  commandBuffer.queue().waitIdle(); // TODO: This is a temporary solution to reset the descriptor pool (or other
                  // resources). My might re-org the task to be three phases: prepare, run, finish
  _descriptorPool->reset(); // Free all sets allocated from this pool
}

Vulk::DescriptorSet::shared_ptr TextureMappingTask::createDescriptorSet() {
  return Vulk::DescriptorSet::make_shared(*_descriptorPool, _pipeline->descriptorSetLayout());
}

//
//
//
AcquireSwapchainImageTask::AcquireSwapchainImageTask(const Vulk::Context& context)
    : RenderTask(context) {
}

AcquireSwapchainImageTask::~AcquireSwapchainImageTask() {
}

void AcquireSwapchainImageTask::prepareSynchronization(const Vulk::Semaphore* signal) {
  _signal = signal;
}

void AcquireSwapchainImageTask::run(CommandBuffer& commandBuffer) {
  auto label = commandBuffer.queue().scopedLabel("AcquireSwapchainImageTask::run()");

  _context.swapchain().acquireNextImage(*_signal);
}

//
//
//
PresentTask::PresentTask(const Vulk::Context& context) : RenderTask(context) {
}

PresentTask::~PresentTask() {
}

void PresentTask::prepareInput(const Vulk::Image2D& frame) {
  _frame = frame.get_shared();
}

void PresentTask::prepareSynchronization(const std::vector<Vulk::Semaphore*> waits,
                                         const std::vector<Vulk::Semaphore*> readyToPreset) {
  _waits = waits;
  _readyToPresent = readyToPreset;
}

void PresentTask::run(CommandBuffer& commandBuffer) {
  auto label = commandBuffer.queue().scopedLabel("PresentTask::run()");

  commandBuffer.beginRecording();
  {
    // TODO: active swapchain image is not VK_IMAGE_LAYOUT_UNDEFINED after the 1 frame rendering!
    auto& swapchainFrame = const_cast<Image&>(_context.swapchain().activeImage());
    swapchainFrame.blitFrom(commandBuffer, *_frame, _waits);
    swapchainFrame.transitToNewLayout(commandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  }
  commandBuffer.endRecording();
  commandBuffer.submitCommands(_waits, _readyToPresent);

  _context.swapchain().present(_readyToPresent);
}

MI_NAMESPACE_END(Vulk)