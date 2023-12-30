#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

#include <Vulk/internal/base.h>

NAMESPACE_BEGIN(Vulk)

class Instance;

class Surface : public Sharable<Surface>, private NotCopyable {
 public:
  struct Supports {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

 public:
  Surface(const Instance& instance, VkSurfaceKHR surface);
  ~Surface() override;

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

 private:
  const Instance& instance() const { return *_instance.lock(); }

 private:
  VkSurfaceKHR _surface = VK_NULL_HANDLE;

  std::weak_ptr<const Instance> _instance;
};

NAMESPACE_END(Vulk)