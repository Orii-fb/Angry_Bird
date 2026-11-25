#ifndef SIEGE_LAB_CORE_LEVEL_BUILDER_H_
#define SIEGE_LAB_CORE_LEVEL_BUILDER_H_

#include <array>
#include <optional>

#include <cyclone/collide_fine.h>

#include "block.h"
#include "projectile.h"
#include "scene_objects.h"
#include "trajectory_predictor.h"

namespace siege_lab
{

class LevelBuilder
{
 public:
  bool BuildPillarCube();
  void Step(float _delta_time, Projectile* _projectile = nullptr);
  void Draw() const;

  const PillarBlock* Pillar() const;
  PillarBlock* Pillar();
  const CubeBlock* Cube() const;
  CubeBlock* Cube();
  std::array<TrajectoryBox, 2> GetTrajectoryBoxes() const;

 private:
  SceneObjects<2> m_entities;
  std::optional<EntityId> m_pillar_id;
  std::optional<EntityId> m_cube_id;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_LEVEL_BUILDER_H_
