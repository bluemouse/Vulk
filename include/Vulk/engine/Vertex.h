#pragma once

#include <Vulk/engine/TypeTraits.h>

#include <vector>

MI_NAMESPACE_BEGIN(Vulk)

template <typename Position, typename Color, typename TexCoord>
struct Vertex {
  using position_type = Position;
  using color_type    = Color;
  using texcoord_type = TexCoord;

  position_type pos;
  color_type color;
  texcoord_type texCoord;

  bool operator==(const Vertex& rhs) const {
    return pos == rhs.pos && color == rhs.color && texCoord == rhs.texCoord;
  }

  static VkVertexInputBindingDescription bindingDescription(uint32_t binding) {
    return {binding, Vertex::size(), VK_VERTEX_INPUT_RATE_VERTEX};
  }
  static std::vector<VkVertexInputAttributeDescription> attributesDescription(uint32_t binding) {
    // In shader.vert, we should have:
    // layout(location = 0) in vec3 inPos;
    // layout(location = 1) in vec3 inColor;
    // layout(location = 2) in vec2 inTexCoord;
    return {{0, binding, formatof(Vertex::pos), offsetof(Vertex, pos)},
            {1, binding, formatof(Vertex::color), offsetof(Vertex, color)},
            {2, binding, formatof(Vertex::texCoord), offsetof(Vertex, texCoord)}};
  }
  static constexpr uint32_t size() { return sizeof(Vertex); }
};

MI_NAMESPACE_END(Vulk)