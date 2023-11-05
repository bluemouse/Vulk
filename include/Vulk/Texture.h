#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/Image.h>
#include <Vulk/ImageView.h>
#include <Vulk/Sampler.h>
#include <Vulk/helpers_vulkan.h>

NAMESPACE_Vulk_BEGIN

    class Device;

class Texture {
 public:
  using Filter = Sampler::Filter;
  using AddressMode = Sampler::AddressMode;

 public:
  Texture() = default;
  Texture(const Device& device,
          VkFormat format,
          VkExtent2D extent,
          Filter filter = {VK_FILTER_LINEAR},
          AddressMode addressMode = {VK_SAMPLER_ADDRESS_MODE_REPEAT});
  ~Texture() = default;

  // Transfer the ownership from `rhs` to `this`
  Texture(Texture&& rhs) noexcept = default;
  Texture& operator=(Texture&& rhs) noexcept(false) = default;

  void create(const Device& device,
              VkFormat format,
              VkExtent2D extent,
              Filter filter = {VK_FILTER_LINEAR},
              AddressMode addressMode = {VK_SAMPLER_ADDRESS_MODE_REPEAT});
  void destroy();

  void copyFrom(const CommandBuffer& cmdBuffer, const StagingBuffer& stagingBuffer);

  const Image& image() const { return _image; }
  const ImageView& view() const { return _view; }
  const Sampler& sampler() const { return _sampler; }

  Image& image() { return _image; }
  ImageView& view() { return _view; }
  Sampler& sampler() { return _sampler; }

  // Note the input reference will lost the ownership of the data
  void setView(ImageView&& view) { _view = std::move(view); }
  void setSampler(Sampler&& sampler) { _sampler = std::move(sampler); }

  operator VkImage() const { return _image; }
  operator VkImageView() const { return _view; }
  operator VkSampler() const { return _sampler; }

  [[nodiscard]] VkImageType type() const { return _image.type(); }
  [[nodiscard]] VkFormat format() const { return _image.format(); }
  [[nodiscard]] VkExtent2D extent() const { return {width(), height()}; }
  [[nodiscard]] VkExtent3D extent3D() const { return _image.extent(); }

  [[nodiscard]] uint32_t width() const { return _image.extent().width; }
  [[nodiscard]] uint32_t height() const { return _image.extent().height; }
  [[nodiscard]] uint32_t depth() const { return _image.extent().depth; }

  [[nodiscard]] bool isCreated() const { return _image.isCreated(); }
  [[nodiscard]] bool isAllocated() const { return _image.isAllocated(); }
  [[nodiscard]] bool isMapped() const { return _image.isMapped(); }

  static VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
      uint32_t binding,
      VkShaderStageFlags stage = VK_SHADER_STAGE_ALL_GRAPHICS) {
    return {binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, stage, nullptr};
  }

 private:
  Image _image;
  ImageView _view;
  Sampler _sampler;
};

NAMESPACE_Vulk_END