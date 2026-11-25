#include <array>

#include <gtest/gtest.h>

#include <cyclone/collide_fine.h>

#include "physics_world.h"
#include "trajectory_predictor.h"

namespace siege_lab
{

namespace
{

void ExpectWorldUnchanged(
    const PhysicsWorld& _world, const cyclone::RigidBody& _witness)
{
  EXPECT_NEAR(-8.0, _witness.getPosition().x, 1e-4);
  EXPECT_NEAR(1.0, _witness.getPosition().y, 1e-4);
  EXPECT_NEAR(12.0, _witness.getVelocity().x, 1e-4);
  EXPECT_NEAR(5.0, _witness.getVelocity().y, 1e-4);
  EXPECT_EQ(1U, _world.RegistrationCount());
}

}  // namespace

TEST(Trajectory, T01_StartsAtLauncher)
{
  TrajectoryPredictor predictor;
  const ShotSettings settings{
      {-8.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 10.0f,
      ProjectileKind::kStandard};
  const TrajectoryResult result = predictor.Predict(settings, {}, 20);

  ASSERT_GT(result.points.size(), 1U);
  EXPECT_FLOAT_EQ(-8.0f, result.points.front().x);
  EXPECT_FLOAT_EQ(1.0f, result.points.front().y);
  EXPECT_FLOAT_EQ(0.0f, result.points.front().z);
}

TEST(Trajectory, T03_DoesNotModifyPhysicsState)
{
  PhysicsWorld world;
  cyclone::RigidBody witness;
  cyclone::CollisionSphere collider;
  witness.setMass(1.0);
  witness.setAcceleration(0.0, -9.81, 0.0);
  witness.setPosition(-8.0, 1.0, 0.0);
  witness.setVelocity(12.0, 5.0, 0.0);
  witness.setAwake();
  witness.calculateDerivedData();
  collider.body = &witness;
  collider.radius = 0.28;
  ASSERT_TRUE(world.RegisterSphere(EntityId{0, 1}, witness, collider));
  const TrajectoryPredictor predictor;
  const ShotSettings settings{
      {-8.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 12.0f,
      ProjectileKind::kStandard};
  const TrajectoryResult result = predictor.Predict(settings, {}, 20);

  ASSERT_GT(result.points.size(), 1U);
  EXPECT_LE(result.points.size(), 21U);
  ExpectWorldUnchanged(world, witness);
}

TEST(Trajectory, T02_StopsAtFirstGroundContact)
{
  TrajectoryPredictor predictor;
  const ShotSettings settings{
      {-8.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 10.0f,
      ProjectileKind::kStandard};

  const TrajectoryResult result = predictor.Predict(settings);

  ASSERT_TRUE(result.hit_ground);
  EXPECT_FALSE(result.hit_obstacle);
  EXPECT_FALSE(result.reached_step_limit);
  ASSERT_GT(result.points.size(), 1U);
  EXPECT_NEAR(0.28f, result.points.back().y, 0.02f);
}

TEST(Trajectory, T04_StopsAtNearestObstacle)
{
  TrajectoryPredictor predictor;
  const ShotSettings settings{
      {-8.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 10.0f,
      ProjectileKind::kStandard};
  const std::array<TrajectoryBox, 1> boxes{{
      {{-6.0f, 1.0f, 0.0f}, {0.25f, 0.5f, 0.5f},
       {0.0f, 0.0f, 0.0f, 1.0f}}}};

  const TrajectoryResult result = predictor.Predict(settings, boxes);

  EXPECT_FALSE(result.hit_ground);
  EXPECT_TRUE(result.hit_obstacle);
  EXPECT_FALSE(result.reached_step_limit);
  ASSERT_GT(result.points.size(), 1U);
  EXPECT_NEAR(-6.53f, result.points.back().x, 0.1f);
  EXPECT_FLOAT_EQ(-6.0f, boxes[0].position.x);
  EXPECT_FLOAT_EQ(1.0f, settings.origin.y);
}

TEST(TrajectoryPredictorTest, StopsAtMaximumStepCount)
{
  TrajectoryPredictor predictor;
  const ShotSettings settings{
      {-8.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 10.0f,
      ProjectileKind::kStandard};

  const TrajectoryResult result = predictor.Predict(settings, {}, 3);

  EXPECT_FALSE(result.hit_ground);
  EXPECT_FALSE(result.hit_obstacle);
  EXPECT_TRUE(result.reached_step_limit);
  EXPECT_EQ(4U, result.points.size());
}

TEST(TrajectoryPredictorTest, UsesSelectedProjectileRadius)
{
  TrajectoryPredictor predictor;
  const ShotSettings standard{
      {-8.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 10.0f,
      ProjectileKind::kStandard};
  ShotSettings heavy = standard;
  heavy.projectile_kind = ProjectileKind::kHeavy;

  const TrajectoryResult standard_path = predictor.Predict(standard);
  const TrajectoryResult heavy_path = predictor.Predict(heavy);

  ASSERT_TRUE(standard_path.hit_ground);
  ASSERT_TRUE(heavy_path.hit_ground);
  EXPECT_NEAR(0.28f, standard_path.points.back().y, 0.02f);
  EXPECT_NEAR(0.36f, heavy_path.points.back().y, 0.02f);
}

}  // namespace siege_lab
