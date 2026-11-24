#include <cmath>

#include <gtest/gtest.h>

#include "launcher.h"

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

TEST(LauncherTest, ClampsYawAndPitchToSpecifiedRanges)
{
  Launcher launcher;
  InputSnapshot input;
  input.launcher_yaw_axis = 1;
  input.launcher_pitch_axis = -1;

  launcher.Aim(input, 10.0f);
  const ShotSettings settings = launcher.GetShotSettings();

  EXPECT_TRUE(Near(settings.direction.z,
                   std::cos(5.0f * PI / 180.0f) *
                       std::sin(30.0f * PI / 180.0f)));
  EXPECT_TRUE(Near(settings.direction.y, std::sin(5.0f * PI / 180.0f)));
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

TEST(LauncherTest, SelectsProjectileWithoutChangingAim)
{
  Launcher launcher;
  InputSnapshot input;
  input.heavy_projectile_pressed = true;

  launcher.Aim(input, 0.0f);
  const ShotSettings settings = launcher.GetShotSettings();

  EXPECT_EQ(ProjectileKind::kHeavy, settings.projectile_kind);
  EXPECT_TRUE(Near(settings.speed, 15.0f));
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
