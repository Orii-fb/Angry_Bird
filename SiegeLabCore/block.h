#ifndef SIEGE_LAB_CORE_BLOCK_H_
#define SIEGE_LAB_CORE_BLOCK_H_

#include <cyclone/collide_fine.h>

#include "game_entity.h"

namespace siege_lab
{

class Block : public GameEntity
{
 public:
  void Draw() const override;
  void RegisterPhysics() override;

  const cyclone::CollisionBox& Collider() const;
  cyclone::CollisionBox& Collider();

 protected:
  Block(
      float _mass, cyclone::Vector3 _half_size, Vector3 _position,
      Color _color);

 private:
  cyclone::CollisionBox m_box;
  Color m_color;
};

class PillarBlock final : public Block
{
 public:
  PillarBlock();
};

class CubeBlock final : public Block
{
 public:
  CubeBlock();
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_BLOCK_H_
