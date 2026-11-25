#include "level_builder.h"

#include <array>
#include <limits>

namespace siege_lab
{

namespace
{

constexpr EntityId kProjectileEntityId{
    std::numeric_limits<std::size_t>::max() - 1, 1};

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
  UnregisterCurrentEntities();
  m_entities.Clear();
  m_pillar_id = m_entities.CreateEntity<PillarBlock>();
  m_cube_id = m_entities.CreateEntity<CubeBlock>();
  if (!m_pillar_id.has_value() || !m_cube_id.has_value())
  {
    return false;
  }
  return RegisterCurrentEntities();
}

void LevelBuilder::UnregisterCurrentEntities()
{
  if (m_pillar_id.has_value())
  {
    m_physics_world.UnregisterObject(*m_pillar_id);
  }
  if (m_cube_id.has_value())
  {
    m_physics_world.UnregisterObject(*m_cube_id);
  }
}

bool LevelBuilder::RegisterCurrentEntities()
{
  if (!m_ground_registered)
  {
    m_ground_registered = m_physics_world.RegisterPlane(m_ground_plane);
  }
  Block* pillar = Pillar();
  Block* cube = Cube();
  return m_ground_registered &&
         m_physics_world.RegisterBox(
             *m_pillar_id, pillar->Body(), pillar->Collider()) &&
         m_physics_world.RegisterBox(
             *m_cube_id, cube->Body(), cube->Collider());
}

bool LevelBuilder::Step(float _delta_time, Projectile* _projectile)
{
  if (!HasBlocks())
  {
    FlushQueuedEntities();
    return true;
  }
  if (!RegisterProjectile(_projectile))
  {
    FlushQueuedEntities();
    return false;
  }
  const bool complete = m_physics_world.Step(_delta_time);
  UnregisterProjectile(_projectile);
  FlushQueuedEntities();
  return complete;
}

bool LevelBuilder::HasBlocks() const
{
  return Pillar() != nullptr && Cube() != nullptr;
}

bool LevelBuilder::RegisterProjectile(Projectile* _projectile)
{
  return _projectile == nullptr ||
         m_physics_world.RegisterSphere(
             kProjectileEntityId, _projectile->Body(),
             _projectile->Collider());
}

void LevelBuilder::UnregisterProjectile(Projectile* _projectile)
{
  if (_projectile != nullptr)
  {
    m_physics_world.UnregisterObject(kProjectileEntityId);
  }
}

bool LevelBuilder::QueueRemoval(EntityId _id)
{
  return m_entities.QueueDestroy(_id);
}

void LevelBuilder::ClearCollisionHistory()
{
  m_physics_world.ClearCollisionHistory();
}

void LevelBuilder::FlushQueuedEntities()
{
  m_entities.FlushRemovals([this](EntityId _id) {
    m_physics_world.UnregisterObject(_id);
    if (m_pillar_id == _id)
    {
      m_pillar_id.reset();
    }
    if (m_cube_id == _id)
    {
      m_cube_id.reset();
    }
  });
}

CollisionEventDispatcher::ListenerId LevelBuilder::Subscribe(
    ICollisionListener& _listener)
{
  return m_physics_world.Subscribe(_listener);
}

bool LevelBuilder::Unsubscribe(
    CollisionEventDispatcher::ListenerId _listener_id)
{
  return m_physics_world.Unsubscribe(_listener_id);
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
