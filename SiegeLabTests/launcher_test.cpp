#include <cmath>

#include <gtest/gtest.h>

#include "launcher.h"
#include "orbit_camera.h"
#include "projectile.h"
#include "scene_objects.h"

namespace siege_lab
{

namespace
{

bool Near(float _left, float _right)
{
  return std::fabs(_left - _right) < 0.01f;
}

float Length(Vector3 _vector)
{
  return std::sqrt(_vector.x * _vector.x + _vector.y * _vector.y +
                   _vector.z * _vector.z);
}

void SetAim(Launcher& _launcher, int _yaw, float _pitch_degrees)
{
  InputSnapshot input;
  input.launcher_pitch_axis = 1;
  _launcher.Aim(input, (_pitch_degrees - 5.0f) / 60.0f);
  input.launcher_pitch_axis = 0;
  input.launcher_yaw_axis = _yaw < 0 ? -1 : 1;
  _launcher.Aim(input, std::fabs(static_cast<float>(_yaw)) / 60.0f);
}

void ExpectSameDirection(Vector3 _left, Vector3 _right)
{
  EXPECT_NEAR(_left.x, _right.x, 1e-4f);
  EXPECT_NEAR(_left.y, _right.y, 1e-4f);
  EXPECT_NEAR(_left.z, _right.z, 1e-4f);
}

bool HasProjectileKind(
    SceneObjects<2>& _objects, EntityId _id, ProjectileKind _kind)
{
  GameEntity* entity = _objects.Get(_id);
  return _kind == ProjectileKind::kHeavy
      ? static_cast<HeavyProjectile*>(entity) != nullptr
      : static_cast<StandardProjectile*>(entity) != nullptr;
}

ShotSettings SelectProjectile(Launcher& _launcher, bool _heavy)
{
  InputSnapshot input;
  input.launcher_speed_axis = 1;
  _launcher.Aim(input, 0.2f);
  input.launcher_speed_axis = 0;
  input.heavy_projectile_pressed = _heavy;
  input.standard_projectile_pressed = !_heavy;
  _launcher.Aim(input, 0.0f);
  return _launcher.GetShotSettings();
}

}  // namespace

TEST(LauncherTest, StartsWithForwardDirectionAndStandardProjectile)
{
  const Launcher launcher;
  const ShotSettings settings = launcher.GetShotSettings();

  EXPECT_TRUE(Near(settings.origin.x, -8.0f));
  EXPECT_TRUE(Near(settings.origin.y, 1.0f));
  EXPECT_TRUE(Near(settings.origin.z, 0.0f));
  EXPECT_TRUE(Near(settings.direction.x, std::cos(5.0f * PI / 180.0f)));
  EXPECT_TRUE(Near(settings.direction.y, std::sin(5.0f * PI / 180.0f)));
  EXPECT_TRUE(Near(settings.direction.z, 0.0f));
  EXPECT_TRUE(Near(settings.speed, 15.0f));
  EXPECT_EQ(ProjectileKind::kStandard, settings.projectile_kind);
}

TEST(Launcher, L01_ZeroAzimuthHasNoZComponent)
{
  Launcher launcher;
  InputSnapshot input;
  input.launcher_pitch_axis = 1;

  launcher.Aim(input, 25.0f / 60.0f);
  const ShotSettings settings = launcher.GetShotSettings();

  EXPECT_NEAR(0.8660f, settings.direction.x, 1e-4f);
  EXPECT_NEAR(0.5000f, settings.direction.y, 1e-4f);
  EXPECT_NEAR(0.0f, settings.direction.z, 1e-4f);
}

TEST(Launcher, L02_DirectionIsNormalized)
{
  Launcher launcher;
  SetAim(launcher, 30, 30.0f);
  const Vector3 positive = launcher.GetShotSettings().direction;
  Launcher negative_launcher;
  SetAim(negative_launcher, -30, 45.0f);
  const Vector3 negative = negative_launcher.GetShotSettings().direction;

  EXPECT_NEAR(0.7500f, positive.x, 1e-4f);
  EXPECT_NEAR(0.5000f, positive.y, 1e-4f);
  EXPECT_NEAR(0.4330f, positive.z, 1e-4f);
  EXPECT_NEAR(0.6124f, negative.x, 1e-4f);
  EXPECT_NEAR(0.7071f, negative.y, 1e-4f);
  EXPECT_NEAR(-0.3536f, negative.z, 1e-4f);
  EXPECT_NEAR(1.0f, Length(positive), 1e-4f);
  EXPECT_NEAR(1.0f, Length(negative), 1e-4f);
}

TEST(Launcher, L03_ParametersAreClamped)
{
  Launcher launcher;
  InputSnapshot input;
  input.launcher_yaw_axis = 1;
  input.launcher_pitch_axis = -1;
  input.launcher_speed_axis = 1;

  launcher.Aim(input, 10.0f);
  const ShotSettings settings = launcher.GetShotSettings();

  EXPECT_NEAR(30.0f, std::atan2(
      settings.direction.z, settings.direction.x) * 180.0f / PI, 1e-3f);
  EXPECT_NEAR(5.0f, std::asin(settings.direction.y) * 180.0f / PI, 1e-3f);
  EXPECT_NEAR(22.0f, settings.speed, 1e-4f);
}

TEST(Launcher, L04_AimIsIndependentFromCamera)
{
  Launcher launcher;
  SetAim(launcher, 20, 30.0f);
  const Vector3 before = launcher.GetShotSettings().direction;
  OrbitCamera camera({0.0f, 0.0f, 0.0f}, 10.0f, 90.0f);
  InputSnapshot input;
  input.yaw_axis = 1;
  camera.Update(input, 120.0f / 90.0f);
  const Vector3 after = launcher.GetShotSettings().direction;

  EXPECT_NEAR(0.8138f, before.x, 1e-4f);
  EXPECT_NEAR(0.5000f, before.y, 1e-4f);
  EXPECT_NEAR(0.2962f, before.z, 1e-4f);
  EXPECT_NEAR(before.x, after.x, 1e-4f);
  EXPECT_NEAR(before.y, after.y, 1e-4f);
  EXPECT_NEAR(before.z, after.z, 1e-4f);
}

TEST(Launcher, L05_SelectsConcreteProjectileKind)
{
  SceneObjects<2> scene_objects;
  Launcher heavy_launcher;
  const ShotSettings heavy_settings =
      SelectProjectile(heavy_launcher, true);
  const auto heavy_id = scene_objects.CreateEntity<HeavyProjectile>();
  Launcher standard_launcher;
  const ShotSettings standard_settings =
      SelectProjectile(standard_launcher, false);
  const auto standard_id =
      scene_objects.CreateEntity<StandardProjectile>();

  ASSERT_TRUE(heavy_id.has_value());
  ASSERT_TRUE(standard_id.has_value());
  EXPECT_EQ(ProjectileKind::kHeavy, heavy_settings.projectile_kind);
  EXPECT_EQ(ProjectileKind::kStandard, standard_settings.projectile_kind);
  EXPECT_EQ(16.0f, heavy_settings.speed);
  EXPECT_EQ(16.0f, standard_settings.speed);
  ExpectSameDirection(heavy_settings.direction, standard_settings.direction);
  EXPECT_TRUE(HasProjectileKind(
      scene_objects, *heavy_id, ProjectileKind::kHeavy));
  EXPECT_TRUE(HasProjectileKind(
      scene_objects, *standard_id, ProjectileKind::kStandard));
}

TEST(LauncherTest, PositiveYawTurnsTowardPositiveZ)
{
  Launcher launcher;
  InputSnapshot input;
  input.launcher_yaw_axis = 1;

  launcher.Aim(input, 1.0f);
  const ShotSettings settings = launcher.GetShotSettings();

  EXPECT_GT(settings.direction.z, 0.0f);
  EXPECT_TRUE(Near(settings.direction.x,
                   std::cos(5.0f * PI / 180.0f) *
                       std::cos(30.0f * PI / 180.0f)));
}

TEST(LauncherTest, PositivePitchRaisesTheShot)
{
  Launcher launcher;
  InputSnapshot input;
  input.launcher_pitch_axis = 1;

  launcher.Aim(input, 1.0f);
  const ShotSettings settings = launcher.GetShotSettings();

  EXPECT_GT(settings.direction.y, std::sin(5.0f * PI / 180.0f));
  EXPECT_TRUE(Near(Length(settings.direction), 1.0f));
}

TEST(LauncherTest, ClampsSpeedAndKeepsDirectionNormalized)
{
  Launcher launcher;
  InputSnapshot input;
  input.launcher_speed_axis = 1;

  launcher.Aim(input, 10.0f);
  ShotSettings settings = launcher.GetShotSettings();
  EXPECT_TRUE(Near(settings.speed, 22.0f));

  input.launcher_speed_axis = -1;
  launcher.Aim(input, 10.0f);
  settings = launcher.GetShotSettings();
  EXPECT_TRUE(Near(settings.speed, 8.0f));
  EXPECT_TRUE(Near(Length(settings.direction), 1.0f));
}

TEST(LauncherTest, CalculatesInitialVelocityFromDirectionAndSpeed)
{
  Launcher launcher;
  InputSnapshot input;
  input.launcher_yaw_axis = 1;
  input.launcher_speed_axis = 1;
  launcher.Aim(input, 0.25f);

  const ShotSettings settings = launcher.GetShotSettings();
  const Vector3 velocity = settings.GetInitialVelocity();

  EXPECT_TRUE(Near(velocity.x, settings.direction.x * settings.speed));
  EXPECT_TRUE(Near(velocity.y, settings.direction.y * settings.speed));
  EXPECT_TRUE(Near(velocity.z, settings.direction.z * settings.speed));
  EXPECT_TRUE(Near(Length(velocity), settings.speed));
}

}  // namespace siege_lab
