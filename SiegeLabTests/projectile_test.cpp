#include <array>

#include <gtest/gtest.h>

#include <cyclone/collide_fine.h>
#include <cyclone/contacts.h>

#include "projectile.h"

namespace siege_lab
{

namespace
{

void SimulateProjectileStep(
    Projectile& _projectile, const cyclone::CollisionPlane& _ground,
    cyclone::ContactResolver& _resolver, float _delta_time)
{
  _projectile.Body().integrate(_delta_time);
  _projectile.Collider().calculateInternals();

  std::array<cyclone::Contact, 4> contacts;
  cyclone::CollisionData data;
  data.contactArray = contacts.data();
  data.friction = 0.8f;
  data.restitution = 0.0f;
  data.tolerance = 0.1f;
  data.reset(static_cast<unsigned>(contacts.size()));
  cyclone::CollisionDetector::sphereAndHalfSpace(
      _projectile.Collider(), _ground, &data);

  if (data.contactCount > 0)
  {
    _resolver.resolveContacts(
        contacts.data(), data.contactCount, _delta_time);
    _projectile.RegisterPhysics();
  }
}

}  // namespace

TEST(ProjectileTest, StandardProfileHasExpectedMassAndCollider)
{
  StandardProjectile projectile;

  EXPECT_FLOAT_EQ(1.0f, projectile.Mass());
  EXPECT_FLOAT_EQ(0.28f, projectile.Radius());
  EXPECT_DOUBLE_EQ(1.0, projectile.Body().getMass());
  EXPECT_NEAR(0.03136, projectile.Body().getInertiaTensor().data[0], 1e-8);
  EXPECT_DOUBLE_EQ(projectile.Radius(), projectile.Collider().radius);
  EXPECT_EQ(&projectile.Body(), projectile.Collider().body);
}

TEST(ProjectileTest, HeavyProfileHasExpectedMassAndCollider)
{
  HeavyProjectile projectile;

  EXPECT_FLOAT_EQ(2.0f, projectile.Mass());
  EXPECT_FLOAT_EQ(0.36f, projectile.Radius());
  EXPECT_DOUBLE_EQ(2.0, projectile.Body().getMass());
  EXPECT_NEAR(0.10368, projectile.Body().getInertiaTensor().data[0], 1e-8);
  EXPECT_DOUBLE_EQ(projectile.Radius(), projectile.Collider().radius);
  EXPECT_EQ(&projectile.Body(), projectile.Collider().body);
}

TEST(ProjectileTest, FireSetsBodyPositionAndVelocity)
{
  StandardProjectile projectile;
  const ShotSettings settings{
      {-8.0f, 1.0f, 0.0f}, {0.6f, 0.8f, 0.0f}, 10.0f,
      ProjectileKind::kStandard};

  projectile.Fire(settings);

  const cyclone::Vector3 position = projectile.Body().getPosition();
  const cyclone::Vector3 velocity = projectile.Body().getVelocity();
  EXPECT_DOUBLE_EQ(-8.0, position.x);
  EXPECT_DOUBLE_EQ(1.0, position.y);
  EXPECT_DOUBLE_EQ(0.0, position.z);
  EXPECT_DOUBLE_EQ(6.0, velocity.x);
  EXPECT_DOUBLE_EQ(8.0, velocity.y);
  EXPECT_DOUBLE_EQ(0.0, velocity.z);
  EXPECT_TRUE(projectile.Body().getAwake());
  EXPECT_NEAR(
      -9.81, projectile.Body().getAcceleration().y, 1e-8);
  const cyclone::Vector3 collider_position =
      projectile.Collider().getAxis(3);
  EXPECT_DOUBLE_EQ(position.x, collider_position.x);
  EXPECT_DOUBLE_EQ(position.y, collider_position.y);
  EXPECT_DOUBLE_EQ(position.z, collider_position.z);
}

TEST(ProjectileTest, CycloneIntegratesProjectileIntoGroundContact)
{
  StandardProjectile projectile;
  const ShotSettings settings{
      {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.0f,
      ProjectileKind::kStandard};
  projectile.Fire(settings);
  cyclone::CollisionPlane ground;
  ground.direction = cyclone::Vector3(0.0f, 1.0f, 0.0f);
  ground.offset = 0.0f;
  cyclone::ContactResolver resolver(8);

  for (int step = 0; step < 240; ++step)
  {
    SimulateProjectileStep(projectile, ground, resolver, 1.0f / 60.0f);
  }

  const cyclone::Vector3 position = projectile.Body().getPosition();
  EXPECT_NEAR(projectile.Radius(), position.y, 0.03f);
  EXPECT_NEAR(0.0f, projectile.Body().getVelocity().y, 0.05f);
}

}  // namespace siege_lab
