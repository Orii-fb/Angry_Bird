#include "orbit_camera.h"

#include <algorithm>
#include <cmath>

namespace siege_lab
{

namespace
{

constexpr float kMinimumRadius = 2.0f;
constexpr float kMaximumRadius = 50.0f;
constexpr float kMinimumPitch = -89.0f;
constexpr float kMaximumPitch = 89.0f;
constexpr float kMouseSensitivity = 0.25f;
constexpr float kKeyboardZoomSpeed = 5.0f;
constexpr float kDegreesToRadians = 0.017453292519943295f;

float ClampRadius(float _radius)
{
  if (!std::isfinite(_radius))
  {
    return 12.0f;
  }

  return std::clamp(_radius, kMinimumRadius, kMaximumRadius);
}

}  // namespace

OrbitCamera::OrbitCamera(
    Vector3 _target, float _radius,
    float _rotation_speed_degrees_per_second)
    : m_target(_target),
      m_radius(ClampRadius(_radius)),
      m_initial_radius(m_radius),
      m_rotation_speed_degrees_per_second(
          std::isfinite(_rotation_speed_degrees_per_second)
              ? std::max(0.0f, _rotation_speed_degrees_per_second)
              : 90.0f)
{
  UpdateCameraPosition();
}

void OrbitCamera::Update(const InputSnapshot& _input, float _delta_time)
{
  const float delta_time =
      std::isfinite(_delta_time) ? std::max(0.0f, _delta_time) : 0.0f;
  const float rotation_step =
      m_rotation_speed_degrees_per_second * delta_time;
  ApplyKeyboardRotation(_input, rotation_step);
  ApplyMouseRotation(_input);
  ClampAngles();
  ApplyZoom(_input, delta_time);
  UpdateCameraPosition();
}

void OrbitCamera::ApplyKeyboardRotation(
    const InputSnapshot& _input, float _rotation_step)
{
  m_yaw_degrees += static_cast<float>(_input.yaw_axis) * _rotation_step;
  m_pitch_degrees += static_cast<float>(_input.pitch_axis) * _rotation_step;
}

void OrbitCamera::ApplyMouseRotation(const InputSnapshot& _input)
{
  if (!_input.orbit_button_down)
  {
    return;
  }

  if (std::isfinite(_input.mouse_delta.x))
  {
    m_yaw_degrees += _input.mouse_delta.x * kMouseSensitivity;
  }

  if (std::isfinite(_input.mouse_delta.y))
  {
    m_pitch_degrees -= _input.mouse_delta.y * kMouseSensitivity;
  }
}

void OrbitCamera::ClampAngles()
{
  m_yaw_degrees = std::remainder(m_yaw_degrees, 360.0f);
  m_pitch_degrees =
      std::clamp(m_pitch_degrees, kMinimumPitch, kMaximumPitch);
}

void OrbitCamera::ApplyZoom(const InputSnapshot& _input, float _delta_time)
{
  m_radius -=
      static_cast<float>(_input.zoom_axis) * kKeyboardZoomSpeed * _delta_time;
  if (std::isfinite(_input.mouse_wheel))
  {
    m_radius -= _input.mouse_wheel;
  }
  m_radius = std::clamp(m_radius, kMinimumRadius, kMaximumRadius);
}

const Camera3D& OrbitCamera::GetCamera() const
{
  return m_camera;
}

void OrbitCamera::Reset()
{
  m_radius = m_initial_radius;
  m_yaw_degrees = 0.0f;
  m_pitch_degrees = 0.0f;
  UpdateCameraPosition();
}

void OrbitCamera::UpdateCameraPosition()
{
  const float yaw = m_yaw_degrees * kDegreesToRadians;
  const float pitch = m_pitch_degrees * kDegreesToRadians;
  const float horizontal_radius = m_radius * std::cos(pitch);

  m_camera.position = {
      m_target.x + horizontal_radius * std::sin(yaw),
      m_target.y + m_radius * std::sin(pitch),
      m_target.z + horizontal_radius * std::cos(yaw)};
  m_camera.target = m_target;
  m_camera.up = {0.0f, 1.0f, 0.0f};
  m_camera.fovy = 45.0f;
  m_camera.projection = CAMERA_PERSPECTIVE;
}

}  // namespace siege_lab
