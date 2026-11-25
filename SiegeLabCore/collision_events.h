#ifndef SIEGE_LAB_CORE_COLLISION_EVENTS_H_
#define SIEGE_LAB_CORE_COLLISION_EVENTS_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include <cyclone/core.h>

#include "object_pool.h"

namespace siege_lab
{

struct CollisionPairKey
{
  EntityId first_id;
  EntityId second_id;

  bool operator==(const CollisionPairKey&) const = default;
};

struct ContactSnapshot
{
  CollisionPairKey pair;
  cyclone::Vector3 point;
  cyclone::Vector3 normal;
  cyclone::real closing_speed = 0.0;
};

enum class CollisionEventType
{
  kEnter,
  kExit
};

struct CollisionEvent
{
  CollisionEventType type = CollisionEventType::kEnter;
  ContactSnapshot contact;
  unsigned step_index = 0;
};

struct CollisionPairState
{
  CollisionPairKey pair;
  ContactSnapshot latest_contact;
  unsigned quiet_steps = 0;
  unsigned missing_steps = 0;
  bool active = false;
};

class ICollisionListener
{
 public:
  virtual ~ICollisionListener() = default;
  virtual void OnCollision(const CollisionEvent& _event) = 0;
};

class CollisionEventGenerator
{
 public:
  std::vector<CollisionEvent> Update(
      const std::vector<ContactSnapshot>& _contacts,
      bool _coverage_complete);
  void RemoveEntity(EntityId _id);
  void Clear();
  std::size_t StateCount() const;

 private:
   void UpdateObservedPairs(
       const std::vector<ContactSnapshot>& _contacts,
       std::vector<CollisionEvent>& _events);
   void UpdateMissingPairs(
       const std::vector<ContactSnapshot>& _contacts,
       bool _coverage_complete, std::vector<CollisionEvent>& _events);
   bool AdvanceMissingPair(
       std::size_t _index, std::vector<CollisionEvent>& _events);
   void AddState(
       const ContactSnapshot& _contact, std::vector<CollisionEvent>& _events);
   void UpdateActiveState(
       CollisionPairState& _state, const ContactSnapshot& _contact,
       std::vector<CollisionEvent>& _events);
   CollisionPairState* FindState(const CollisionPairKey& _pair);
   void RemovePair(const CollisionPairKey& _pair);
   bool ContainsPair(
       const std::vector<ContactSnapshot>& _contacts,
       const CollisionPairKey& _pair) const;

   std::vector<CollisionPairState> m_states;
   unsigned m_step_index = 0;
};

class CollisionEventDispatcher
{
 public:
  using ListenerId = std::uint64_t;

  ListenerId Subscribe(ICollisionListener& _listener);
  bool Unsubscribe(ListenerId _listener_id);
  void Dispatch(const std::vector<CollisionEvent>& _events);
  std::size_t ListenerCount() const;

 private:
  ICollisionListener* FindListener(ListenerId _listener_id) const;

  struct ListenerEntry
  {
    ListenerId id;
    ICollisionListener* listener;
  };

  std::vector<ListenerEntry> m_listeners;
  ListenerId m_next_listener_id = 1;
};

EntityId WorldGroundId();

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_COLLISION_EVENTS_H_
