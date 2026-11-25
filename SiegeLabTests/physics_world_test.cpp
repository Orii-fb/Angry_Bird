#include <cmath>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <cyclone/core.h>

#include "physics_world.h"
#include "projectile.h"
#include "scene_objects.h"

namespace siege_lab
{

namespace
{

void ConfigureBody(
    cyclone::RigidBody& _body, cyclone::real _mass,
    cyclone::Vector3 _position)
{
  cyclone::Matrix3 inertia;
  inertia.setDiagonal(1.0, 1.0, 1.0);
  _body.setMass(_mass);
  _body.setInertiaTensor(inertia);
  _body.setDamping(1.0, 1.0);
  _body.setAcceleration(0.0, 0.0, 0.0);
  _body.setPosition(_position);
  _body.setVelocity(0.0, 0.0, 0.0);
  _body.setRotation(0.0, 0.0, 0.0);
  _body.calculateDerivedData();
}

void ConfigureFixedBody(cyclone::RigidBody& _body)
{
  cyclone::Matrix3 inverse_inertia;
  inverse_inertia.setDiagonal(0.0, 0.0, 0.0);
  _body.setInverseMass(0.0);
  _body.setInverseInertiaTensor(inverse_inertia);
  _body.calculateDerivedData();
}

void ConfigureSphere(
    cyclone::RigidBody& _body, cyclone::CollisionSphere& _sphere,
    cyclone::Vector3 _position, cyclone::real _radius)
{
  ConfigureBody(_body, 1.0, _position);
  _sphere.body = &_body;
  _sphere.radius = _radius;
}

void ConfigureMovingSphere(
    cyclone::RigidBody& _body, cyclone::CollisionSphere& _sphere,
    cyclone::Vector3 _position, cyclone::real _radius,
    cyclone::Vector3 _velocity)
{
  ConfigureSphere(_body, _sphere, _position, _radius);
  _body.setVelocity(_velocity);
  _body.setAwake();
}

void ConfigureGround(cyclone::CollisionPlane& _ground)
{
  _ground.direction = cyclone::Vector3(0.0, 1.0, 0.0);
  _ground.offset = 0.0;
}

void ConfigureBox(
    cyclone::RigidBody& _body, cyclone::CollisionBox& _box,
    cyclone::Vector3 _position, cyclone::Vector3 _half_size)
{
  ConfigureBody(_body, 1.0, _position);
  _box.body = &_body;
  _box.halfSize = _half_size;
}

void ConfigureCollisionData(
    cyclone::CollisionData& _data, cyclone::Contact* _contacts,
    unsigned _capacity)
{
  _data.contactArray = _contacts;
  _data.friction = 0.5;
  _data.restitution = 0.0;
  _data.tolerance = 0.01;
  _data.reset(_capacity);
}

void ExpectFullContactBuffer(
    const cyclone::CollisionData& _data, cyclone::Contact* _contacts,
    std::uint64_t _before, std::uint64_t _after)
{
  EXPECT_EQ(4U, _data.contactCount);
  EXPECT_EQ(0, _data.contactsLeft);
  EXPECT_EQ(_contacts + 4, _data.contacts);
  EXPECT_EQ(0x12345678U, _before);
  EXPECT_EQ(0x87654321U, _after);
}

EntityId TestId(std::size_t _slot)
{
  return EntityId{_slot, 1};
}

class RecordingListener final : public ICollisionListener
{
 public:
  void OnCollision(const CollisionEvent& _event) override
  {
    events.push_back(_event);
    if (observed_body != nullptr)
    {
      body_y_at_callback = observed_body->getPosition().y;
    }
  }

