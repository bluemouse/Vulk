#pragma once

#include <Vulk/engine/DeviceContext.h>
#include <Vulk/engine/FrameContext.h>
#include <Vulk/engine/Drawable.h>
#include <Vulk/engine/Vertex.h>
#include <Vulk/engine/Camera.h>

#include <apps/App.h>
#include <RenderTaskRepo.h>

#include <filesystem>

class ImageViewer : public App {
 public:
  using Vertex = Vulk::Vertex<glm::vec3, glm::vec3, glm::vec2>;

 public:
  explicit ImageViewer(Vulk::DeviceContext::shared_ptr deviceContext);

  void init(const Params& params) override;
  void render() override;
  void cleanup() override;
  void resize(uint width, uint height) override;

  Vulk::Camera& camera() override { return *_camera; }

 protected:
  void drawFrame();

  [[nodiscard]] Vulk::DeviceContext& context() { return *_deviceContext; }
  [[nodiscard]] const Vulk::DeviceContext& context() const { return *_deviceContext; }

 private:
  void createDrawable(const std::filesystem::path& modelFile   = {},
                      const std::filesystem::path& textureFile = {});
  void createRenderTask();
  void createFrames();

  void nextFrame();

  void loadModel(const std::filesystem::path& modelFile,
                 std::vector<Vertex>& vertices,
                 std::vector<uint32_t>& indices);
  void initCamera(const std::vector<Vertex>& vertices);

 private:
  Vulk::DeviceContext* _deviceContext;

  Vulk::TextureMappingTask::shared_ptr _textureMappingTask;
  Vulk::PresentTask::shared_ptr _presentTask;

  Vulk::Camera::shared_ptr _camera;

  Vulk::Drawable<Vertex, uint32_t> _drawable;
  Vulk::Texture2D::shared_ptr _texture;

  struct Frame {
    Vulk::FrameContext::shared_ptr context;

    Vulk::Image2D::shared_ptr colorBuffer;
    Vulk::DepthImage::shared_ptr depthBuffer;
  };

  std::vector<Frame> _frames;
  Frame* _currentFrame = nullptr;

  constexpr static uint32_t _maxFramesInFlight = 3;
  uint32_t _currentFrameIdx                    = 0;
};
