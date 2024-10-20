#include "ImageViewer.h"

#include <Vulk/Exception.h>

#include <Vulk/engine/Toolbox.h>

// Defined in CMakeLists.txt:GLM_FORCE_DEPTH_ZERO_TO_ONE, GLM_FORCE_RADIANS, GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <tiny_obj_loader.h>

#include <filesystem>

namespace std {
template <>
struct hash<ImageViewer::Vertex> {
  size_t operator()(const ImageViewer::Vertex& vertex) const {
    return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
           (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};
} // namespace std

namespace {
struct Checkerboard : public std::vector<uint8_t> {
  Checkerboard(glm::uvec2 numBlocks,
               glm::uvec2 blockSize,
               glm::uvec4 black = {0, 0, 0, 255},
               glm::uvec4 white = {255, 255, 255, 255})
      : extent{numBlocks * blockSize} {
    resize(extent.x * extent.y * kNumColorChannels);

    for (unsigned int x = 0; x < extent.x; ++x) {
      for (unsigned int y = 0; y < extent.y; ++y) {
        auto p     = at(x, y);
        auto color = (((x & blockSize.x) == 0) ^ ((y & blockSize.y) == 0)) ? white : black;

        p[0] = color[0];
        p[1] = color[1];
        p[2] = color[2];
        p[3] = color[3];
      }
    }
  }

  glm::uvec4 operator()(uint32_t x, uint32_t y) {
    uint8_t* pixel = at(x, y);
    return {pixel[0], pixel[1], pixel[2], pixel[3]};
  }

  glm::uvec2 extent;

 private:
  static constexpr unsigned int kNumColorChannels = 4;
  uint8_t* at(uint32_t x, uint32_t y) { return data() + (x + y * extent.x) * kNumColorChannels; }
};

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

ImageViewer::ImageViewer() : App{ID, DESCRIPTION} {
}

void ImageViewer::init(Vulk::DeviceContext::shared_ptr deviceContext, const Params& params) {
  App::init(deviceContext, params);

  auto* textureFile = params[PARAM_TEXTURE_FILE];
  createDrawable(textureFile ? textureFile->value<std::filesystem::path>() : "");
  createRenderTask();
  createFrames();

  // After we init all Vulkan resource and before the rendering, make sure the/ device is idle and
  // all resource is ready.
  deviceContext->waitIdle();
}

void ImageViewer::cleanup() {
  // Before we clean up all Vulkan resource, make sure the device is idle.
  deviceContext().waitIdle();

  _textureMappingTask.reset();
  _presentTask.reset();

  _texture->destroy();
  _drawable.destroy();

  for (auto& frame : _frames) {
    frame.colorBuffer->destroy();
    frame.depthBuffer->destroy();
  }
  _frames.clear();
}

void ImageViewer::render() {
  drawFrame();
}

void ImageViewer::resize(uint width, uint height) {
  _camera->update(glm::vec2{width, height});
}

void ImageViewer::drawFrame() {
  try {
    nextFrame(); // Move to the next frame.

    // TODO Label the drawFrame() function in the queue

    // The new current frame may be in use so make sure the last time this frame is rendered to has
    // been finished.
    _currentFrame->context->waitFrameRendered();
    _currentFrame->context->reset();

    _textureMappingTask->setFrameContext(*_currentFrame->context);
    _presentTask->setFrameContext(*_currentFrame->context);

    //
    // Texture Mapping Task
    //
    _textureMappingTask->prepareGeometry(
        _drawable.vertexBuffer(), _drawable.indexBuffer(), _drawable.numIndices());
    _textureMappingTask->prepareUniforms(
        glm::mat4{1.0F}, _camera->viewMatrix(), _camera->projectionMatrix());
    _textureMappingTask->prepareInputs(*_texture);
    _textureMappingTask->prepareOutputs(*_currentFrame->colorBuffer, *_currentFrame->depthBuffer);
    _textureMappingTask->prepareSynchronization();

    auto [frameReady, _] = _textureMappingTask->run();

    //
    // Present Task
    //
    _presentTask->prepareInput(*_currentFrame->colorBuffer);
    _presentTask->prepareSynchronization({frameReady});

    auto [__, framePresented] = _presentTask->run();

    _currentFrame->context->setFrameRendered(framePresented);

  } catch (const Vulk::Exception& e) {
    if (e.result() == VK_ERROR_OUT_OF_DATE_KHR || e.result() == VK_SUBOPTIMAL_KHR) {
      // The size or format of the swapchain image is not correct. We'll just ignore them
      // and let the resize callback to recreate the swapchain.
    } else {
      throw e;
    }
  }
}

void ImageViewer::createRenderTask() {
  _textureMappingTask = Vulk::TextureMappingTask::make_shared(deviceContext());
  _presentTask        = Vulk::PresentTask::make_shared(deviceContext());
}

void ImageViewer::initCamera(const std::vector<Vertex>& vertices) {
  auto bbox = Vulk::Camera::BBox::null();
  for (const auto& vertex : vertices) {
    bbox += vertex.pos;
  }
  bbox.expandPlanarSide(1.0F);

  auto extent = deviceContext().swapchain().surfaceExtent();
  _camera     = Vulk::FlatCamera::make_shared(glm::vec2{extent.width, extent.height}, bbox);
}

void ImageViewer::createDrawable(const std::filesystem::path& textureFile) {
  if (textureFile.empty()) {
    const glm::uvec2 numBlocks{4, 4};
    const glm::uvec2 blockSize{128, 128};
    const glm::uvec4 black{60, 60, 60, 255};
    const glm::uvec4 white{255, 255, 255, 255};
    Checkerboard checkerboard{numBlocks, blockSize, black, white};
    _texture = Vulk::Toolbox(deviceContext())
                   .createTexture2D(Vulk::Toolbox::TextureFormat::RGBA,
                                    checkerboard.data(),
                                    checkerboard.extent.x,
                                    checkerboard.extent.y);

    VkImage image = *_texture;
    deviceContext().device().setObjectName(
        VK_OBJECT_TYPE_IMAGE, (uint64_t)image, "Created texture (checkerboard)");
  } else {
    _texture = Vulk::Toolbox(deviceContext()).createTexture2D(textureFile.c_str());

    std::string name = "Loaded texture (" + textureFile.string() + ")";
    VkImage image    = *_texture;
    deviceContext().device().setObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)image, name.c_str());
  }

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  float left{-1.0F};
  float right{1.0F};
  float bottom{-1.0F};
  float top{1.0F};

