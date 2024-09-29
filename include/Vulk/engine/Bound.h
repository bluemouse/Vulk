#pragma once

#include <Vulk/internal/base.h>

#include <glm/glm.hpp>
#include <array>

MI_NAMESPACE_BEGIN(Vulk)

template <typename T>
class Bound {
 public:
  using vector_type = T;
  using value_type  = typename T::value_type;

  struct Face {
    std::array<T, 4> v;

    Face() = default;
    Face(const T& a, const T& b, const T& c, const T& d) : v{a, b, c, d} {}

    [[nodiscard]] T normal() const { return glm::normalize(glm::cross(v[2] - v[1], v[0] - v[1])); }
    // center of the face
    [[nodiscard]] T center() const { return (v[0] + v[1] + v[2] + v[3]) / 4.0f; }
    // up vector of the face
    [[nodiscard]] T up() const { return glm::normalize(v[0] - v[1]); }
    value_type diagonal() const { return glm::distance(v[0], v[2]); }
  };

 public:
  Bound() = default;
  Bound(const T& lower, const T& upper) : _lower{lower}, _upper{upper} {}
  Bound(const T& center, value_type radius) : _lower{center - radius}, _upper{center + radius} {}

  static Bound infinite() { return Bound{min(), max()}; }
  static Bound null() { return Bound{max(), min()}; }
  static Bound unit() { return Bound{T{-1}, T{1}}; }

  [[nodiscard]] const T& lower() const { return _lower; }
  [[nodiscard]] const T& upper() const { return _upper; }

  void setLower(const T& lower) { set(lower, _upper); }
  void setUpper(const T& upper) { set(_lower, upper); }

  void set(const T& lower, const T& upper);

  Bound operator+(const T& point) const;
  void operator+=(const T& point);

  Bound operator&(const Bound& rhs) const;
  void operator&=(const Bound& rhs);

  [[nodiscard]] T center() const { return (_lower + _upper) / 2.0f; }
  [[nodiscard]] T extent() const { return _upper - _lower; };
  // The radius of the sphere that encompasses the bound
  [[nodiscard]] value_type radius() const { return glm::length(extent()) / 2; }

  [[nodiscard]] Face nearZ() const;
  [[nodiscard]] Face farZ() const;

  void scale(value_type multiplier);
  // aspect = width / height
  void fit(value_type aspect);
  void move(const T& offset);

  // Expand the planar side of the bound by `padding` if the bound is not a volume.
  void expandPlanarSide(value_type padding);

  [[nodiscard]] value_type left() const { return _lower.x; }
  [[nodiscard]] value_type right() const { return _upper.x; }
  [[nodiscard]] value_type bottom() const { return _lower.y; }
  [[nodiscard]] value_type top() const { return _upper.y; }
  [[nodiscard]] value_type near() const { return _lower.z; }
  [[nodiscard]] value_type far() const { return _upper.z; }

  bool isValid() const { return glm::all(glm::lessThan(_lower, _upper)); }

 protected:
  T _lower{};
  T _upper{};

 private:
  static constexpr T max() { return T{std::numeric_limits<value_type>::max()}; }
  static constexpr T min() { return T{std::numeric_limits<value_type>::lowest()}; }
};

template <typename T>
void Bound<T>::set(const T& lower, const T& upper) {
  _lower = lower;
  _upper = upper;
}

template <typename T>
auto Bound<T>::operator+(const T& point) const -> Bound {
  return Bound{glm::min(point, _lower), glm::max(point, _upper)};
}

template <typename T>
void Bound<T>::operator+=(const T& point) {
  _lower = glm::min(point, _lower);
  _upper = glm::max(point, _upper);
}

template <typename T>
auto Bound<T>::operator&(const Bound& rhs) const -> Bound {
  return Bound{glm::max(_lower, rhs._lower), glm::min(_upper, rhs._upper)};
}

template <typename T>
void Bound<T>::operator&=(const Bound& rhs) {
  _lower = glm::max(_lower, rhs._lower);
  _upper = glm::min(_upper, rhs._upper);
}

template <typename T>
auto Bound<T>::nearZ() const -> Face {
  return Face{_lower,
              T{_lower.x, _upper.y, _lower.z},
              T{_upper.x, _upper.y, _lower.z},
              T{_upper.x, _lower.y, _lower.z}};
}

template <typename T>
auto Bound<T>::farZ() const -> Face {
  return Face{_lower,
              T{_lower.x, _upper.y, _upper.z},
              T{_upper.x, _upper.y, _upper.z},
              T{_upper.x, _lower.y, _upper.z}};
}

template <typename T>
void Bound<T>::scale(value_type multiplier) {
  auto center = this->center();
  auto extent = this->extent();
  auto offset = extent * multiplier * 0.5F;
  _lower      = center - offset;
  _upper      = center + offset;
}

template <typename T>
void Bound<T>::fit(value_type aspect) {
  auto extent = this->extent();
  auto center = this->center();
  auto offset = extent * 0.5F;

  value_type boundAspect = extent.x / extent.y;

  auto widthScale  = aspect / boundAspect;
  auto heightScale = 1.0F / widthScale;

  if (widthScale > heightScale) {
    offset.x *= widthScale;
  } else if (widthScale < heightScale) {
    offset.y *= heightScale;
  }

  _lower = center - offset;
  _upper = center + offset;

  // to extend the bound depth to fit sphere that contains the bound
  auto r   = radius();
  _lower.z = center.z - r;
  _upper.z = center.z + r;
}

template <typename T>
void Bound<T>::move(const T& offset) {
  _lower += offset;
  _upper += offset;
}

template <typename T>
void Bound<T>::expandPlanarSide(value_type padding) {
  for (int i = 0; i < 3; ++i) {
    if (_lower[i] == _upper[i]) {
      _lower[i] -= padding;
      _upper[i] += padding;
    }
  }
}

MI_NAMESPACE_END(Vulk)