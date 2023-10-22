#pragma once

#include <vulkan/vulkan.h>

#include <Vulk/helpers_vulkan.h>

#include <functional>

NAMESPACE_Vulk_BEGIN

class Device;

class Sampler {
 public:
  struct AddressMode {
    VkSamplerAddressMode u;
    VkSamplerAddressMode v;
    VkSamplerAddressMode w;

    AddressMode(VkSamplerAddressMode mode) : u(mode), v(mode), w(mode) {}
    AddressMode(VkSamplerAddressMode u,
                VkSamplerAddressMode v,
                VkSamplerAddressMode w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
        : u(u), v(v), w(w) {}
  };
  struct Filter {
    VkFilter mag;
    VkFilter min;

    Filter(VkFilter filter) : mag(filter), min(filter) {}
    Filter(VkFilter mag, VkFilter min) : mag(mag), min(min) {}
  };

  using SamplerCreateInfoOverride = std::function<void(VkSamplerCreateInfo*)>;

 public:
  Sampler() = default;
  Sampler(const Device& device,
          Filter filter = {VK_FILTER_LINEAR},
          AddressMode addressMode = {VK_SAMPLER_ADDRESS_MODE_REPEAT},
          const SamplerCreateInfoOverride& createInfoOverride = {});
  ~Sampler();

  // Transfer the ownership from `rhs` to `this`
  Sampler(Sampler&& rhs) noexcept;
  Sampler& operator=(Sampler&& rhs) noexcept(false);

  void create(const Device& device,
              Filter filter = {VK_FILTER_LINEAR},
              AddressMode addressMode = {VK_SAMPLER_ADDRESS_MODE_REPEAT},
              const SamplerCreateInfoOverride& createInfoOverride = {});

  void destroy();

  operator VkSampler() const { return _sampler; }

  [[nodiscard]] bool isCreated() const { return _sampler != VK_NULL_HANDLE; }

 private:
  void moveFrom(Sampler& rhs);

 private:
  VkSampler _sampler = VK_NULL_HANDLE;

  const Device* _device = nullptr;
};

NAMESPACE_Vulk_END