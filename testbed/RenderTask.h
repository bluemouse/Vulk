#pragma once

#include <Vulk/engine/Context.h>
#include <Vulk/engine/Texture2D.h>

#include <Vulk/RenderPass.h>
#include <Vulk/Pipeline.h>
#include <Vulk/DescriptorPool.h>
#include <Vulk/UniformBuffer.h>
#include <Vulk/DescriptorSet.h>
#include <Vulk/CommandBuffer.h>
#include <Vulk/Framebuffer.h>
#include <Vulk/Semaphore.h>
#include <Vulk/Fence.h>
#include <Vulk/DepthImage.h>


//
//
//
class RenderTask {
 public:
  explicit RenderTask(const Vulk::Context& context);
  virtual ~RenderTask() {};

  virtual void prepare() = 0;
  virtual void render() = 0;

 protected:
  const Vulk::Context& _context;
};


//
//
//
class TextureMappingTask : public RenderTask {
 public:
  TextureMappingTask(const Vulk::Context& context);
  ~TextureMappingTask() override;

  void prepare() override;
  void render() override;

 private:
  void createFrames();

 private:
};

//
//
//
class PresentTask : public RenderTask {
 public:
  PresentTask(const Vulk::Context& context);
  ~PresentTask() override;

  void prepare() override;
  void render() override;
};