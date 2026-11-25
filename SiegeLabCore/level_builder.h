#ifndef SIEGE_LAB_CORE_LEVEL_BUILDER_H_
#define SIEGE_LAB_CORE_LEVEL_BUILDER_H_

#include <array>
#include <optional>

#include <cyclone/collide_fine.h>

#include "block.h"
#include "physics_world.h"
#include "projectile.h"
#include "scene_objects.h"
#include "trajectory_predictor.h"

namespace siege_lab
{

class LevelBuilder
{
 public:
  bool BuildPillarCube();
  bool Step(float _delta_time, Projectile* _projectile = nullptr);
  bool QueueRemoval(EntityId _id);
  void ClearCollisionHistory();
  CollisionEventDispatcher::ListenerId Subscribe(
      ICollisionListener& _listener);
  bool Unsubscribe(CollisionEventDispatcher::ListenerId _listener_id);
  void Draw() const;

  const PillarBlock* Pillar() const;
  PillarBlock* Pillar();
  const CubeBlock* Cube() const;
  CubeBlock* Cube();
  std::array<TrajectoryBox, 2> GetTrajectoryBoxes() const;

 private:
  void UnregisterCurrentEntities();
  bool RegisterCurrentEntities();
  bool HasBlocks() const;
  bool RegisterProjectile(Projectile* _projectile);
  void UnregisterProjectile(Projectile* _projectile);
  void FlushQueuedEntities();

  SceneObjects<2> m_entities;
  std::optional<EntityId> m_pillar_id;
  std::optional<EntityId> m_cube_id;
  PhysicsWorld m_physics_world;
  cyclone::CollisionPlane m_ground_plane{
      cyclone::Vector3(0.0, 1.0, 0.0), 0.0};
  bool m_ground_registered = false;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_LEVEL_BUILDER_H_
