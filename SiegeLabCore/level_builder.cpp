#include "level_builder.h"

#include <array>

#include <cyclone/contacts.h>

namespace siege_lab
{

namespace
{

constexpr cyclone::real kGroundFriction = 0.5;
constexpr cyclone::real kRestitution = 0.0;
constexpr unsigned kSolverIterations = 100;
constexpr unsigned kMaximumContacts = 32;

void ConfigureCollisionData(
    cyclone::CollisionData& _data,
    std::array<cyclone::Contact, kMaximumContacts>& _contacts)
{
  _data.contactArray = _contacts.data();
  _data.friction = kGroundFriction;
  _data.restitution = kRestitution;
  _data.tolerance = 0.01;
  _data.reset(kMaximumContacts);
}

void AddGroundContacts(
    const cyclone::CollisionBox& _box, cyclone::CollisionData& _data)
{
  cyclone::CollisionPlane ground;
  ground.direction = cyclone::Vector3(0.0, 1.0, 0.0);
  ground.offset = 0.0;
  cyclone::CollisionDetector::boxAndHalfSpace(_box, ground, &_data);
}

void AddBlockContacts(
    Block& _pillar, Block& _cube, cyclone::CollisionData& _data)
{
  cyclone::CollisionDetector::boxAndBox(
      _pillar.Collider(), _cube.Collider(), &_data);
}

void AddProjectileContacts(
    Projectile* _projectile, Block& _pillar, Block& _cube,
    cyclone::CollisionData& _data)
{
  if (_projectile == nullptr)
  {
    return;
  }
  cyclone::CollisionPlane ground;
  ground.direction = cyclone::Vector3(0.0, 1.0, 0.0);
  ground.offset = 0.0;
  cyclone::CollisionDetector::sphereAndHalfSpace(
      _projectile->Collider(), ground, &_data);
  cyclone::CollisionDetector::boxAndSphere(
      _pillar.Collider(), _projectile->Collider(), &_data);
  cyclone::CollisionDetector::boxAndSphere(
      _cube.Collider(), _projectile->Collider(), &_data);
}

void IntegrateProjectile(Projectile* _projectile, float _delta_time)
{
  if (_projectile == nullptr)
  {
    return;
  }
  _projectile->Body().integrate(_delta_time);
  _projectile->RegisterPhysics();
}

void IntegrateEntities(
    SceneObjects<2>& _entities, float _delta_time)
{
  _entities.ForEach([_delta_time](EntityId, GameEntity& _entity) {
    _entity.Body().integrate(_delta_time);
    _entity.RegisterPhysics();
  });
}

void ResolveWorldContacts(
    Block& _pillar, Block& _cube, Projectile* _projectile,
    float _delta_time)
{
  std::array<cyclone::Contact, kMaximumContacts> contacts;
  cyclone::CollisionData data;
  ConfigureCollisionData(data, contacts);
  AddGroundContacts(_pillar.Collider(), data);
  AddGroundContacts(_cube.Collider(), data);
  AddBlockContacts(_pillar, _cube, data);
  AddProjectileContacts(_projectile, _pillar, _cube, data);

  if (data.contactCount == 0)
  {
    return;
  }
  cyclone::ContactResolver resolver(kSolverIterations);
  resolver.resolveContacts(contacts.data(), data.contactCount, _delta_time);
  _pillar.RegisterPhysics();
  _cube.RegisterPhysics();
  if (_projectile != nullptr)
  {
    _projectile->RegisterPhysics();
  }
}

void DrawEntities(const SceneObjects<2>& _entities)
{
  _entities.ForEach([](EntityId, const GameEntity& _entity) {
    _entity.Draw();
  });
}

TrajectoryBox CreateTrajectoryBox(const Block& _block)
{
  const cyclone::Vector3 position = _block.Body().getPosition();
  const cyclone::Quaternion orientation = _block.Body().getOrientation();
  const cyclone::Vector3 half_size = _block.Collider().halfSize;
  const ::Quaternion rotation{
      static_cast<float>(orientation.i), static_cast<float>(orientation.j),
      static_cast<float>(orientation.k), static_cast<float>(orientation.r)};
  return {
      ::Vector3{
          static_cast<float>(position.x), static_cast<float>(position.y),
          static_cast<float>(position.z)},
      ::Vector3{
          static_cast<float>(half_size.x), static_cast<float>(half_size.y),
          static_cast<float>(half_size.z)}, rotation};
}

}  // namespace

bool LevelBuilder::BuildPillarCube()
{
  m_entities.Clear();
  m_pillar_id = m_entities.CreateEntity<PillarBlock>();
  m_cube_id = m_entities.CreateEntity<CubeBlock>();
  return m_pillar_id.has_value() && m_cube_id.has_value();
}

void LevelBuilder::Step(float _delta_time, Projectile* _projectile)
{
  Block* pillar = Pillar();
  Block* cube = Cube();
  if (pillar == nullptr || cube == nullptr)
  {
    return;
  }

  IntegrateEntities(m_entities, _delta_time);
  IntegrateProjectile(_projectile, _delta_time);
  ResolveWorldContacts(*pillar, *cube, _projectile, _delta_time);
}

void LevelBuilder::Draw() const
{
  DrawEntities(m_entities);
}

const PillarBlock* LevelBuilder::Pillar() const
{
  if (!m_pillar_id.has_value())
  {
    return nullptr;
  }
  return static_cast<const PillarBlock*>(m_entities.Get(*m_pillar_id));
}

PillarBlock* LevelBuilder::Pillar()
{
  if (!m_pillar_id.has_value())
  {
    return nullptr;
  }
  return static_cast<PillarBlock*>(m_entities.Get(*m_pillar_id));
}

const CubeBlock* LevelBuilder::Cube() const
{
  if (!m_cube_id.has_value())
  {
    return nullptr;
  }
  return static_cast<const CubeBlock*>(m_entities.Get(*m_cube_id));
}

CubeBlock* LevelBuilder::Cube()
{
  if (!m_cube_id.has_value())
  {
    return nullptr;
  }
  return static_cast<CubeBlock*>(m_entities.Get(*m_cube_id));
}

std::array<TrajectoryBox, 2> LevelBuilder::GetTrajectoryBoxes() const
{
  const PillarBlock* pillar = Pillar();
  const CubeBlock* cube = Cube();
  if (pillar == nullptr || cube == nullptr)
  {
    return {};
  }
  return {CreateTrajectoryBox(*pillar), CreateTrajectoryBox(*cube)};
}

}  // namespace siege_lab
