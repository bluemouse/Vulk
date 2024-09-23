#include <Vulk/engine/Camera.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <cmath>
#include <iostream>

// TODO move to internal/helpers.h
#define DEFINE_OSTREAM_GLM_TYPE(type)                                   \
  std::ostream& operator<<(std::ostream& ostream, const glm::type& v) { \
    ostream << glm::to_string(v);                                       \
    return ostream;                                                     \
  }

DEFINE_OSTREAM_GLM_TYPE(vec2);
DEFINE_OSTREAM_GLM_TYPE(vec3);
DEFINE_OSTREAM_GLM_TYPE(vec4);
DEFINE_OSTREAM_GLM_TYPE(mat4);

NAMESPACE_BEGIN(Vulk)

glm::vec3 Camera::screen2view(glm::vec2 p) const {
  return ndc2view(glm::vec3(screen2ndc(p), _viewVolume.near()));
}

glm::vec3 Camera::screen2world(glm::vec2 p) const {
  return ndc2world(glm::vec3(screen2ndc(p), _viewVolume.near()));
}

glm::vec2 Camera::screen2ndc(glm::vec2 p) const {
  const float w2 = frameWidth() / 2.0F;
  const float h2 = frameHeight() / 2.0F;
  return {(p.x - w2) / w2, (p.y - h2) / h2};
}

glm::vec3 Camera::ndc2view(glm::vec3 p) const {
  auto viewPos = _invProjection * glm::vec4{p, 1.0f};
  return viewPos / viewPos.w;
}

glm::vec3 Camera::ndc2world(glm::vec3 p) const {
  return view2world(ndc2view(p));
}

glm::vec3 Camera::view2world(glm::vec3 p) const {
  return _view2World * glm::vec4{p, 1.0f};
}



void ArcCamera::init(const glm::vec2& frameSize, const BBox& roi) {
  init(frameSize, roi, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, -1.0F}, 1.0F);
}

void ArcCamera::init(const glm::vec2& frameSize,
                     const BBox& roi,
                     const glm::vec3& up,
                     const glm::vec3& eyeRay,
                     float zoomScale) {
  _roi       = roi;
  _frameSize = frameSize;

  _lookAt = _roi.center();
  _up     = up;
  // We want to place the camera at 2x radius of the roi toward the positive z axis.
  _eye = _lookAt + ((2.0F * _roi.radius()) * eyeRay);

  _zoomScale = zoomScale;

  update();
}

void ArcCamera::update() {
  _world2View = glm::lookAt(_eye, _lookAt, _up);

  // Convert _roi to view-space volume
  glm::vec3 volumeOrigin{_lookAt.x, _lookAt.y, _eye.z};
  _viewVolume.set(_roi.lower() - volumeOrigin, _roi.upper() - volumeOrigin);

  // Fit the whole view volume inside the frame
  _viewVolume.fit(_frameSize.x / _frameSize.y);
  _viewVolume.scale(1.0F / _zoomScale);

  auto near = _viewVolume.near() - _viewVolume.radius();
  auto far  = _viewVolume.far() + _viewVolume.radius() * 4;

  // Since Vulkan has the y-axis from top to bottom, we need to inverse top and bottom here.
  _projection = glm::ortho(
      _viewVolume.left(), _viewVolume.right(), _viewVolume.top(), _viewVolume.bottom(), near, far);

  _view2World    = glm::inverse(_world2View);
  _invProjection = glm::inverse(_projection);
}

void ArcCamera::update(const glm::vec2& frameSize) {
  _frameSize = frameSize;
  update();
}

void ArcCamera::move(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) {
  if (toScreenPosition == fromScreenPosition) {
    return;
  }

  auto offset = screen2view(toScreenPosition) - screen2view(fromScreenPosition);

  auto right = glm::normalize(glm::cross(_lookAt - _eye, _up));
  auto delta = (offset.x * right) + (offset.y * _up);

  _eye -= delta;
  _lookAt -= delta;
  _roi.move(-delta);

  update();
}

void ArcCamera::rotate(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) {
  if (toScreenPosition == fromScreenPosition) {
    return;
  }

  auto [axis, angle] = computeTrackballRotation(fromScreenPosition, toScreenPosition);
  orbit(axis, angle * 2.0F);
}

