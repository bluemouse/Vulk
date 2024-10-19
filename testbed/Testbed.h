#pragma once

#include "MainWindow.h"

#include <Vulk/engine/DeviceContext.h>

#include "RenderModule.h"

#include <filesystem>

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

  void onKeyInput(int key, int action, int mods) override;
  void onMouseMove(double xpos, double ypos) override;
  void onMouseButton(int button, int action, int mods) override;
  void onScroll(double xoffset, double yoffset) override;
  void onFramebufferResize(int width, int height) override;

 private:
  void createDeviceContext();

  void nextFrame();

  void loadModel(const std::string& modelFile,
                 std::vector<Vertex>& vertices,
                 std::vector<uint32_t>& indices);
  void initCamera(const std::vector<Vertex>& vertices, bool is3D);

 private:
  Vulk::DeviceContext::shared_ptr _deviceContext;

  // Settings of the Testbed execution
  static ValidationLevel _validationLevel;
  static bool _debugUtilsEnabled;

  std::shared_ptr<RenderModule> _renderModule;

  // UI control variables
  float _zoomFactor = 1.0F;

  // Input data
  std::filesystem::path _modelFile{};
  std::filesystem::path _textureFile{};
};
