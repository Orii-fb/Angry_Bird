#ifndef SIEGE_LAB_CORE_SCENE_OBJECTS_H_
#define SIEGE_LAB_CORE_SCENE_OBJECTS_H_

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "game_entity.h"
#include "object_pool.h"

namespace siege_lab
{

template <std::size_t Capacity>
class SceneObjects
{
 public:
  template <typename EntityType, typename... ArgumentTypes>
    requires std::derived_from<EntityType, GameEntity> &&
             (!std::is_abstract_v<EntityType>) &&
             std::constructible_from<EntityType, ArgumentTypes...>
  std::optional<EntityId> CreateEntity(ArgumentTypes&&... _arguments)
  {
    std::unique_ptr<GameEntity> entity =
        std::make_unique<EntityType>(
            std::forward<ArgumentTypes>(_arguments)...);
    return m_entities.Create(std::move(entity));
  }

  GameEntity* Get(EntityId _id)
  {
    std::unique_ptr<GameEntity>* entity = m_entities.Get(_id);
    return entity == nullptr ? nullptr : entity->get();
  }

  const GameEntity* Get(EntityId _id) const
  {
    const std::unique_ptr<GameEntity>* entity = m_entities.Get(_id);
    return entity == nullptr ? nullptr : entity->get();
  }

  bool Destroy(EntityId _id)
  {
    return m_entities.Destroy(_id);
  }

  template <typename FunctionType>
  void ForEach(FunctionType&& _function)
  {
    m_entities.ForEach(
        [&_function](EntityId _id, std::unique_ptr<GameEntity>& _entity) {
          std::invoke(_function, _id, *_entity);
        });
  }

  template <typename FunctionType>
  void ForEach(FunctionType&& _function) const
  {
    m_entities.ForEach(
        [&_function](
            EntityId _id, const std::unique_ptr<GameEntity>& _entity) {
          std::invoke(_function, _id, *_entity);
        });
  }

  void Clear()
  {
    m_entities.Clear();
  }

 private:
  ObjectPool<std::unique_ptr<GameEntity>, Capacity> m_entities;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_SCENE_OBJECTS_H_
