#include <cmath>

#include <gtest/gtest.h>

#include "level_builder.h"

namespace siege_lab
{

namespace
{

void ExpectBlockProfile(
    const Block& _block, double _mass, cyclone::Vector3 _half_size,
    double _center_y, double _inertia_x, double _inertia_y)
{
  EXPECT_DOUBLE_EQ(_mass, _block.Body().getMass());
  EXPECT_DOUBLE_EQ(_half_size.x, _block.Collider().halfSize.x);
  EXPECT_DOUBLE_EQ(_half_size.y, _block.Collider().halfSize.y);
  EXPECT_DOUBLE_EQ(_half_size.z, _block.Collider().halfSize.z);
  EXPECT_DOUBLE_EQ(_center_y, _block.Body().getPosition().y);
  EXPECT_NEAR(_inertia_x, _block.Body().getInertiaTensor().data[0], 1e-8);
  EXPECT_NEAR(_inertia_y, _block.Body().getInertiaTensor().data[4], 1e-8);
}

void AdvanceLevel(
    LevelBuilder& _level, unsigned _ticks, Projectile* _projectile = nullptr)
{
  for (unsigned tick = 0; tick < _ticks; ++tick)
  {
    _level.Step(1.0f / 60.0f, _projectile);
  }
}

}  // namespace

TEST(LevelBuilderTest, BuildsExactPillarAndCubeProfiles)
{
  LevelBuilder level;
  ASSERT_TRUE(level.BuildPillarCube());
  ASSERT_NE(nullptr, level.Pillar());
  ASSERT_NE(nullptr, level.Cube());

  ExpectBlockProfile(
      *level.Pillar(), 3.0, cyclone::Vector3(0.5, 1.5, 0.5),
      1.5, 2.5, 0.5);
  ExpectBlockProfile(
      *level.Cube(), 1.5, cyclone::Vector3(0.5, 0.5, 0.5),
      3.5, 0.25, 0.25);
  EXPECT_NEAR(-9.81, level.Pillar()->Body().getAcceleration().y, 1e-8);
  EXPECT_NEAR(0.98, level.Pillar()->Body().getLinearDamping(), 1e-8);
  EXPECT_NEAR(0.85, level.Cube()->Body().getAngularDamping(), 1e-8);
}

TEST(LevelBuilderTest, KeepsTwoBlocksAtRestForThirteenSeconds)
{
  LevelBuilder level;
  ASSERT_TRUE(level.BuildPillarCube());
  AdvanceLevel(level, 780);

  EXPECT_FALSE(level.Pillar()->Body().getAwake());
  EXPECT_FALSE(level.Cube()->Body().getAwake());
  EXPECT_NEAR(1.5, level.Pillar()->Body().getPosition().y, 0.03);
  EXPECT_NEAR(3.5, level.Cube()->Body().getPosition().y, 0.03);
  EXPECT_NEAR(0.0, level.Pillar()->Body().getPosition().x, 0.001);
  EXPECT_NEAR(0.0, level.Cube()->Body().getPosition().x, 0.001);
  EXPECT_NEAR(0.0, level.Pillar()->Body().getPosition().z, 0.001);
  EXPECT_NEAR(0.0, level.Cube()->Body().getPosition().z, 0.001);
  EXPECT_NEAR(0.0, level.Pillar()->Body().getVelocity().y, 0.05);
  EXPECT_NEAR(0.0, level.Cube()->Body().getVelocity().y, 0.05);
  EXPECT_NEAR(0.0, level.Pillar()->Body().getRotation().z, 0.05);
  EXPECT_NEAR(0.0, level.Cube()->Body().getRotation().x, 0.05);
  EXPECT_NEAR(0.0, level.Cube()->Body().getRotation().y, 0.05);
  EXPECT_NEAR(0.0, level.Cube()->Body().getRotation().z, 0.05);
}

TEST(LevelBuilderTest, ProjectileImpactTopplesPillarAndSeparatesCube)
{
  LevelBuilder level;
  ASSERT_TRUE(level.BuildPillarCube());
  AdvanceLevel(level, 780);

  StandardProjectile projectile;
  const float angle = 9.0f * PI / 180.0f;
  const ShotSettings shot{
      {-8.0f, 0.8f, 0.0f}, {std::cos(angle), std::sin(angle), 0.0f},
      16.0f, ProjectileKind::kStandard};
  projectile.Fire(shot);

  AdvanceLevel(level, 180, &projectile);

  EXPECT_TRUE(level.Pillar()->Body().getAwake());
  EXPECT_TRUE(level.Cube()->Body().getAwake());
  EXPECT_GT(std::fabs(level.Pillar()->Body().getOrientation().k), 0.02);
  EXPECT_GT(
      std::fabs(level.Cube()->Body().getPosition().x -
                level.Pillar()->Body().getPosition().x),
      0.1);
}

}  // namespace siege_lab
