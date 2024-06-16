#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/base.h>

#include <vector>

NAMESPACE_BEGIN(Vulk)

class Instance;

NAMESPACE_END(Vulk)

class GLFWwindow;

class MainWindow {
 public:
  MainWindow()                          = default;
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

  void postEmptyEvent() const;

  [[nodiscard]] bool isMinimized() const { return _width == 0 || _height == 0; }

  static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
  static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
  static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

  virtual void onKeyInput(int key, int action, int mods);
  virtual void onMouseMove(double xpos, double ypos);
  virtual void onMouseButton(int button, int action, int mods);
  virtual void onScroll(double xoffset, double yoffset);
  virtual void onFramebufferResize(int width, int height);

  void setFramebufferResized(bool resized) { _framebufferResized = resized; }
  [[nodiscard]] bool isFramebufferResized() const { return _framebufferResized; }

  [[nodiscard]] static std::vector<const char*> getRequiredInstanceExtensions();

  [[nodiscard]] VkSurfaceKHR createWindowSurface(const Vulk::Instance& instance);

  int getKeyModifier() const;
  int getMouseButton() const;

 private:
  GLFWwindow* _window = nullptr;

  int _width  = 0;
  int _height = 0;

  bool _framebufferResized = false;

  // Settings of the MainWindow
  static bool _continuousUpdate;
};
