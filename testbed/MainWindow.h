#pragma once

#include <vulkan/vulkan.h>

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

  [[nodiscard]] VkSurfaceKHR createWindowSurface(VkInstance instance);

 private:
  GLFWwindow* _window = nullptr;

  int _width = 0;
  int _height = 0;

  bool _framebufferResized = false;
};
