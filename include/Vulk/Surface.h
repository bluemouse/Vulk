#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include <Vulk/helpers_vulkan.h>

NAMESPACE_Vulk_BEGIN

class Instance;

class Surface {
public:
  struct Supports {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

 public:
  Surface() = default;
  Surface(const Instance& instance, VkSurfaceKHR surface);
  ~Surface();

  // TODO we should be able to create the surface using functions such as vkCreateWin32SurfaceKHR,
  //      vkCreateXcbSurfaceKHR or vkCreateAndroidSurfaceKHR. Details can be found at
  //      https://registry.khronos.org/vulkan/site/spec/latest/chapters/VK_KHR_surface/wsi.html.
  //      For now, we'll just take the surface created by the caller.
  void create(const Instance& instance, VkSurfaceKHR surface);
  void destroy();

  operator VkSurfaceKHR() const { return _surface; }

  [[nodiscard]] bool isCreated() const { return _surface != VK_NULL_HANDLE; }

  [[nodiscard]] Supports querySupports(VkPhysicalDevice physicalDevice) const;
  [[nodiscard]] Supports querySupports() const;

  [[nodiscard]] bool isAdequate(VkPhysicalDevice physicalDevice) const;

  // Disable copy and assignment operators
  Surface(const Surface&) = delete;
  Surface& operator=(const Surface&) = delete;

 private:
  VkSurfaceKHR _surface = VK_NULL_HANDLE;

  const Instance* _instance = nullptr;
};

NAMESPACE_Vulk_END