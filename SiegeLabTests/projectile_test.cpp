#include <array>

#include <gtest/gtest.h>

#include <cyclone/core.h>
#include <cyclone/collide_fine.h>
#include <cyclone/contacts.h>

#include "input_controller.h"
#include "physics_world.h"
#include "projectile.h"

namespace siege_lab
{

namespace
{

bool SimulateSphereStep(
    cyclone::RigidBody& _body, cyclone::CollisionSphere& _sphere,
    const cyclone::CollisionPlane& _ground,
    cyclone::ContactResolver& _resolver, float _delta_time)
{
  _body.integrate(_delta_time);
  _sphere.calculateInternals();

  std::array<cyclone::Contact, 4> contacts;
  cyclone::CollisionData data;
  data.contactArray = contacts.data();
  data.friction = 0.8f;
  data.restitution = 0.0f;
  data.tolerance = 0.1f;
  data.reset(static_cast<unsigned>(contacts.size()));
  cyclone::CollisionDetector::sphereAndHalfSpace(
      _sphere, _ground, &data);

  if (data.contactCount > 0)
  {
    _resolver.resolveContacts(
        contacts.data(), data.contactCount, _delta_time);
    _sphere.calculateInternals();
  }
  return data.contactCount > 0;
}

bool SimulateProjectileStep(
    Projectile& _projectile, const cyclone::CollisionPlane& _ground,
    cyclone::ContactResolver& _resolver, float _delta_time)
{
  return SimulateSphereStep(
      _projectile.Body(), _projectile.Collider(), _ground,
      _resolver, _delta_time);
}

void ConfigureFreeFallBody(
    cyclone::RigidBody& _body, cyclone::CollisionSphere& _sphere,
    cyclone::Vector3 _position, cyclone::Vector3 _velocity)
{
  cyclone::Matrix3 inertia;
  inertia.setDiagonal(1.0, 1.0, 1.0);
  _body.setMass(1.0);
  _body.setInertiaTensor(inertia);
  _body.setDamping(1.0, 1.0);
  _body.setAcceleration(0.0, -9.81, 0.0);
  _body.setCanSleep(false);
  _body.setPosition(_position);
  _body.setVelocity(_velocity);
  _body.setAwake();
  _body.calculateDerivedData();
  _sphere.body = &_body;
  _sphere.radius = 0.5;
}

struct FlightResult
{
  cyclone::Vector3 position;
  cyclone::Vector3 velocity;
  int physics_steps = 0;
};

FlightResult RunFixedFlight(int _render_fps)
{
  PhysicsWorld world;
  cyclone::RigidBody body;
  cyclone::CollisionSphere sphere;
  ConfigureFreeFallBody(
      body, sphere, cyclone::Vector3(0.0, 50.0, 0.0),
      cyclone::Vector3(10.0, 0.0, 0.0));
  world.RegisterSphere(EntityId{0, 1}, body, sphere);
  int accumulated_ticks = 0;
  int steps = 0;
  for (int frame = 0; frame < _render_fps * 2; ++frame)
  {
    accumulated_ticks += 60;
    while (accumulated_ticks >= _render_fps)
    {
      world.Step(1.0f / 60.0f);
      accumulated_ticks -= _render_fps;
      ++steps;
    }
  }
  return {body.getPosition(), body.getVelocity(), steps};
}

void ExpectProjectileProfile(
    const Projectile& _projectile, double _mass, double _radius)
{
  EXPECT_NEAR(_mass, _projectile.Body().getMass(), 1e-4);
  EXPECT_NEAR(_radius, _projectile.Collider().radius, 1e-4);
}

void ExpectPositionAndVelocity(
    const cyclone::RigidBody& _body, cyclone::Vector3 _position,
    cyclone::Vector3 _velocity)
{
  const cyclone::Vector3 position = _body.getPosition();
  const cyclone::Vector3 velocity = _body.getVelocity();
  EXPECT_DOUBLE_EQ(_position.x, position.x);
  EXPECT_DOUBLE_EQ(_position.y, position.y);
  EXPECT_DOUBLE_EQ(_position.z, position.z);
  EXPECT_DOUBLE_EQ(_velocity.x, velocity.x);
  EXPECT_DOUBLE_EQ(_velocity.y, velocity.y);
  EXPECT_DOUBLE_EQ(_velocity.z, velocity.z);
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

TEST(Entities, GO03_ProjectileProfilesAreDistinct)
{
  StandardProjectile standard;
  HeavyProjectile heavy;

  ExpectProjectileProfile(standard, 1.0, 0.28);
  ExpectProjectileProfile(heavy, 2.0, 0.36);
}

TEST(ProjectileTest, FireSetsBodyPositionAndVelocity)
{
  StandardProjectile projectile;
  const ShotSettings settings{
      {-8.0f, 1.0f, 0.0f}, {0.6f, 0.8f, 0.0f}, 10.0f,
      ProjectileKind::kStandard};

  projectile.Fire(settings);

  ExpectPositionAndVelocity(
      projectile.Body(), cyclone::Vector3(-8.0, 1.0, 0.0),
      cyclone::Vector3(6.0, 8.0, 0.0));
  EXPECT_TRUE(projectile.Body().getAwake());
  EXPECT_NEAR(
      -9.81, projectile.Body().getAcceleration().y, 1e-8);
  const cyclone::Vector3 collider_position =
      projectile.Collider().getAxis(3);
  EXPECT_DOUBLE_EQ(-8.0, collider_position.x);
  EXPECT_DOUBLE_EQ(1.0, collider_position.y);
  EXPECT_DOUBLE_EQ(0.0, collider_position.z);
}

TEST(Projectile, PR01_OnlyOneShotPerKeyPress)
{
  FirePressTracker tracker;
  unsigned shots = 0;
  for (int update = 0; update < 10; ++update)
  {
    shots += tracker.Update(true) ? 1U : 0U;
  }
  EXPECT_EQ(1U, shots);
  EXPECT_FALSE(tracker.Update(false));
  EXPECT_TRUE(tracker.Update(true));
}

TEST(Projectile, PR02_InitialVelocityMatchesAim)
{
  StandardProjectile projectile;
  projectile.Fire({
      {-8.0f, 1.0f, 0.0f}, {0.8660254f, 0.5f, 0.0f}, 20.0f,
      ProjectileKind::kStandard});
  const cyclone::Vector3 velocity = projectile.Body().getVelocity();

  EXPECT_NEAR(17.3205, velocity.x, 1e-3);
  EXPECT_NEAR(10.0, velocity.y, 1e-3);
  EXPECT_NEAR(0.0, velocity.z, 1e-3);
}

TEST(Projectile, PR03_FallsUnderGravity)
{
  PhysicsWorld world;
  cyclone::RigidBody body;
  cyclone::CollisionSphere sphere;
  ConfigureFreeFallBody(
      body, sphere, cyclone::Vector3(0.0, 20.0, 0.0),
      cyclone::Vector3(0.0, 0.0, 0.0));
  ASSERT_TRUE(world.RegisterSphere(EntityId{0, 1}, body, sphere));
  for (int step = 0; step < 60; ++step)
  {
    ASSERT_TRUE(world.Step(1.0f / 60.0f));
  }

  EXPECT_NEAR(-9.81, body.getVelocity().y, 0.05);
  EXPECT_NEAR(15.01, body.getPosition().y, 0.10);
}

TEST(Projectile, PR04_DoesNotCrossGround)
{
  cyclone::RigidBody body;
  cyclone::CollisionSphere sphere;
  ConfigureFreeFallBody(
      body, sphere, cyclone::Vector3(0.0, 3.0, 0.0),
      cyclone::Vector3(0.0, 0.0, 0.0));
  sphere.radius = 0.5;
  cyclone::CollisionPlane ground;
  ground.direction = cyclone::Vector3(0.0, 1.0, 0.0);
  ground.offset = 0.0;
  cyclone::ContactResolver resolver(8);
  unsigned contact_steps = 0;
  body.setAwake();
  for (int step = 0; step < 180; ++step)
  {
    contact_steps += SimulateSphereStep(
        body, sphere, ground, resolver, 1.0f / 60.0f) ? 1U : 0U;
  }
  EXPECT_NEAR(0.5, body.getPosition().y, 0.1);
  EXPECT_GT(contact_steps, 0U);
}

TEST(Projectile, PR05_IsIndependentOfRenderFPS)
{
  const FlightResult thirty_fps = RunFixedFlight(30);
  const FlightResult one_forty_four_fps = RunFixedFlight(144);

  EXPECT_EQ(120, thirty_fps.physics_steps);
  EXPECT_EQ(120, one_forty_four_fps.physics_steps);
  EXPECT_NEAR(
      0.0, (thirty_fps.position - one_forty_four_fps.position).magnitude(),
      1e-4);
  EXPECT_NEAR(
      0.0, (thirty_fps.velocity - one_forty_four_fps.velocity).magnitude(),
      1e-4);
  EXPECT_NEAR(20.0, thirty_fps.position.x, 1e-4);
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
