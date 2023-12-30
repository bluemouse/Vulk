#include <Vulk/Sampler.h>

#include <Vulk/internal/vulkan_debug.h>

#include <Vulk/Device.h>
#include <Vulk/PhysicalDevice.h>

NAMESPACE_BEGIN(Vulk)

Sampler::Sampler(const Device& device,
                 Filter filter,
                 AddressMode addressMode,
                 const SamplerCreateInfoOverride& createInfoOverride) {
  create(device, filter, addressMode, createInfoOverride);
}

Sampler::~Sampler() {
  if (isCreated()) {
    destroy();
  }
}

void Sampler::create(const Device& device,
                     Filter filter,
                     AddressMode addressMode,
                     const SamplerCreateInfoOverride& createInfoOverride) {
  MI_VERIFY(!isCreated());
  _device = device.get_weak();

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device.physicalDevice(), &properties);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter               = filter.mag;
  samplerInfo.minFilter               = filter.min;
  samplerInfo.addressModeU            = addressMode.u;
  samplerInfo.addressModeV            = addressMode.v;
  samplerInfo.addressModeW            = addressMode.w;
  samplerInfo.anisotropyEnable        = VK_TRUE;
  samplerInfo.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE;
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if (createInfoOverride) {
    createInfoOverride(&samplerInfo);
  }

  MI_VERIFY_VKCMD(vkCreateSampler(device, &samplerInfo, nullptr, &_sampler));
}

void Sampler::destroy() {
  MI_VERIFY(isCreated());
  vkDestroySampler(device(), _sampler, nullptr);

  _sampler = VK_NULL_HANDLE;
  _device.reset();
}

NAMESPACE_END(Vulk)
