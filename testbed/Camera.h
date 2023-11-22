#pragma once

#include "Bound.h"

#include <glm/glm.hpp>

#include <limits>

class Camera {
 public:
  using BBox = Bound<glm::vec3>;

 public:
  Camera() { init(); }

  // Camera is located at `position` and looking at center of the front face of`bbox` with the up
  // vector of `bbox`
  void init(const BBox& bbox, const glm::vec2& frameSize);
  void reset();
  void update(const glm::vec2& frameSize);
  void update();

  void move(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition);
  void rotate(const glm::vec2& fromScreenPosition, const glm::vec2& toScreenPosition);
  void zoom(float scale);

  [[nodiscard]] glm::mat4 mvpMatrix() const { return _mvp; }
  [[nodiscard]] glm::mat4 modelMatrix() const { return _model2World; }
  [[nodiscard]] glm::mat4 viewMatrix() const { return _world2View; }
  [[nodiscard]] glm::mat4 projectionMatrix() const { return _projection; }

  [[nodiscard]] float frameWidth() const { return _frameSize.x; }
  [[nodiscard]] float frameHeight() const { return _frameSize.y; }

  // Coordinate conversion functions
  [[nodiscard]] glm::vec3 screen2view(glm::vec2 p) const;
  [[nodiscard]] glm::vec3 screen2world(glm::vec2 p) const;
  [[nodiscard]] glm::vec3 screen2model(glm::vec2 p) const;
  [[nodiscard]] glm::vec2 screen2ndc(glm::vec2 p) const;
  [[nodiscard]] glm::vec3 ndc2view(glm::vec3 p) const;
  [[nodiscard]] glm::vec3 ndc2world(glm::vec3 p) const;
  [[nodiscard]] glm::vec3 view2world(glm::vec3 p) const;
  [[nodiscard]] glm::vec3 world2model(glm::vec3 p) const;

 private:
  void init() { init(BBox::unit(), glm::vec2{1.0f}); }

  void setFrameSize(glm::vec2 size) { _frameSize = size; }

  // void updateTransformation() { updateTransformation({frameWidth(), frameHeight()}); }
  void updateMVP() { _mvp = _projection * _world2View * _model2World; }

  [[nodiscard]] glm::vec3 trackballPoint(const glm::vec2& screenPos) const;

 private:
  glm::vec3 _eye;
  glm::vec3 _lookAt;
  glm::vec3 _up;

  BBox _bbox;

  struct ViewVolume {
    float left{-1.0F};
    float right{1.0F};
    float top{-1.0F};
    float bottom{1.0F};
    float near{0.0F};
    float far{1.0F};
  };
  ViewVolume _viewVolume;

  float _zoomScale{1.0F};
  glm::vec2 _panOffset = {0.0F, 0.0F};

  glm::vec2 _frameSize;

  glm::mat4 _model2World, _world2Model;
  glm::mat4 _world2View, _view2World;
  glm::mat4 _projection, _invProjection;
  glm::mat4 _mvp;

  static constexpr float ZOOM_OUT_STEP = 0.09f;
  static constexpr float ZOOM_IN_STEP  = 0.25f;
  static constexpr float MAX_ZOOMSCALE = 16.0f;
  static constexpr float MIN_ZOOMSCALE = 0.1f;
};
