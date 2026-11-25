#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "level_builder.h"

namespace siege_lab
{

namespace
{

class RecordingListener final : public ICollisionListener
{
 public:
  void OnCollision(const CollisionEvent& _event) override
  {
    events.push_back(_event);
  }

  std::vector<CollisionEvent> events;
};

class RemovalListener final : public ICollisionListener
{
 public:
  explicit RemovalListener(LevelBuilder& _level) : m_level(_level)
  {
  }

  void OnCollision(const CollisionEvent& _event) override
  {
    if (_event.type != CollisionEventType::kEnter || m_removed_id.has_value())
    {
      return;
    }
    const EntityId candidates[]{
        _event.contact.pair.first_id, _event.contact.pair.second_id};
    for (EntityId candidate : candidates)
    {
      if (m_level.QueueRemoval(candidate))
      {
        m_removed_id = candidate;
        m_entity_survived_callback = m_level.Pillar() != nullptr;
        return;
      }
    }
  }

  LevelBuilder& m_level;
  std::optional<EntityId> m_removed_id;
  bool m_entity_survived_callback = false;
};

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

unsigned CountTowerContacts(LevelBuilder& _level)
{
  cyclone::CollisionPlane ground;
  ground.direction = cyclone::Vector3(0.0, 1.0, 0.0);
  cyclone::Contact contacts[16];
  cyclone::CollisionData data{};
  data.contactArray = contacts;
  data.reset(16);
  data.friction = 0.5;
  data.restitution = 0.0;
  data.tolerance = 0.01;
  _level.Pillar()->Collider().calculateInternals();
  _level.Cube()->Collider().calculateInternals();
  cyclone::CollisionDetector::boxAndHalfSpace(
      _level.Pillar()->Collider(), ground, &data);
  cyclone::CollisionDetector::boxAndBox(
      _level.Pillar()->Collider(), _level.Cube()->Collider(), &data);
  return data.contactCount;
}

float PillarTiltDegrees(const PillarBlock& _pillar)
{
  const cyclone::Quaternion orientation = _pillar.Body().getOrientation();
  return static_cast<float>(
      2.0 * std::acos(std::min(1.0, std::fabs(orientation.r))) *
      180.0 / PI);
}

bool ProjectileHitPillar(const std::vector<CollisionEvent>& _events)
{
  return std::any_of(
      _events.begin(), _events.end(),
      [](const CollisionEvent& _event) {
        return _event.type == CollisionEventType::kEnter &&
               _event.contact.pair.first_id == EntityId{0, 1} &&
               _event.contact.pair.second_id ==
                   EntityId{
                       std::numeric_limits<std::size_t>::max() - 1, 1};
      });
}

}  // namespace

TEST(Entities, GO04_PillarAndCubeProfilesInImplementedScope)
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

TEST(Tower, CO01_RemainsStandingAtRest)
{
  LevelBuilder level;
  RecordingListener listener;
  ASSERT_TRUE(level.BuildPillarCube());
  ASSERT_NE(0U, level.Subscribe(listener));
  unsigned detected_contacts = 0;
  for (int step = 0; step < 780; ++step)
  {
    detected_contacts += CountTowerContacts(level);
    ASSERT_TRUE(level.Step(1.0f / 60.0f));
  }

  const cyclone::Vector3 pillar_position =
      level.Pillar()->Body().getPosition();
  const cyclone::Vector3 cube_position = level.Cube()->Body().getPosition();
  EXPECT_GT(detected_contacts, 0U);
  EXPECT_GT(cube_position.y, pillar_position.y);
  EXPECT_LE(std::hypot(pillar_position.x, pillar_position.z), 0.15);
  EXPECT_LE(std::hypot(cube_position.x, cube_position.z), 0.15);
  EXPECT_LE(PillarTiltDegrees(*level.Pillar()), 5.0f);
  EXPECT_NEAR(1.5, pillar_position.y, 0.10);
  EXPECT_NEAR(3.5, cube_position.y, 0.10);
  EXPECT_TRUE(listener.events.empty());
}

TEST(Tower, CO02_BoxesCollideWithEachOther)
{
  LevelBuilder level;
  RecordingListener listener;
  ASSERT_TRUE(level.BuildPillarCube());
  ASSERT_NE(0U, level.Subscribe(listener));
  level.Cube()->Body().setPosition(0.0, 4.5, 0.0);
  level.Cube()->Body().setVelocity(0.0, -2.0, 0.0);
  level.Cube()->Body().setAwake();
  level.Cube()->Body().calculateDerivedData();
  AdvanceLevel(level, 120);

  const cyclone::Vector3 pillar = level.Pillar()->Body().getPosition();
  const cyclone::Vector3 cube = level.Cube()->Body().getPosition();
  const bool pillar_cube_enter = std::any_of(
      listener.events.begin(), listener.events.end(),
      [](const CollisionEvent& _event) {
        return _event.type == CollisionEventType::kEnter &&
               _event.contact.pair.first_id == EntityId{0, 1} &&
               _event.contact.pair.second_id == EntityId{1, 1};
      });
  EXPECT_TRUE(pillar_cube_enter);
  EXPECT_GT(cube.y, pillar.y);
  EXPECT_GE(cube.y - pillar.y, 1.8);
}

TEST(CollisionEvent, EV18_RealRestingTowerDoesNotSpam)
{
  LevelBuilder level;
  ASSERT_TRUE(level.BuildPillarCube());
  AdvanceLevel(level, 180);
  level.ClearCollisionHistory();
  RecordingListener listener;
  ASSERT_NE(0U, level.Subscribe(listener));
  unsigned detected_contacts = 0;
  for (int step = 0; step < 600; ++step)
  {
    detected_contacts += CountTowerContacts(level);
    ASSERT_TRUE(level.Step(1.0f / 60.0f));
  }

  EXPECT_GT(detected_contacts, 0U);
  EXPECT_TRUE(listener.events.empty());
}

TEST(Tower, CO03_OffCenterHitProducesRotation)
{
  LevelBuilder level;
  RecordingListener listener;
  ASSERT_TRUE(level.BuildPillarCube());
  ASSERT_NE(0U, level.Subscribe(listener));
  AdvanceLevel(level, 780);

  StandardProjectile projectile;
  const float speed = std::sqrt(16.0f * 16.0f + 1.5f * 1.5f);
  const ShotSettings shot{
      {-8.0f, 1.1f, 0.0f}, {16.0f / speed, 1.5f / speed, 0.0f},
      speed, ProjectileKind::kStandard};
  projectile.Fire(shot);
  AdvanceLevel(level, 600, &projectile);

  EXPECT_GE(PillarTiltDegrees(*level.Pillar()), 15.0f);
  EXPECT_LE(level.Cube()->Body().getPosition().y, 1.0);
  EXPECT_TRUE(ProjectileHitPillar(listener.events));
}

TEST(LevelBuilderTest, DefersEntityDestructionUntilAfterCollisionDispatch)
{
  LevelBuilder level;
  ASSERT_TRUE(level.BuildPillarCube());
  AdvanceLevel(level, 780);
  RemovalListener listener(level);
  ASSERT_NE(0U, level.Subscribe(listener));
  StandardProjectile projectile;
  const float angle = 9.0f * PI / 180.0f;
  projectile.Fire({
      {-8.0f, 0.8f, 0.0f}, {std::cos(angle), std::sin(angle), 0.0f},
      16.0f, ProjectileKind::kStandard});

  AdvanceLevel(level, 180, &projectile);

  ASSERT_TRUE(listener.m_removed_id.has_value());
  EXPECT_TRUE(listener.m_entity_survived_callback);
  EXPECT_TRUE(
      level.Pillar() == nullptr || level.Cube() == nullptr);
}

}  // namespace siege_lab
