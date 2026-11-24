#ifndef SIEGE_LAB_CORE_GAME_ENTITY_H_
#define SIEGE_LAB_CORE_GAME_ENTITY_H_

#include <cyclone/body.h>

#include "raylib.h"

namespace siege_lab
{

class GameEntity
{
 public:
  virtual ~GameEntity() = default;
  virtual void Draw() const = 0;
  virtual void RegisterPhysics() = 0;

  cyclone::RigidBody& Body()
  {
    return m_body;
  }

  const cyclone::RigidBody& Body() const
  {
    return m_body;
  }

 private:
  cyclone::RigidBody m_body;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_GAME_ENTITY_H_
