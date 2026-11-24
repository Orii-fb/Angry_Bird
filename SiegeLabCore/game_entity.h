#ifndef SIEGE_LAB_CORE_GAME_ENTITY_H_
#define SIEGE_LAB_CORE_GAME_ENTITY_H_

namespace siege_lab
{

class GameEntity
{
 public:
  virtual ~GameEntity() = default;
  virtual void Draw() const = 0;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_GAME_ENTITY_H_