  std::vector<CollisionEvent> events;
  cyclone::RigidBody* observed_body = nullptr;
  cyclone::real body_y_at_callback = 0.0;
};

struct ContactCanaries
{
  std::uint64_t before = 0x12345678;
  cyclone::Contact contacts[4];
  std::uint64_t after = 0x87654321;
};

void ExpectGroundImpact(
    const RecordingListener& _listener, EntityId _body_id)
{
  ASSERT_EQ(1U, _listener.events.size());
  const ContactSnapshot& contact = _listener.events[0].contact;
  EXPECT_EQ(CollisionEventType::kEnter, _listener.events[0].type);
  EXPECT_EQ(_body_id, contact.pair.first_id);
  EXPECT_EQ(WorldGroundId(), contact.pair.second_id);
  EXPECT_GE(contact.closing_speed, 3.9);
  EXPECT_NEAR(1.0, contact.normal.y, 1e-6);
}

bool RegisterProjectileContact(
    PhysicsWorld& _world, EntityId _id,
    StandardProjectile& _projectile, cyclone::CollisionPlane& _ground,
    RecordingListener& _listener)
{
  _projectile.Fire({
      {0.0f, 0.32f, 0.0f}, {0.0f, -1.0f, 0.0f}, 4.0f,
      ProjectileKind::kStandard});
  return _world.Subscribe(_listener) != 0 &&
         _world.RegisterSphere(
             _id, _projectile.Body(), _projectile.Collider()) &&
         _world.RegisterPlane(_ground) && _world.Step(1.0f / 60.0f);
}

void ExpectPublishedContact(
    const RecordingListener& _listener, EntityId _first, EntityId _second,
    cyclone::real _closing_speed)
{
  ASSERT_EQ(1U, _listener.events.size());
  const CollisionEvent& event = _listener.events.front();
  EXPECT_EQ(CollisionEventType::kEnter, event.type);
  EXPECT_EQ(_first, event.contact.pair.first_id);
  EXPECT_EQ(_second, event.contact.pair.second_id);
  EXPECT_NEAR(_closing_speed, event.contact.closing_speed, 0.01);
}

void ExpectListenerEventCounts(
    const RecordingListener& _first, const RecordingListener& _second,
    std::size_t _first_count, std::size_t _second_count)
{
  EXPECT_EQ(_first_count, _first.events.size());
  EXPECT_EQ(_second_count, _second.events.size());
}

void ExpectEquivalentBodies(
    const cyclone::RigidBody& _expected, const cyclone::RigidBody& _actual)
{
  EXPECT_NEAR(
      _expected.getPosition().y, _actual.getPosition().y, 1e-4);
  EXPECT_NEAR(
      _expected.getVelocity().y, _actual.getVelocity().y, 1e-4);
}

struct PreparedContactResult
{
  unsigned count = 0;
  cyclone::real closing_speed = 0.0;
  cyclone::RigidBody* observed_body = nullptr;
  cyclone::Vector3 velocity_at_callback;
};

void CapturePreparedContact(
    const cyclone::Contact* _contacts, unsigned _count, void* _result)
{
  PreparedContactResult* result =
      static_cast<PreparedContactResult*>(_result);
  result->count = _count;
  result->closing_speed =
      _count == 0 ? 0.0 : _contacts[0].getClosingSpeed();
  if (_count > 0 && result->observed_body != nullptr)
  {
    result->velocity_at_callback =
        result->observed_body->getVelocity();
  }
}

}  // namespace

TEST(PhysicsWorld, PW01_RegisterBorrowsObject)
{
  SceneObjects<1> pool;
  PhysicsWorld world;
  const auto id = pool.CreateEntity<StandardProjectile>();
  ASSERT_TRUE(id.has_value());
  auto* object = static_cast<StandardProjectile*>(pool.Get(*id));
  ASSERT_NE(nullptr, object);
  cyclone::RigidBody* body_address = &object->Body();
  ASSERT_TRUE(world.RegisterSphere(
      *id, object->Body(), object->Collider()));

  EXPECT_EQ(1U, world.RegistrationCount());
  EXPECT_EQ(object, pool.Get(*id));
  EXPECT_EQ(body_address, &object->Body());
  EXPECT_TRUE(world.UnregisterObject(*id));
  EXPECT_EQ(0U, world.RegistrationCount());
  EXPECT_EQ(object, pool.Get(*id));
}

TEST(PhysicsWorld, PW02_StepUsesCyclone)
{
  PhysicsWorld world;
  cyclone::RigidBody world_body;
  cyclone::CollisionSphere world_sphere;
  ConfigureSphere(
      world_body, world_sphere, cyclone::Vector3(0.0, 20.0, 0.0), 0.5);
  world_body.setAcceleration(0.0, -9.81, 0.0);
  world_body.setCanSleep(false);
  cyclone::RigidBody direct_body;
  cyclone::CollisionSphere direct_sphere;
  ConfigureSphere(
      direct_body, direct_sphere, cyclone::Vector3(0.0, 20.0, 0.0), 0.5);
  direct_body.setAcceleration(0.0, -9.81, 0.0);
  direct_body.setCanSleep(false);
  ASSERT_TRUE(world.RegisterSphere(TestId(0), world_body, world_sphere));

  for (int step = 0; step < 60; ++step)
  {
    ASSERT_TRUE(world.Step(1.0f / 60.0f));
    direct_body.integrate(1.0f / 60.0f);
  }
  EXPECT_NEAR(-9.81, world_body.getVelocity().y, 0.05);
  ExpectEquivalentBodies(direct_body, world_body);
}

TEST(PhysicsWorld, PW03_RegisteredColliderIsUsed)
{
  PhysicsWorld world;
  cyclone::RigidBody body;
  cyclone::CollisionSphere sphere;
  ConfigureSphere(body, sphere, cyclone::Vector3(0.0, 0.45, 0.0), 0.5);
  body.setAcceleration(0.0, 0.0, 0.0);
  body.setCanSleep(false);
  cyclone::CollisionPlane ground;
  ConfigureGround(ground);
  ASSERT_TRUE(world.RegisterSphere(TestId(0), body, sphere));
  ASSERT_TRUE(world.RegisterPlane(ground));

  ASSERT_TRUE(world.Step(1.0f / 60.0f));
  EXPECT_GE(body.getPosition().y, 0.49);
  ASSERT_TRUE(world.UnregisterObject(TestId(0)));
  body.setPosition(0.0, 0.45, 0.0);
  body.calculateDerivedData();
  ASSERT_TRUE(world.Step(1.0f / 60.0f));
  EXPECT_NEAR(0.45, body.getPosition().y, 1e-6);
}

TEST(PhysicsWorld, PW04_NoDoubleIntegrationAfterMerge)
{
  PhysicsWorld world;
  cyclone::RigidBody body;
  cyclone::CollisionSphere sphere;
  ConfigureSphere(
      body, sphere, cyclone::Vector3(0.0, 10.0, 0.0), 0.25);
  body.setAcceleration(0.0, 0.0, 0.0);
  body.setVelocity(1.0, 0.0, 0.0);
  body.setCanSleep(false);
  ASSERT_TRUE(world.RegisterSphere(TestId(0), body, sphere));
  for (int step = 0; step < 60; ++step)
  {
    ASSERT_TRUE(world.Step(1.0f / 60.0f));
  }
  EXPECT_NEAR(1.0, body.getPosition().x, 0.001);
}

TEST(PhysicsWorld, PW05_UnregisterBeforeDeletion)
{
  SceneObjects<1> objects;
  PhysicsWorld world;
  RecordingListener listener;
  const auto id = objects.CreateEntity<StandardProjectile>();
  ASSERT_TRUE(id.has_value());
  auto* projectile = static_cast<StandardProjectile*>(objects.Get(*id));
  ASSERT_NE(nullptr, projectile);
  cyclone::CollisionPlane ground;
  ConfigureGround(ground);
  ASSERT_TRUE(RegisterProjectileContact(
      world, *id, *projectile, ground, listener));
  ExpectGroundImpact(listener, *id);
  ASSERT_TRUE(world.UnregisterObject(*id));
  ASSERT_TRUE(objects.Destroy(*id));
  EXPECT_EQ(0U, world.RegistrationCount());
  EXPECT_TRUE(world.Step(1.0f / 60.0f));
  EXPECT_EQ(nullptr, objects.Get(*id));
  EXPECT_EQ(1U, listener.events.size());
  ASSERT_TRUE(world.UnregisterPlane(ground));
}

TEST(PhysicsWorldTest, ResolvesBoxSphereContactsWithBoxFirst)
{
  PhysicsWorld world;
  cyclone::RigidBody box_body;
  cyclone::CollisionBox box;
  ConfigureBox(
      box_body, box, cyclone::Vector3(0.0, 0.0, 0.0),
      cyclone::Vector3(1.0, 1.0, 1.0));
  ConfigureFixedBody(box_body);
  cyclone::RigidBody sphere_body;
  cyclone::CollisionSphere sphere;
  ConfigureSphere(
      sphere_body, sphere, cyclone::Vector3(1.3, 0.0, 0.0), 0.5);
  ASSERT_TRUE(world.RegisterBox(TestId(0), box_body, box));
  ASSERT_TRUE(world.RegisterSphere(TestId(1), sphere_body, sphere));

  EXPECT_TRUE(world.Step(1.0f / 60.0f));

  EXPECT_GT(sphere_body.getPosition().x, 1.4);
}

TEST(PhysicsWorldTest, DetectsContactsAgainstRegisteredGroundPlane)
{
  PhysicsWorld world;
  cyclone::RigidBody body;
  cyclone::CollisionSphere sphere;
  ConfigureSphere(body, sphere, cyclone::Vector3(0.0, 0.2, 0.0), 0.5);
  cyclone::CollisionPlane ground;
  ground.direction = cyclone::Vector3(0.0, 1.0, 0.0);
  ground.offset = 0.0;
  ASSERT_TRUE(world.RegisterSphere(TestId(0), body, sphere));
  ASSERT_TRUE(world.RegisterPlane(ground));

  EXPECT_TRUE(world.Step(1.0f / 60.0f));

  EXPECT_GE(body.getPosition().y, 0.49);
  EXPECT_TRUE(world.UnregisterPlane(ground));
}

TEST(CollisionEvent, EV06_GroundHasNoBodyPointer)
{
  PhysicsWorld world;
  RecordingListener listener;
  ASSERT_NE(0U, world.Subscribe(listener));
  cyclone::RigidBody body;
  listener.observed_body = &body;
  cyclone::CollisionSphere sphere;
  ConfigureMovingSphere(
      body, sphere, cyclone::Vector3(0.0, 0.2, 0.0), 0.5,
      cyclone::Vector3(0.0, -4.0, 0.0));
  cyclone::CollisionPlane ground;
  ConfigureGround(ground);
  ASSERT_TRUE(world.RegisterSphere(TestId(0), body, sphere));
  ASSERT_TRUE(world.RegisterPlane(ground));

  EXPECT_TRUE(world.Step(1.0f / 60.0f));

  ExpectGroundImpact(listener, TestId(0));
  EXPECT_NEAR(body.getPosition().y, listener.body_y_at_callback, 1e-6);
  EXPECT_GE(listener.body_y_at_callback, 0.49);
}

TEST(CyclonePatch, EV10_ClosingSpeedIsExposed)
{
  cyclone::RigidBody body;
  ConfigureBody(body, 1.0, cyclone::Vector3(0.0, 0.0, 0.0));
  body.setAwake();
  body.setVelocity(0.0, -4.0, 0.0);
  cyclone::Contact contact{};
  contact.setBodyData(&body, nullptr, 0.5, 0.0);
  contact.contactPoint = cyclone::Vector3(0.0, 0.0, 0.0);
  contact.contactNormal = cyclone::Vector3(0.0, 1.0, 0.0);
  PreparedContactResult result;
  cyclone::ContactResolver resolver(1);
  resolver.setPreparedContactObserver(CapturePreparedContact, &result);

  resolver.resolveContacts(&contact, 1, 1.0f / 60.0f);

  EXPECT_EQ(1U, result.count);
  EXPECT_NEAR(4.0, result.closing_speed, 1e-6);
}

TEST(CyclonePatch, EV11_ContactPointRotationIsIncluded)
{
  cyclone::RigidBody body;
  ConfigureBody(body, 1.0, cyclone::Vector3(0.0, 0.0, 0.0));
  body.setRotation(0.0, 0.0, 2.0);
  body.setAwake();
  cyclone::Contact contact{};
  contact.setBodyData(&body, nullptr, 0.5, 0.0);
  contact.contactPoint = cyclone::Vector3(0.0, 1.0, 0.0);
  contact.contactNormal = cyclone::Vector3(1.0, 0.0, 0.0);
  PreparedContactResult result;
  cyclone::ContactResolver resolver(1);
  resolver.setPreparedContactObserver(CapturePreparedContact, &result);

  resolver.resolveContacts(&contact, 1, 1.0f / 60.0f);

  EXPECT_EQ(1U, result.count);
  EXPECT_NEAR(2.0, result.closing_speed, 0.01);
}

TEST(PhysicsWorldTest, UnregistrationClearsActivePairHistory)
{
  PhysicsWorld world;
  RecordingListener listener;
  world.Subscribe(listener);
  cyclone::RigidBody body;
  cyclone::CollisionSphere sphere;
  ConfigureSphere(body, sphere, cyclone::Vector3(0.0, 0.2, 0.0), 0.5);
  body.setVelocity(0.0, -3.0, 0.0);
  body.setAwake();
  cyclone::CollisionPlane ground;
  ground.direction = cyclone::Vector3(0.0, 1.0, 0.0);
  ground.offset = 0.0;
  ASSERT_TRUE(world.RegisterSphere(TestId(0), body, sphere));
  ASSERT_TRUE(world.RegisterPlane(ground));
  ASSERT_TRUE(world.Step(1.0f / 60.0f));
  ASSERT_EQ(1U, listener.events.size());

  EXPECT_TRUE(world.UnregisterObject(TestId(0)));
  EXPECT_TRUE(world.UnregisterPlane(ground));

  EXPECT_TRUE(world.Step(1.0f / 60.0f));
  EXPECT_EQ(1U, listener.events.size());
}

TEST(PhysicsWorldTest, BoxPlaneDetectorUpdatesCapacityOnEarlyReturn)
{
  cyclone::RigidBody body;
  cyclone::CollisionBox box;
  ConfigureBox(
      body, box, cyclone::Vector3(0.0, 0.0, 0.0),
      cyclone::Vector3(1.0, 1.0, 1.0));
  cyclone::CollisionPlane plane;
  plane.direction = cyclone::Vector3(0.0, 1.0, 0.0);
  plane.offset = 0.0;
  cyclone::Contact contacts[2];
  cyclone::CollisionData data{};
  ConfigureCollisionData(data, contacts, 2);

  const unsigned generated =
      cyclone::CollisionDetector::boxAndHalfSpace(box, plane, &data);

  EXPECT_EQ(2U, generated);
  EXPECT_EQ(2U, data.contactCount);
  EXPECT_EQ(0, data.contactsLeft);
  EXPECT_EQ(contacts + 2, data.contacts);
}

TEST(PhysicsWorld, PW06_ContactBufferNeverOverflows)
{
  ContactCanaries buffer;
  cyclone::RigidBody body;
  cyclone::CollisionBox box;
  ConfigureBox(
      body, box, cyclone::Vector3(0.0, -2.0, 0.0),
      cyclone::Vector3(1.0, 1.0, 1.0));
  box.calculateInternals();
  cyclone::CollisionPlane ground;
  ConfigureGround(ground);
  cyclone::CollisionData data{};
  ConfigureCollisionData(data, buffer.contacts, 4);

  EXPECT_EQ(
      4U, cyclone::CollisionDetector::boxAndHalfSpace(box, ground, &data));
  ExpectFullContactBuffer(
      data, buffer.contacts, buffer.before, buffer.after);
  cyclone::CollisionSphere sphere;
  EXPECT_EQ(
      0U, cyclone::CollisionDetector::sphereAndHalfSpace(
              sphere, ground, &data));
  EXPECT_EQ(0x87654321U, buffer.after);
}

TEST(CollisionEvent, EV08_RealCycloneContactIsPublished)
{
  PhysicsWorld world;
  RecordingListener listener;
  ASSERT_NE(0U, world.Subscribe(listener));
  cyclone::RigidBody first_body;
  cyclone::CollisionSphere first_sphere;
  ConfigureMovingSphere(
      first_body, first_sphere, cyclone::Vector3(0.0, 1.0, 0.0), 0.5,
      cyclone::Vector3(0.0, 0.0, 2.0));
  cyclone::RigidBody second_body;
  cyclone::CollisionSphere second_sphere;
  ConfigureMovingSphere(
      second_body, second_sphere, cyclone::Vector3(0.0, 1.0, 0.8), 0.5,
      cyclone::Vector3(0.0, 0.0, -1.0));
  ASSERT_TRUE(world.RegisterSphere(TestId(0), first_body, first_sphere));
  ASSERT_TRUE(world.RegisterSphere(TestId(1), second_body, second_sphere));

  ASSERT_TRUE(world.Step(1.0f / 60.0f));

  ExpectPublishedContact(listener, TestId(0), TestId(1), 3.0);
}

TEST(CollisionEventDispatcherTest, EV09_NotifiesOnlySubscribedListeners)
{
  CollisionEventDispatcher dispatcher;
  RecordingListener first;
  RecordingListener second;
  const auto second_id = dispatcher.Subscribe(second);
  ASSERT_NE(0U, dispatcher.Subscribe(first));
  CollisionEvent enter_event;
  enter_event.type = CollisionEventType::kEnter;
  enter_event.contact = {
      {TestId(0), TestId(1)}, {}, {1.0, 0.0, 0.0}, 4.0};
  const std::vector<CollisionEvent> enter{enter_event};
  dispatcher.Dispatch(enter);
  ExpectListenerEventCounts(first, second, 1U, 1U);

  ASSERT_TRUE(dispatcher.Unsubscribe(second_id));
  CollisionEvent exit_event;
  exit_event.type = CollisionEventType::kExit;
  exit_event.contact = {
      {TestId(0), TestId(1)}, {}, {1.0, 0.0, 0.0}, 0.0};
  const std::vector<CollisionEvent> exit{exit_event};
  dispatcher.Dispatch(exit);
  ExpectListenerEventCounts(first, second, 2U, 1U);
}

TEST(CyclonePatch, EV12_ObserverRunsBeforeResolution)
{
  cyclone::RigidBody body;
  ConfigureBody(body, 1.0, cyclone::Vector3(0.0, 0.0, 0.0));
  body.setAwake();
  body.setVelocity(-4.0, 0.0, 0.0);
  cyclone::Contact contact{};
  contact.setBodyData(&body, nullptr, 0.5, 0.0);
  contact.contactPoint = cyclone::Vector3(0.0, 0.0, 0.0);
  contact.contactNormal = cyclone::Vector3(1.0, 0.0, 0.0);
  PreparedContactResult result;
  result.observed_body = &body;
  cyclone::ContactResolver resolver(1);
  resolver.setPreparedContactObserver(CapturePreparedContact, &result);

  resolver.resolveContacts(&contact, 1, 1.0f / 60.0f);
  EXPECT_EQ(1U, result.count);
  EXPECT_NEAR(4.0, result.closing_speed, 1e-3);
  EXPECT_NEAR(-4.0, result.velocity_at_callback.x, 1e-3);
  EXPECT_NEAR(0.0, body.getVelocity().x, 1e-3);
  result.count = 0;
  resolver.resolveContacts(nullptr, 0, 1.0f / 60.0f);
  EXPECT_EQ(0U, result.count);
}

TEST(PhysicsWorldTest, ResolvesBoxBoxContacts)
{
  PhysicsWorld world;
  cyclone::RigidBody fixed_body;
  cyclone::CollisionBox fixed_box;
  ConfigureBox(
      fixed_body, fixed_box, cyclone::Vector3(0.0, 0.0, 0.0),
      cyclone::Vector3(1.0, 1.0, 1.0));
  ConfigureFixedBody(fixed_body);
  cyclone::RigidBody moving_body;
  cyclone::CollisionBox moving_box;
  ConfigureBox(
      moving_body, moving_box, cyclone::Vector3(1.5, 0.0, 0.0),
      cyclone::Vector3(1.0, 1.0, 1.0));
  ASSERT_TRUE(world.RegisterBox(TestId(0), fixed_body, fixed_box));
  ASSERT_TRUE(world.RegisterBox(TestId(1), moving_body, moving_box));

  EXPECT_TRUE(world.Step(1.0f / 60.0f));

  EXPECT_GT(moving_body.getPosition().x, 1.6);
}

TEST(PhysicsWorldTest, ResolvesBoxContactsAgainstGroundPlane)
{
  PhysicsWorld world;
  cyclone::RigidBody body;
  cyclone::CollisionBox box;
  ConfigureBox(
      body, box, cyclone::Vector3(0.0, 0.2, 0.0),
      cyclone::Vector3(0.5, 0.5, 0.5));
  cyclone::CollisionPlane ground;
  ground.direction = cyclone::Vector3(0.0, 1.0, 0.0);
  ground.offset = 0.0;
  ASSERT_TRUE(world.RegisterBox(TestId(0), body, box));
  ASSERT_TRUE(world.RegisterPlane(ground));

  EXPECT_TRUE(world.Step(1.0f / 60.0f));

  EXPECT_GE(body.getPosition().y, 0.49);
}

TEST(PhysicsWorldTest, ReportsIncompleteDetectionWhenContactBufferIsFull)
{
  PhysicsWorld world(1);
  cyclone::RigidBody bodies[3];
  cyclone::CollisionSphere spheres[3];
  for (std::size_t index = 0; index < 3; ++index)
  {
    ConfigureBody(
        bodies[index], 1.0,
        cyclone::Vector3(static_cast<double>(index) * 0.5, 0.0, 0.0));
    spheres[index].body = &bodies[index];
    spheres[index].radius = 1.0;
    ASSERT_TRUE(world.RegisterSphere(
        TestId(index), bodies[index], spheres[index]));
  }

  EXPECT_FALSE(world.Step(1.0f / 60.0f));
  for (const cyclone::RigidBody& body : bodies)
  {
    EXPECT_TRUE(std::isfinite(body.getPosition().x));
    EXPECT_TRUE(std::isfinite(body.getPosition().y));
    EXPECT_TRUE(std::isfinite(body.getPosition().z));
  }
}

TEST(PhysicsWorldTest, SkipsFixedFixedPairsEvenWithNoContactCapacity)
{
  PhysicsWorld world(0);
  cyclone::RigidBody first_body;
  cyclone::CollisionSphere first_sphere;
  ConfigureBody(first_body, 1.0, cyclone::Vector3(0.0, 0.0, 0.0));
  ConfigureFixedBody(first_body);
  first_sphere.body = &first_body;
  first_sphere.radius = 1.0;
  cyclone::RigidBody second_body;
  cyclone::CollisionSphere second_sphere;
  ConfigureBody(second_body, 1.0, cyclone::Vector3(0.5, 0.0, 0.0));
  ConfigureFixedBody(second_body);
  second_sphere.body = &second_body;
  second_sphere.radius = 1.0;
  ASSERT_TRUE(world.RegisterSphere(TestId(0), first_body, first_sphere));
  ASSERT_TRUE(world.RegisterSphere(TestId(1), second_body, second_sphere));

  EXPECT_TRUE(world.Step(1.0f / 60.0f));
  EXPECT_NEAR(0.0, first_body.getPosition().x, 1e-6);
  EXPECT_NEAR(0.5, second_body.getPosition().x, 1e-6);
}

TEST(PhysicsWorldTest, AllowsUnregistrationBeforeBorrowedObjectsAreDestroyed)
{
  PhysicsWorld world;
  EntityId id = TestId(0);
  {
    cyclone::RigidBody body;
    cyclone::CollisionSphere sphere;
    ConfigureBody(body, 1.0, cyclone::Vector3(0.0, 2.0, 0.0));
    sphere.body = &body;
    sphere.radius = 0.5;
    ASSERT_TRUE(world.RegisterSphere(id, body, sphere));
    ASSERT_TRUE(world.UnregisterObject(id));
  }

  EXPECT_TRUE(world.Step(1.0f / 60.0f));
  EXPECT_EQ(0U, world.RegistrationCount());
}

TEST(PhysicsWorldTest, IntegratesDynamicBodiesButLeavesFixedBodiesUntouched)
{
  PhysicsWorld world;
  cyclone::RigidBody moving_body;
  cyclone::CollisionSphere moving_sphere;
  ConfigureSphere(
      moving_body, moving_sphere, cyclone::Vector3(0.0, 5.0, 0.0), 0.25);
  moving_body.setVelocity(2.0, 0.0, 0.0);
  cyclone::RigidBody fixed_body;
  cyclone::CollisionBox fixed_box;
  ConfigureBox(
      fixed_body, fixed_box, cyclone::Vector3(5.0, 5.0, 0.0),
      cyclone::Vector3(0.5, 0.5, 0.5));
  ConfigureFixedBody(fixed_body);
  fixed_body.setVelocity(3.0, 0.0, 0.0);
  ASSERT_TRUE(world.RegisterSphere(TestId(0), moving_body, moving_sphere));
  ASSERT_TRUE(world.RegisterBox(TestId(1), fixed_body, fixed_box));

  ASSERT_TRUE(world.Step(0.5f));

  EXPECT_NEAR(1.0, moving_body.getPosition().x, 1e-6);
  EXPECT_NEAR(5.0, fixed_body.getPosition().x, 1e-6);
  EXPECT_NEAR(1.0, moving_sphere.getTransform().data[3], 1e-6);
}

TEST(PhysicsWorldTest, ResolvesSphereSphereContactsTogether)
{
  PhysicsWorld world;
  cyclone::RigidBody first_body;
  cyclone::RigidBody second_body;
  cyclone::CollisionSphere first_sphere;
  cyclone::CollisionSphere second_sphere;
  ConfigureSphere(
      first_body, first_sphere, cyclone::Vector3(-0.4, 0.0, 0.0), 0.5);
  ConfigureSphere(
      second_body, second_sphere, cyclone::Vector3(0.4, 0.0, 0.0), 0.5);
  ASSERT_TRUE(world.RegisterSphere(TestId(0), first_body, first_sphere));
  ASSERT_TRUE(world.RegisterSphere(TestId(1), second_body, second_sphere));

  ASSERT_TRUE(world.Step(1.0f / 60.0f));

  EXPECT_GE(
      std::fabs(second_body.getPosition().x - first_body.getPosition().x),
      0.99);
}

}  // namespace siege_lab
