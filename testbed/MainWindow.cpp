#include "MainWindow.h"

#include <GLFW/glfw3.h>

#include <array>
#include <stdexcept>

#include <Vulk/Instance.h>
#include <Vulk/internal/debug.h>

bool MainWindow::_continuousUpdate = false;
void MainWindow::setContinuousUpdate(bool continous) {
  _continuousUpdate = continous;
}

void MainWindow::init(int width, int height) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  _width              = width;
  _height             = height;
  _window             = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
  _framebufferResized = false;

  glfwSetWindowUserPointer(_window, this);

  glfwSetKeyCallback(_window, keyCallback);
  glfwSetCursorPosCallback(_window, cursorPosCallback);
  glfwSetMouseButtonCallback(_window, mouseButtonCallback);
  glfwSetScrollCallback(_window, scrollCallback);
  glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}

void MainWindow::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int mods) {
  auto* win = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(window));
  win->onKeyInput(key, action, mods);
}

void MainWindow::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  auto* win = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(window));
  win->onMouseMove(xpos, ypos);
}

void MainWindow::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  auto* win = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(window));
  win->onMouseButton(button, action, mods);
}

void MainWindow::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  auto* win = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(window));
  win->onScroll(xoffset, yoffset);
}

void MainWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
  auto* win = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(window));
  win->onFramebufferResize(width, height);
}

void MainWindow::onKeyInput(int key, int action, int mods) {
  MI_LOG("onKeyInput: %d, %d, %d", key, action, mods);
  if (key == GLFW_KEY_Q && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
    glfwSetWindowShouldClose(_window, GLFW_TRUE);
  }
}

void MainWindow::onMouseMove(double /* xpos */, double /* ypos */) {
  // MI_LOG("onMouseMove: %f, %f", xpos, ypos);
}

void MainWindow::onMouseButton(int /* button */, int /* action */, int /* mods */) {
  // MI_LOG("onMouseButton: %d, %d, %d", button, action, mods);
}

void MainWindow::onScroll(double /* xoffset */, double /* yoffset */) {
  // MI_LOG("onScroll: %f, %f", xoffset, yoffset);
}

void MainWindow::onFramebufferResize(int width, int height) {
  // MI_LOG("onFramebufferResize: %d, %d", width, height);
  _width              = width;
  _height             = height;
  _framebufferResized = true;
}

void MainWindow::cleanup() {
  glfwDestroyWindow(_window);
  glfwTerminate();
}

void MainWindow::run() {
  mainLoop();
  cleanup();
}

void MainWindow::mainLoop() {
  while (glfwWindowShouldClose(_window) == 0) {
    if (_continuousUpdate) {
      glfwPollEvents();
    } else {
      glfwWaitEvents();
    }
    if (!isMinimized()) {
      drawFrame();
    }
  }
}

MainWindow::Extensions MainWindow::getRequiredInstanceExtensions() {
  uint32_t count          = 0;
  const char** extensions = glfwGetRequiredInstanceExtensions(&count);

  return {count, extensions};
}

VkSurfaceKHR MainWindow::createWindowSurface(const Vulk::Instance& instance) {
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(instance, _window, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("Error: glfwCreateWindowSurface failed");
  }
  return surface;
}

void MainWindow::postEmptyEvent() const {
  glfwPostEmptyEvent();
}

int MainWindow::getKeyModifier() const {
  bool shift = glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
               glfwGetKey(_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
  bool ctrl = glfwGetKey(_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
              glfwGetKey(_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
  bool alt = glfwGetKey(_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
             glfwGetKey(_window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

  int mods{0};
  if (shift) {
    mods |= GLFW_MOD_SHIFT;
  }
  if (ctrl) {
    mods |= GLFW_MOD_CONTROL;
  }
  if (alt) {
    mods |= GLFW_MOD_ALT;
  }
  return mods;
}

int MainWindow::getMouseButton() const {
  std::array<int, 3> buttons = {
      GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_MIDDLE, GLFW_MOUSE_BUTTON_RIGHT};

  for (auto button : buttons) {
    if (glfwGetMouseButton(_window, button) == GLFW_PRESS) {
      return button;
    }
  }
  return -1;
}
