#pragma once

#include "MainWindow.h"

#include <Vulk/Framebuffer.h>
#include <Vulk/Fence.h>
#include <Vulk/Image2D.h>
#include <Vulk/DepthImage.h>
#include <Vulk/RenderPass.h>
#include <Vulk/Pipeline.h>
#include <Vulk/VertexShader.h>
#include <Vulk/FragmentShader.h>
#include <Vulk/DescriptorSet.h>
#include <Vulk/DescriptorPool.h>
#include <Vulk/UniformBuffer.h>

#include <Vulk/engine/Context.h>
#include <Vulk/engine/Drawable.h>
#include <Vulk/engine/Texture2D.h>
#include <Vulk/engine/Vertex.h>
#include <Vulk/engine/Camera.h>

#include "RenderTask.h"

class Testbed : public MainWindow {
 public:
  using Vertex = Vulk::Vertex<glm::vec3, glm::vec3, glm::vec2>;

 public:
  void init(int width, int height) override;
  void cleanup() override;
  void run() override;

  void drawFrame() override;

  void setModelFile(const std::string& modelFile);
  void setTextureFile(const std::string& textureFile);

  // Settings of the Testbed execution
  using ValidationLevel = Vulk::DeviceContext::ValidationLevel;
  static void setVulkanValidationLevel(ValidationLevel level);
  static void setVulkanDebugUtilsExt(bool enable);
  static void setPrintSpirvReflect(bool print);

 protected:
  void mainLoop() override;

  void resizeSwapchain();

  [[nodiscard]] Vulk::DeviceContext& context() { return *_deviceContext; }
  [[nodiscard]] const Vulk::DeviceContext& context() const { return *_deviceContext; }

  // Callbacks to support creating Context
  [[nodiscard]] static VkExtent2D chooseSwapchainSurfaceExtent(const VkSurfaceCapabilitiesKHR& caps,
                                                               uint32_t windowWidth,
                                                               uint32_t windowHeight);

  [[nodiscard]] static VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats);
  [[nodiscard]] static VkPresentModeKHR chooseSwapchainPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes);

  [[nodiscard]] static VkFormat chooseDepthFormat();

  void onKeyInput(int key, int action, int mods) override;
  void onMouseMove(double xpos, double ypos) override;
  void onMouseButton(int button, int action, int mods) override;
  void onScroll(double xoffset, double yoffset) override;
  void onFramebufferResize(int width, int height) override;

 private:
  void createDeviceContext();
  void createDrawable();
  void createRenderTask();
  void createFrames();

  void nextFrame();

  void loadModel(const std::string& modelFile,
                 std::vector<Vertex>& vertices,
                 std::vector<uint32_t>& indices);
  void initCamera(const std::vector<Vertex>& vertices, bool is3D);

 private:
  Vulk::DeviceContext::shared_ptr _deviceContext;

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

  // Settings of the Testbed execution
  static ValidationLevel _validationLevel;
  static bool _debugUtilsEnabled;

  // UI control variables
  float _zoomFactor = 1.0F;

  // Input data
  std::string _modelFile{};
  std::string _textureFile{};
};
