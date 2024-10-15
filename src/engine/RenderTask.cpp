#include <Vulk/engine/RenderTask.h>

#include <Vulk/internal/debug.h>

MI_NAMESPACE_BEGIN(Vulk)

uint32_t RenderTask::_nextId = 1;

RenderTask::RenderTask(const DeviceContext::shared_ptr& deviceContext, Type type)
    : _deviceContext(deviceContext), _type(type), _id(_nextId++) {
}

void RenderTask::setFrameContext(const FrameContext::shared_ptr& frameContext) {
  _frameContext = frameContext;

  switch (_type)
  {
  case Type::Graphics:
    _commandBuffer = _frameContext->acquireCommandBuffer(Device::QueueFamilyType::Graphics);
    break;
  case Type::Compute:
    _commandBuffer = _frameContext->acquireCommandBuffer(Device::QueueFamilyType::Compute);
    break;
  case Type::Transfer:
    _commandBuffer = _frameContext->acquireCommandBuffer(Device::QueueFamilyType::Transfer);
    break;
  default:
    MI_LOG_ERROR("Unknown RenderTask type");
  }
}

MI_NAMESPACE_END(Vulk)