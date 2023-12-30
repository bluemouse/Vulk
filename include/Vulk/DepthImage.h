#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/base.h>

#include <Vulk/Image.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class DepthImage : public Image {
 public:
  using ImageCreateInfoOverride = std::function<void(VkImageCreateInfo*)>;

 public:
  DepthImage() = default;
  DepthImage(const Device& device,
             VkExtent2D extent,
             uint32_t depthBits,
             uint32_t stencilBits                    = 0,
             const ImageCreateInfoOverride& override = {});
  DepthImage(const Device& device,
             VkExtent2D extent,
             VkFormat format,
             const ImageCreateInfoOverride& override = {});

  ~DepthImage() override = default;

  void create(const Device& device,
              VkExtent2D extent,
              uint32_t depthBits,
              uint32_t stencilBits                    = 0,
              const ImageCreateInfoOverride& override = {});
  void create(const Device& device,
              VkExtent2D extent,
              VkFormat format,
              const ImageCreateInfoOverride& override = {});

  using Image::destroy;
  using Image::allocate;
  using Image::free;

  void promoteLayout(const CommandBuffer& cmdBuffer, bool waitForFinish);

  [[nodiscard]] VkExtent2D extent() const { return {_extent.width, _extent.height}; }

  using Image::isCreated;
  using Image::isAllocated;

  operator VkImage() const { return Image::operator VkImage(); }

  [[nodiscard]] bool hasDepthBits() const;
  [[nodiscard]] bool hasStencilBits() const;

  [[nodiscard]] static VkFormat findFormat(uint32_t depthBits, uint32_t stencilBits);

  //
  // Override the sharable types and functions
  //
  using shared_ptr = std::shared_ptr<DepthImage>;
  using weak_ptr   = std::weak_ptr<DepthImage>;

  template <class... Args>
  static shared_ptr make_shared(Args&&... args) {
    return std::make_shared<DepthImage>(std::forward<Args>(args)...);
  }

  shared_ptr get_shared() { return std::static_pointer_cast<DepthImage>(Image::get_shared()); }
  weak_ptr get_weak() { return std::static_pointer_cast<DepthImage>(Image::get_weak().lock()); }

 private:
  VkFormat _format = VK_FORMAT_UNDEFINED;
};

NAMESPACE_END(Vulk)