void ArcCamera::orbit(const glm::vec3& axis, float angle) {
  if (angle == 0.0F || axis == glm::vec3{0.0F}) {
    return;
  }

  auto rotation = glm::rotate(glm::mat4{1}, angle, axis);
  auto up0Pos   = _up + (_eye - _lookAt);
  auto eye0     = glm::vec3{rotation * glm::vec4{_eye - _lookAt, 1.0f}};
  auto up       = glm::vec3{rotation * glm::vec4{up0Pos, 1.0f}} - eye0;
  _eye          = eye0 + _lookAt;

  auto ray   = _lookAt - _eye;
  auto right = glm::cross(ray, up);
  _up = glm::normalize(glm::cross(right, ray)); // make sure up is orthogonal to ray direction

  update();
}

void ArcCamera::orbitHorizontal(float angle) {
  orbit(_up, angle);
}

void ArcCamera::orbitVertical(float angle) {
  orbit(glm::cross(_up, _lookAt - _eye), angle);
}

void ArcCamera::zoom(float scale) {
  _zoomScale = scale;
  update();
}

auto ArcCamera::computeTrackballRotation(const glm::vec2& screenFrom,
                                         const glm::vec2& screenTo) const -> Rotation {
  const auto from = trackballPoint(screen2ndc(screenFrom));
  const auto to   = trackballPoint(screen2ndc(screenTo));

  if (from == to) {
    return {glm::vec3{0.0F, 0.0F, 0.0F}, 0.0F};
  }

  auto angle = std::acos(std::min(1.0F, glm::dot(from, to)));

  const auto worldOrig = ndc2world({0.0F, 0.0F, 0.0F});
  const auto worldFrom = ndc2world(from) - worldOrig;
  const auto worldTo   = ndc2world(to) - worldOrig;
  auto axis            = glm::normalize(glm::cross(worldFrom, worldTo));

  return {axis, angle};
}

glm::vec3 ArcCamera::trackballPoint(glm::vec2 ndcPos) const {
  float z  = 0.0F;
  float d2 = glm::dot(ndcPos, ndcPos);
  if (d2 <= 1.0F) {
    z = std::sqrt(1.0F - d2);
  }

  return glm::vec3{ndcPos, z};
}

void FlatCamera::init(const glm::vec2& frameSize, const BBox& roi) {
  init(frameSize, roi, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, -1.0F}, 1.0F);
}

void FlatCamera::init(const glm::vec2& frameSize,
                      const BBox& roi,
                      const glm::vec3& up,
                      const glm::vec3& eyeRay,
                      float zoomScale) {
  _roi       = roi;
  _frameSize = frameSize;

  _lookAt = _roi.center();
  _up     = up;
  // We want to place the camera at 2x radius of the roi toward the positive z axis.
  _eye = _lookAt + ((2.0F * _roi.radius()) * eyeRay);

  _zoomScale = zoomScale;

  update();
}

void FlatCamera::update() {
  _world2View = glm::lookAt(_eye, _lookAt, _up);

  // Convert _roi to view-space volume
  glm::vec3 volumeOrigin{_lookAt.x, _lookAt.y, _eye.z};
  _viewVolume.set(_roi.lower() - volumeOrigin, _roi.upper() - volumeOrigin);

  // Fit the whole view volume inside the frame
  _viewVolume.fit(_frameSize.x / _frameSize.y);
  _viewVolume.scale(1.0F / _zoomScale);

  auto near = _viewVolume.near() - _viewVolume.radius();
  auto far  = _viewVolume.far() + _viewVolume.radius() * 4;

  // Since Vulkan has the y-axis from top to bottom, we need to inverse top and bottom here.
  _projection = glm::ortho(
      _viewVolume.left(), _viewVolume.right(), _viewVolume.top(), _viewVolume.bottom(), near, far);

  _view2World    = glm::inverse(_world2View);
  _invProjection = glm::inverse(_projection);
}

void FlatCamera::update(const glm::vec2& frameSize) {
  _frameSize = frameSize;
  update();
}

void FlatCamera::move(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) {
  if (toScreenPosition == fromScreenPosition) {
    return;
  }

  auto offset = screen2view(toScreenPosition) - screen2view(fromScreenPosition);

  auto right = glm::normalize(glm::cross(_lookAt - _eye, _up));
  auto delta = (offset.x * right) + (offset.y * _up);

  _eye -= delta;
  _lookAt -= delta;
  _roi.move(-delta);

  update();
}

void FlatCamera::zoom(float scale) {
  _zoomScale = scale;
  update();
}

NAMESPACE_END(Vulk)