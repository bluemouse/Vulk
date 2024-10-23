#pragma once

#include <Vulk/engine/DeviceContext.h>
#include <Vulk/engine/FrameContext.h>
#include <Vulk/engine/Drawable.h>
#include <Vulk/engine/Vertex.h>
#include <Vulk/engine/Camera.h>

#include <apps/App.h>
#include <RenderTaskRepo.h>

#include <filesystem>

class ParticlesViewer : public App {
 public:
  using Particle = Vulk::VertexPC<glm::vec3, glm::vec4>;

 public:
  ParticlesViewer();

  void init(Vulk::DeviceContext::shared_ptr deviceContext, const Params& params) override;
  void render() override;
  void cleanup() override;
  void resize(uint width, uint height) override;

  [[nodiscard]] bool isPlaying() const override { return true; }

  Vulk::Camera& camera() override { return *_camera; }

  static constexpr const char* ID          = "ParticlesViewer";
  static constexpr const char* DESCRIPTION = "Basic viewer of the particles";

 protected:
  void drawFrame();

 private:
  void createDrawable();
  void createRenderTask();
  void createFrames();

  void nextFrame();

  void initCamera(const std::vector<Particle>& vertices);

  void initParticles();
  void updateParticles(float deltaTime);

  void updateDrawable(float deltaTime);

 private:
  Vulk::ParticlesRenderingTask::shared_ptr _particlesRenderingTask;
  Vulk::PresentTask::shared_ptr _presentTask;

  Vulk::Camera::shared_ptr _camera;

  Vulk::PointsDrawable<Particle> _drawable;

  struct Frame {
    Vulk::FrameContext::shared_ptr context;

    Vulk::Image2D::shared_ptr colorBuffer;
    Vulk::DepthImage::shared_ptr depthBuffer;
  };

  std::vector<Frame> _frames;
  Frame* _currentFrame = nullptr;

  constexpr static uint32_t _maxFramesInFlight = 3;
  uint32_t _currentFrameIdx                    = 0;

  std::vector<Particle> _particles;
};
