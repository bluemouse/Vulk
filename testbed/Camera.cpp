#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>

#define DEFINE_OSTREAM_GLM_TYPE(type)                                   \
  std::ostream& operator<<(std::ostream& ostream, const glm::type& v) { \
    ostream << glm::to_string(v);                                       \
    return ostream;                                                     \
  }

DEFINE_OSTREAM_GLM_TYPE(vec2);
DEFINE_OSTREAM_GLM_TYPE(vec3);
DEFINE_OSTREAM_GLM_TYPE(vec4);
DEFINE_OSTREAM_GLM_TYPE(mat4);

void Camera::init(const BBox& bbox, const glm::vec2& frameSize) {
  _bbox      = bbox;
  _frameSize = frameSize;

  BBox::Face nearZ = _bbox.nearZ();

  _lookAt = _bbox.center();
  _up     = nearZ.up();

  auto center = nearZ.center();
  // We use 45 degree diagonal fov to calculate the distance to eye
  auto offsetToEye =
      (glm::distance(_lookAt, center) + glm::distance(center, nearZ.v[0])) * nearZ.normal();
  _eye = _lookAt + offsetToEye;

  _zoomScale = 1.0F;
  _panOffset = {0.0F, 0.0F};
}

void Camera::update() {
  // std::cout << "---------" << '\n';
  // std::cout << "eye = " << _eye << '\n';
  // std::cout << "lookAt = " << _lookAt << '\n';
  // std::cout << "up = " << _up << '\n';
  // std::cout << "bbox = " << _bbox.lower() << '\n' << "          " << _bbox.upper() <<
  // '\n';

  _model2World = glm::mat4{1.0f};
  // std::cout << "model2World = " << _model2World << std::endl;

  _world2View = glm::lookAt(_eye, _lookAt, _up);
  // std::cout << "world2View = " << _world2View << '\n';

  auto activeFrustum = _bbox;
  activeFrustum.fit(_frameSize.x / _frameSize.y);
  activeFrustum.scale(1.0F / _zoomScale);

  _viewVolume.right  = activeFrustum.upper().x - _lookAt.x;
  _viewVolume.left   = activeFrustum.lower().x - _lookAt.x;
  _viewVolume.top    = activeFrustum.lower().y - _lookAt.y;
  _viewVolume.bottom = activeFrustum.upper().y - _lookAt.y;

  constexpr float margin = 1.0F;
  _viewVolume.near       = (activeFrustum.lower().z - margin) - _eye.z;
  _viewVolume.far        = (activeFrustum.upper().z + margin) - _eye.z;

  _projection = glm::ortho(_viewVolume.left,
                           _viewVolume.right,
                           _viewVolume.bottom,
                           _viewVolume.top,
                           _viewVolume.near,
                           _viewVolume.far);
  // std::cout << "projection = " << _projection << '\n';

  _world2Model   = glm::inverse(_model2World);
  _view2World    = glm::inverse(_world2View);
  _invProjection = glm::inverse(_projection);

  // updateMVP();
}

void Camera::update(const glm::vec2& frameSize) {
  _frameSize = frameSize;
  update();
}

glm::vec3 Camera::screen2view(glm::vec2 p) const {
  return ndc2view(glm::vec3(screen2ndc(p), _viewVolume.near));
}

glm::vec3 Camera::screen2world(glm::vec2 p) const {
  return ndc2world(glm::vec3(screen2ndc(p), _viewVolume.near));
}

glm::vec3 Camera::screen2model(glm::vec2 p) const {
  return world2model(screen2world(p));
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

glm::vec3 Camera::world2model(glm::vec3 p) const {
  return _world2Model * glm::vec4{p, 1.0f};
}

void Camera::move(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) {
  auto from = screen2world(fromScreenPosition);
  auto to   = screen2world(toScreenPosition);
  glm::vec2 distance{from.x - to.x, from.y - to.y};

  auto ray   = normalize(_lookAt - _eye);
  auto up    = normalize(_up);
  auto right = normalize(cross(up, ray));

  auto delta = (distance.x * right) + (distance.y * up);

  _eye -= delta;
  _lookAt -= delta;

  _bbox.move(-delta);

  update();
}

void Camera::rotate(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition) {
  using namespace glm;

  auto from = fromScreenPosition;
  auto to   = toScreenPosition;
  if (to != from) {
    auto va     = trackballPoint(from);
    auto vb     = trackballPoint(to);
    float angle = std::acos(std::min(1.0f, dot(va, vb))) * 2;
    auto axis   = vec3{ndc2world(cross(va, vb))};

    auto rotation = glm::rotate(mat4{1}, angle, axis - _lookAt);
    auto up0Pos   = _up + (_eye - _lookAt);
    auto eye0     = vec3{rotation * vec4{_eye - _lookAt, 1.0f}};
    auto up       = vec3{rotation * vec4{up0Pos, 1.0f}} - eye0;
    _eye          = eye0 + _lookAt;
    auto ray      = _lookAt - _eye;
    auto right    = cross(ray, up);
    _up           = normalize(cross(right, ray)); // make sure up is orthogonal to ray direction

    update();
  }
}

void Camera::zoom(float scale) {
  _zoomScale = scale;
  update();
}

glm::vec3 Camera::trackballPoint(const glm::vec2& screenPos) const {
  glm::vec3 p = {screen2ndc(screenPos), 0};
  float d2    = p.x * p.x + p.y * p.y;
  if (d2 <= 1)
    p.z = sqrtf(1 - d2);
  else
    p = glm::normalize(p);
  return p;
}
