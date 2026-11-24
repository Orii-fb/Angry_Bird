#ifndef SIEGE_LAB_CORE_LAUNCHER_H_
#define SIEGE_LAB_CORE_LAUNCHER_H_

#include "game_input.h"

namespace siege_lab
{

enum class ProjectileKind
{
  kStandard,
  kHeavy
};

struct ShotSettings
{
  Vector3 origin;
  Vector3 direction;
  float speed;
  ProjectileKind projectile_kind;

  Vector3 GetInitialVelocity() const
  {
    return {direction.x * speed, direction.y * speed, direction.z * speed};
  }
};

class Launcher
{
 public:
  explicit Launcher(Vector3 _origin = {-8.0f, 1.0f, 0.0f});

  void Aim(const InputSnapshot& _input, float _delta_time);
  ShotSettings GetShotSettings() const;

 private:
  Vector3 m_origin;
  float m_yaw_degrees = 0.0f;
  float m_pitch_degrees = 5.0f;
  float m_speed = 15.0f;
  ProjectileKind m_projectile_kind = ProjectileKind::kStandard;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_LAUNCHER_H_
