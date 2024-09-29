#pragma once

#include <volk/volk.h>

#include <functional>
#include <memory>

#include <Vulk/internal/base.h>

MI_NAMESPACE_BEGIN(Vulk)

class Device;

class Sampler : public Sharable<Sampler>, private NotCopyable {
 public:
  struct Filter {
    VkFilter mag;
    VkFilter min;

    Filter(VkFilter filter) : mag(filter), min(filter) {}
    Filter(VkFilter mag, VkFilter min) : mag(mag), min(min) {}
  };

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

  using SamplerCreateInfoOverride = std::function<void(VkSamplerCreateInfo*)>;

 public:
  Sampler() = default;
  Sampler(const Device& device,
          Filter filter                                       = {VK_FILTER_LINEAR},
          AddressMode addressMode                             = {VK_SAMPLER_ADDRESS_MODE_REPEAT},
          const SamplerCreateInfoOverride& createInfoOverride = {});
  ~Sampler();

  void create(const Device& device,
              Filter filter           = {VK_FILTER_LINEAR},
              AddressMode addressMode = {VK_SAMPLER_ADDRESS_MODE_REPEAT},
              const SamplerCreateInfoOverride& createInfoOverride = {});

  void destroy();

  operator VkSampler() const { return _sampler; }

  [[nodiscard]] bool isCreated() const { return _sampler != VK_NULL_HANDLE; }

  [[nodiscard]] const Device& device() const { return *_device.lock(); }

 private:
  VkSampler _sampler = VK_NULL_HANDLE;

  std::weak_ptr<const Device> _device;
};

MI_NAMESPACE_END(Vulk)