  if (_texture->isValid()) {
    // to make sure the texture and the quad has the same aspect ratio
    auto [textureW, textureH] = _texture->extent();
    const float textureAspect = static_cast<float>(textureW) / static_cast<float>(textureH);

    if (textureAspect > 1.0F) {
      left  = -textureAspect;
      right = textureAspect;
    } else {
      bottom = -1.0F / textureAspect;
      top    = 1.0F / textureAspect;
    }
  }
  vertices = {{{left, bottom, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
              {{left, top, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
              {{right, top, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
              {{right, bottom, 0.0F}, {1.0F, 1.0F, 1.0F}, {1.0F, 1.0F}}};
  indices  = {0, 1, 2, 2, 3, 0};

  _drawable.create(deviceContext().device(), vertices, indices);

  initCamera(vertices);
}

void ImageViewer::createFrames() {
  _frames.resize(_maxFramesInFlight);

  _currentFrameIdx = 0;
  _currentFrame    = &_frames[_currentFrameIdx];

  const auto& device = deviceContext().device();
  const auto& extent = deviceContext().swapchain().surfaceExtent();

  std::vector<Vulk::RenderTask*> tasks = {_textureMappingTask.get()};
  for (auto& frame : _frames) {
    frame.context = Vulk::FrameContext::make_shared(deviceContext(), tasks);
  }

  constexpr uint32_t depthBits   = 24U;
  constexpr uint32_t stencilBits = 8U;
  auto depthFormat               = Vulk::DepthImage::findFormat(depthBits, stencilBits);

  auto commandBuffer =
      _currentFrame->context->acquireCommandBuffer(Vulk::Device::QueueFamilyType::Transfer);

  commandBuffer->beginRecording();
  for (auto& frame : _frames) {
    const auto usage  = Vulk::Image2D::Usage::COLOR_ATTACHMENT | Vulk::Image2D::Usage::TRANSFER_SRC;
    frame.colorBuffer = Vulk::Image2D::make_shared(device, VK_FORMAT_B8G8R8A8_SRGB, extent, usage);
    frame.colorBuffer->allocate();
    frame.depthBuffer = Vulk::DepthImage::make_shared(device, extent, depthFormat);
    frame.depthBuffer->allocate();

    frame.colorBuffer->transitToNewLayout(*commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  }
  commandBuffer->endRecording();

  commandBuffer->submitCommands();
  // No need to wait here since we are going to ait for the device idle after before the 1st
  // frame rendering
}

void ImageViewer::nextFrame() {
  _currentFrameIdx = (_currentFrameIdx + 1) % _maxFramesInFlight;
  _currentFrame    = &_frames[_currentFrameIdx];
}
