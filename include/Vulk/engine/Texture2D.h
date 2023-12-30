#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/internal/base.h>

#include <Vulk/Image2D.h>
#include <Vulk/ImageView.h>
#include <Vulk/Sampler.h>

NAMESPACE_BEGIN(Vulk)

class Device;

class Texture2D : public Sharable<Texture2D>, private NotCopyable {
 public:
  using Filter      = Sampler::Filter;
  using AddressMode = Sampler::AddressMode;

 public:
  Texture2D() = default;
  Texture2D(const Device& device,
            VkFormat format,
            VkExtent2D extent,
            VkImageUsageFlags usage,
            Filter filter           = {VK_FILTER_LINEAR},
            AddressMode addressMode = {VK_SAMPLER_ADDRESS_MODE_REPEAT});
  ~Texture2D() override = default;

  void create(const Device& device,
              VkFormat format,
              VkExtent2D extent,
              VkImageUsageFlags usage,
              Filter filter           = {VK_FILTER_LINEAR},
              AddressMode addressMode = {VK_SAMPLER_ADDRESS_MODE_REPEAT});
  void destroy();

  void copyFrom(const CommandBuffer& cmdBuffer, const StagingBuffer& stagingBuffer);
  void copyFrom(const CommandBuffer& cmdBuffer, const Image2D& srcImage);
  void copyFrom(const CommandBuffer& cmdBuffer, const Texture2D& srcTexture);
  void blitFrom(const CommandBuffer& cmdBuffer, const Image2D& srcImage);
  void blitFrom(const CommandBuffer& cmdBuffer, const Texture2D& srcTexture);

  const Image2D& image() const { return *_image; }
  const ImageView& view() const { return *_view; }
  const Sampler& sampler() const { return *_sampler; }

  Image2D& image() { return *_image; }
  ImageView& view() { return *_view; }
  Sampler& sampler() { return *_sampler; }

  // Note the input reference will lost the ownership of the data
  void setView(ImageView::shared_ptr view) { _view = view; }
  void setSampler(Sampler::shared_ptr sampler) { _sampler = sampler; }

  operator VkImage() const { return *_image; }
  operator VkImageView() const { return *_view; }
  operator VkSampler() const { return *_sampler; }

  [[nodiscard]] VkImageType type() const { return _image->type(); }
  [[nodiscard]] VkFormat format() const { return _image->format(); }
  [[nodiscard]] VkExtent2D extent() const { return _image->extent(); }

  [[nodiscard]] uint32_t width() const { return _image->extent().width; }
  [[nodiscard]] uint32_t height() const { return _image->extent().height; }

  [[nodiscard]] bool isCreated() const { return _image->isCreated(); }
  [[nodiscard]] bool isAllocated() const { return _image->isAllocated(); }
  [[nodiscard]] bool isMapped() const { return _image->isMapped(); }

  [[nodiscard]] bool isValid() const { return _image->isCreated(); }

  static VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
      uint32_t binding,
      VkShaderStageFlags stage = VK_SHADER_STAGE_ALL_GRAPHICS) {
    return {binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, stage, nullptr};
  }

 private:
  Image2D::shared_ptr _image;
  ImageView::shared_ptr _view;
  Sampler::shared_ptr _sampler;
};

NAMESPACE_END(Vulk)