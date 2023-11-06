#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/helpers_vulkan.h>

NAMESPACE_Vulk_BEGIN
class Instance;
NAMESPACE_Vulk_END

class GLFWwindow;

class MainWindow {
 public:
  MainWindow() = default;
  virtual ~MainWindow() noexcept(false) = default;

  virtual void init(int width, int height);
  virtual void cleanup();

  virtual void run();

  [[nodiscard]] GLFWwindow* window() const { return _window; }
  [[nodiscard]] int width() const { return _width; }
  [[nodiscard]] int height() const { return _height; }

  // Settings of the MainWindow execution
  static void setContinuousUpdate(bool continous);

 protected:
  virtual void mainLoop();
  virtual void drawFrame() = 0;

  [[nodiscard]] bool isMinimized() const { return _width == 0 || _height == 0; }

  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

  void setFramebufferResized(bool resized) { _framebufferResized = resized; }
  [[nodiscard]] bool isFramebufferResized() const { return _framebufferResized; }

  struct Extensions {
    unsigned int count;
    const char** extensions;
  };
  [[nodiscard]] static Extensions getRequiredInstanceExtensions() ;

  [[nodiscard]] VkSurfaceKHR createWindowSurface(const Vulk::Instance& instance);

 private:
  GLFWwindow* _window = nullptr;

  int _width = 0;
  int _height = 0;

  bool _framebufferResized = false;

  // Settings of the MainWindow
  static bool _continuousUpdate;
};
