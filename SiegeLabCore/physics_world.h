#ifndef SIEGE_LAB_CORE_PHYSICS_WORLD_H_
#define SIEGE_LAB_CORE_PHYSICS_WORLD_H_

#include <cstddef>
#include <optional>
#include <vector>

#include <cyclone/collide_fine.h>

#include "collision_events.h"
#include "object_pool.h"

namespace siege_lab
{

struct PhysicsRegistration
{
  EntityId id;
  cyclone::RigidBody* body = nullptr;
  cyclone::CollisionSphere* sphere = nullptr;
  cyclone::CollisionBox* box = nullptr;
};

class PhysicsWorld
{
 public:
  explicit PhysicsWorld(std::size_t _maximum_contacts = 256);

  bool RegisterSphere(
      EntityId _id, cyclone::RigidBody& _body,
      cyclone::CollisionSphere& _sphere);
  bool RegisterBox(
      EntityId _id, cyclone::RigidBody& _body,
      cyclone::CollisionBox& _box);
  bool RegisterPlane(const cyclone::CollisionPlane& _plane);
  bool UnregisterObject(EntityId _id);
  bool UnregisterPlane(const cyclone::CollisionPlane& _plane);
  bool Step(float _delta_time);
  CollisionEventDispatcher::ListenerId Subscribe(
      ICollisionListener& _listener);
  bool Unsubscribe(CollisionEventDispatcher::ListenerId _listener_id);
  void ClearCollisionHistory();

  std::size_t RegistrationCount() const;

 private:
  bool HasId(EntityId _id) const;
  bool DetectContacts(cyclone::CollisionData& _data);
  bool DetectSpherePairs(cyclone::CollisionData& _data);
  bool DetectBoxSpherePairs(cyclone::CollisionData& _data);
  bool DetectBoxPairs(cyclone::CollisionData& _data);
  bool DetectPlaneContacts(cyclone::CollisionData& _data);
  void DetectPlaneContact(
      const PhysicsRegistration& _registration,
      const cyclone::CollisionPlane& _plane,
      cyclone::CollisionData& _data, bool& _complete);
  cyclone::CollisionData CreateCollisionData();
  void ResolveContacts(cyclone::CollisionData& _data, float _delta_time);
  void PublishEvents(bool _coverage_complete);
  static void CapturePreparedContacts(
      const cyclone::Contact* _contacts, unsigned _count, void* _world);
  void CopyPreparedContacts(
      const cyclone::Contact* _contacts, unsigned _count);
  void AppendPreparedContact(const cyclone::Contact& _contact);
  std::optional<EntityId> FindEntityId(
      const cyclone::RigidBody* _body) const;
  void IntegrateBodies(float _delta_time);
  void RefreshColliders();

  std::vector<PhysicsRegistration> m_registrations;
  std::vector<const cyclone::CollisionPlane*> m_planes;
  std::vector<cyclone::Contact> m_contacts;
  std::vector<ContactSnapshot> m_contact_snapshots;
  CollisionEventGenerator m_event_generator;
  CollisionEventDispatcher m_event_dispatcher;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_PHYSICS_WORLD_H_
