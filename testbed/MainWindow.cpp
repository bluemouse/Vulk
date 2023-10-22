#include "MainWindow.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <stdexcept>

#include <Vulk/Instance.h>

void MainWindow::init(int width, int height) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  _width = width;
  _height = height;
  _window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
  _framebufferResized = false;

  glfwSetWindowUserPointer(_window, this);
  glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}

void MainWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
  auto win = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(window));

  win->_width = width;
  win->_height = height;
  win->_framebufferResized = true;
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
    glfwPollEvents();
    if (!isMinimized()) {
      drawFrame();
    }
  }
}

MainWindow::Extensions MainWindow::getRequiredInstanceExtensions() {
  uint32_t count = 0;
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