#pragma once

#include <Vulk/Device.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/VertexBuffer.h>
#include <Vulk/IndexBuffer.h>
#include <Vulk/StagingBuffer.h>

#include <glm/glm.hpp>
#include <vector>

NAMESPACE_VULKAN_BEGIN

template <typename Vertex, typename Index>
class Drawable {
 public:
  using vertex_type = Vertex;
  using index_type = Index;

 public:
  Drawable() = default;

  void create(const Vulkan::Device& device,
              const Vulkan::CommandBuffer& commandBuffer,
              std::vector<vertex_type> vertices,
              std::vector<index_type> indices);
  void destroy();

  [[nodiscard]] const Vulkan::VertexBuffer& vertexBuffer() const { return _vertexBuffer; }
  [[nodiscard]] const Vulkan::IndexBuffer& indexBuffer() const { return _indexBuffer; }

  [[nodiscard]] size_t numIndices() const { return _numIndices; }

 private:
  Vulkan::VertexBuffer _vertexBuffer;
  Vulkan::IndexBuffer _indexBuffer;

  size_t _numIndices = 0;
};

template <typename V, typename I>
inline void Drawable<V, I>::create(const Vulkan::Device& device,
                             const Vulkan::CommandBuffer& commandBuffer,
                             std::vector<vertex_type> vertices,
                             std::vector<index_type> indices) {
  { // TODO Create Buffer::loadFromHost() to simplify this
    VkDeviceSize bufferSize = sizeof(vertex_type) * vertices.size();
    Vulkan::StagingBuffer stagingBuffer(device, bufferSize);
    stagingBuffer.copyFromHost(vertices.data(), bufferSize);
    _vertexBuffer.create(device, bufferSize);
    stagingBuffer.copyToBuffer(commandBuffer, _vertexBuffer, bufferSize);
  }
  { // TODO Create Buffer::loadFromHost() to simplify this
    VkDeviceSize bufferSize = sizeof(index_type) * indices.size();
    Vulkan::StagingBuffer stagingBuffer(device, bufferSize);
    stagingBuffer.copyFromHost(indices.data(), bufferSize);
    _indexBuffer.create(device, bufferSize);
    stagingBuffer.copyToBuffer(commandBuffer, _indexBuffer, bufferSize);
  }
  _numIndices = indices.size();
}

template <typename V, typename I>
inline void Drawable<V, I>::destroy() {
  _vertexBuffer.destroy();
  _indexBuffer.destroy();

  _numIndices = 0;
}

NAMESPACE_VULKAN_END