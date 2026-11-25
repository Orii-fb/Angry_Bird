#ifndef SIEGE_LAB_CORE_SCENE_OBJECTS_H_
#define SIEGE_LAB_CORE_SCENE_OBJECTS_H_

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

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

  bool Contains(const GameEntity* _entity) const
  {
    bool found = false;
    ForEach([_entity, &found](EntityId, const GameEntity& _candidate) {
      found = found || &_candidate == _entity;
    });
    return found;
  }

  bool Destroy(EntityId _id)
  {
    RemovePending(_id);
    return m_entities.Destroy(_id);
  }

  bool QueueDestroy(EntityId _id)
  {
    if (Get(_id) == nullptr)
    {
      return false;
    }
    if (std::find(m_pending_removals.begin(), m_pending_removals.end(), _id) ==
        m_pending_removals.end())
    {
      m_pending_removals.push_back(_id);
    }
    return true;
  }

  void FlushRemovals()
  {
    FlushRemovals([](EntityId) {});
  }

  template <typename FunctionType>
  void FlushRemovals(FunctionType&& _before_destroy)
  {
    std::vector<EntityId> removals;
    removals.swap(m_pending_removals);
    for (EntityId id : removals)
    {
      std::invoke(_before_destroy, id);
      m_entities.Destroy(id);
    }
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
    m_pending_removals.clear();
    m_entities.Clear();
  }

 private:
  void RemovePending(EntityId _id)
  {
    m_pending_removals.erase(
        std::remove(m_pending_removals.begin(), m_pending_removals.end(), _id),
        m_pending_removals.end());
  }

  ObjectPool<std::unique_ptr<GameEntity>, Capacity> m_entities;
  std::vector<EntityId> m_pending_removals;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_SCENE_OBJECTS_H_
