#include "ParticlesViewer.h"

#include <Vulk/Exception.h>

#include <Vulk/engine/Toolbox.h>

// Defined in CMakeLists.txt:GLM_FORCE_DEPTH_ZERO_TO_ONE, GLM_FORCE_RADIANS, GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <random>
#include <chrono>

namespace std {
template <>
struct hash<ParticlesViewer::Particle> {
  size_t operator()(const ParticlesViewer::Particle& particle) const {
    return ((hash<glm::vec3>()(particle.pos) ^ (hash<glm::vec3>()(particle.color) << 1)) >> 1);
  }
};
} // namespace std

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

ParticlesViewer::ParticlesViewer() : App{ID, DESCRIPTION} {
}

void ParticlesViewer::init(Vulk::DeviceContext::shared_ptr deviceContext, const Params& params) {
  App::init(deviceContext, params);

  createDrawable();
  createRenderTask();
  createFrames();

  // After we init all Vulkan resource and before the rendering, make sure the/ device is idle and
  // all resource is ready.
  deviceContext->waitIdle();
}

void ParticlesViewer::cleanup() {
  // Before we clean up all Vulkan resource, make sure the device is idle.
  deviceContext().waitIdle();

  _particlesRenderingTask.reset();
  _presentTask.reset();

  _drawable.destroy();

  for (auto& frame : _frames) {
    frame.colorBuffer->destroy();
    frame.depthBuffer->destroy();
  }
  _frames.clear();
}

void ParticlesViewer::render() {
  drawFrame();
}

void ParticlesViewer::resize(uint width, uint height) {
  _camera->update(glm::vec2{width, height});
}

void ParticlesViewer::drawFrame() {
  static auto startTime = std::chrono::high_resolution_clock::now();

  // Calculate the elapsed time in seconds
  const auto currentTime  = std::chrono::high_resolution_clock::now();
  const float elapsedTime = std::chrono::duration<float>(currentTime - startTime).count();
  startTime               = currentTime;

  try {
    nextFrame(); // Move to the next frame.

    // TODO Label the drawFrame() function in the queue

    updateDrawable(elapsedTime);

    // The new current frame may be in use so make sure the last time this frame is rendered to has
    // been finished.
    _currentFrame->context->waitFrameRendered();
    _currentFrame->context->reset();

    _particlesRenderingTask->setFrameContext(*_currentFrame->context);
    _presentTask->setFrameContext(*_currentFrame->context);

    //
    //
    //
    _particlesRenderingTask->prepareGeometry(_drawable.vertexBuffer());
    _particlesRenderingTask->prepareUniforms(
        glm::mat4{1.0F}, _camera->viewMatrix(), _camera->projectionMatrix());
    _particlesRenderingTask->prepareInputs();
    _particlesRenderingTask->prepareOutputs(*_currentFrame->colorBuffer,
                                            *_currentFrame->depthBuffer);
    _particlesRenderingTask->prepareSynchronization();

    auto [frameReady, _] = _particlesRenderingTask->run();

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

void ParticlesViewer::createRenderTask() {
  _particlesRenderingTask = Vulk::ParticlesRenderingTask::make_shared(deviceContext());
  _presentTask            = Vulk::PresentTask::make_shared(deviceContext());
}

void ParticlesViewer::initCamera(const std::vector<Particle>& particles) {
  auto bbox = Vulk::Camera::BBox::null();
  for (const auto& vertex : particles) {
    bbox += vertex.pos;
  }
  bbox.expandPlanarSide(1.0F);

  auto extent = deviceContext().swapchain().surfaceExtent();
  _camera     = Vulk::ArcCamera::make_shared(glm::vec2{extent.width, extent.height}, bbox);
}

void ParticlesViewer::initParticles() {
  std::default_random_engine rndEngine((unsigned)time(nullptr));
  std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

  constexpr uint32_t numParticles = 32 * 1024U;
  _particles.resize(numParticles);

  for (auto& particle : _particles) {
    // Cube root to ensure uniform distribution on the sphere surface
    float r = 0.25F;
    // Azimuthal angle
    float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
    // Polar angle
    float phi = acos(2.0f * rndDist(rndEngine) - 1.0f);

    float x = r * sin(phi) * cos(theta);
    float y = r * sin(phi) * sin(theta);
    float z = r * cos(phi);

    particle.pos   = glm::vec3(x, y, z);
    particle.color = glm::vec4(x * x * 2, y * y * 2, z * z * 2, 1.0f);
  }
}

void ParticlesViewer::createDrawable() {
  initParticles();

  _drawable.create(deviceContext().device(), _particles);

  initCamera(_particles);
}

void ParticlesViewer::updateParticles(float deltaTime) {
  // Define the rotation angle based on the delta time (in seconds)
  float angle = glm::radians(12.0F) * deltaTime; // Rotate by 12 degree per second

  // Create the rotation matrix around the y-axis
  glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0F), angle, glm::vec3(0.0F, 1.0F, 0.0F));

  for (auto& particle : _particles) {
    glm::vec4 pos = glm::vec4(particle.pos, 1.0F);
    pos           = rotationMatrix * pos;
    particle.pos  = glm::vec3(pos);
  }
}

void ParticlesViewer::updateDrawable(float deltaTime) {
  updateParticles(deltaTime);
  _drawable.update(_particles);
}

void ParticlesViewer::createFrames() {
  _frames.resize(_maxFramesInFlight);

  _currentFrameIdx = 0;
  _currentFrame    = &_frames[_currentFrameIdx];

  const auto& device = deviceContext().device();
  const auto& extent = deviceContext().swapchain().surfaceExtent();

  std::vector<Vulk::RenderTask*> tasks = {_particlesRenderingTask.get()};
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

void ParticlesViewer::nextFrame() {
  _currentFrameIdx = (_currentFrameIdx + 1) % _maxFramesInFlight;
  _currentFrame    = &_frames[_currentFrameIdx];
}
