#include "launcher.h"

#include <algorithm>
#include <cmath>

namespace siege_lab
{

namespace
{

constexpr float kMinimumYaw = -30.0f;
constexpr float kMaximumYaw = 30.0f;
constexpr float kMinimumPitch = 5.0f;
constexpr float kMaximumPitch = 55.0f;
constexpr float kMinimumSpeed = 8.0f;
constexpr float kMaximumSpeed = 22.0f;
constexpr float kAngleSpeed = 60.0f;
constexpr float kSpeedAdjustment = 5.0f;
constexpr float kDegreesToRadians = 0.017453292519943295f;

float ClampDeltaTime(float _delta_time)
{
  return std::isfinite(_delta_time) ? std::max(0.0f, _delta_time) : 0.0f;
}

}  // namespace

Launcher::Launcher(Vector3 _origin) : m_origin(_origin)
{
}

void Launcher::Aim(const InputSnapshot& _input, float _delta_time)
{
  const float delta_time = ClampDeltaTime(_delta_time);
  m_yaw_degrees +=
      static_cast<float>(_input.launcher_yaw_axis) * kAngleSpeed * delta_time;
  m_pitch_degrees += static_cast<float>(_input.launcher_pitch_axis) *
                     kAngleSpeed * delta_time;
  m_speed += static_cast<float>(_input.launcher_speed_axis) *
             kSpeedAdjustment * delta_time;

  m_yaw_degrees = std::clamp(m_yaw_degrees, kMinimumYaw, kMaximumYaw);
  m_pitch_degrees =
      std::clamp(m_pitch_degrees, kMinimumPitch, kMaximumPitch);
  m_speed = std::clamp(m_speed, kMinimumSpeed, kMaximumSpeed);

  if (_input.standard_projectile_pressed)
  {
    m_projectile_kind = ProjectileKind::kStandard;
  }
  else if (_input.heavy_projectile_pressed)
  {
    m_projectile_kind = ProjectileKind::kHeavy;
  }
}

ShotSettings Launcher::GetShotSettings() const
{
  const float yaw = m_yaw_degrees * kDegreesToRadians;
  const float pitch = m_pitch_degrees * kDegreesToRadians;
  const float horizontal_direction = std::cos(pitch);

  return {
      m_origin,
      {horizontal_direction * std::cos(yaw), std::sin(pitch),
       horizontal_direction * std::sin(yaw)},
      m_speed,
      m_projectile_kind};
}

}  // namespace siege_lab
