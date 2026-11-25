#include "physics_world.h"

#include <algorithm>
#include <optional>

#include <cyclone/contacts.h>

namespace siege_lab
{

namespace
{

constexpr cyclone::real kContactFriction = 0.5;
constexpr cyclone::real kContactRestitution = 0.0;
constexpr cyclone::real kContactTolerance = 0.01;
constexpr unsigned kSolverIterations = 100;

bool IsIdBefore(EntityId _one, EntityId _two)
{
  return _one.slot < _two.slot ||
         (_one.slot == _two.slot && _one.generation < _two.generation);
}

bool CanGenerateContact(
    cyclone::CollisionData& _data, bool& _complete)
{
  if (_data.hasMoreContacts())
  {
    return true;
  }
  _complete = false;
  return false;
}

bool CanGenerateBoxPlaneContact(
    cyclone::CollisionData& _data, bool& _complete)
{
  constexpr int kMaximumBoxPlaneContacts = 8;
  if (_data.contactsLeft >= kMaximumBoxPlaneContacts)
  {
    return true;
  }
  _complete = false;
  return false;
}

bool CanCollide(
    const PhysicsRegistration& _one, const PhysicsRegistration& _two)
{
  return _one.body != _two.body &&
         (_one.body->getInverseMass() > 0.0 ||
          _two.body->getInverseMass() > 0.0);
}

}  // namespace

PhysicsWorld::PhysicsWorld(std::size_t _maximum_contacts)
    : m_contacts(_maximum_contacts)
{
}

bool PhysicsWorld::RegisterSphere(
    EntityId _id, cyclone::RigidBody& _body,
    cyclone::CollisionSphere& _sphere)
{
  if (_id.generation == 0 || _id == WorldGroundId() || HasId(_id))
  {
    return false;
  }
  _sphere.body = &_body;
  m_registrations.push_back({_id, &_body, &_sphere, nullptr});
  return true;
}

bool PhysicsWorld::RegisterBox(
    EntityId _id, cyclone::RigidBody& _body, cyclone::CollisionBox& _box)
{
  if (_id.generation == 0 || _id == WorldGroundId() || HasId(_id))
  {
    return false;
  }
  _box.body = &_body;
  m_registrations.push_back({_id, &_body, nullptr, &_box});
  return true;
}

bool PhysicsWorld::RegisterPlane(const cyclone::CollisionPlane& _plane)
{
  if (std::find(m_planes.begin(), m_planes.end(), &_plane) != m_planes.end())
  {
    return false;
  }
  m_planes.push_back(&_plane);
  return true;
}

bool PhysicsWorld::UnregisterObject(EntityId _id)
{
  const auto registration = std::find_if(
      m_registrations.begin(), m_registrations.end(),
      [_id](const PhysicsRegistration& _entry) { return _entry.id == _id; });
  if (registration == m_registrations.end())
  {
    return false;
  }
  m_event_generator.RemoveEntity(_id);
  m_registrations.erase(registration);
  return true;
}

bool PhysicsWorld::UnregisterPlane(const cyclone::CollisionPlane& _plane)
{
  const auto plane = std::find(m_planes.begin(), m_planes.end(), &_plane);
  if (plane == m_planes.end())
  {
    return false;
  }
  m_planes.erase(plane);
  m_event_generator.RemoveEntity(WorldGroundId());
  return true;
}

bool PhysicsWorld::Step(float _delta_time)
{
  IntegrateBodies(_delta_time);
  RefreshColliders();
  cyclone::CollisionData data = CreateCollisionData();
  m_contact_snapshots.clear();
  const bool complete = DetectContacts(data);
  ResolveContacts(data, _delta_time);
  RefreshColliders();
  PublishEvents(complete);
  return complete;
}

cyclone::CollisionData PhysicsWorld::CreateCollisionData()
{
  cyclone::CollisionData data{};
  data.contactArray = m_contacts.data();
  data.friction = kContactFriction;
  data.restitution = kContactRestitution;
  data.tolerance = kContactTolerance;
  data.reset(static_cast<unsigned>(m_contacts.size()));
  return data;
}

void PhysicsWorld::ResolveContacts(
    cyclone::CollisionData& _data, float _delta_time)
{
  if (_data.contactCount == 0)
  {
    return;
  }
  cyclone::ContactResolver resolver(kSolverIterations);
  resolver.setIterations(kSolverIterations, kSolverIterations);
  resolver.setPreparedContactObserver(CapturePreparedContacts, this);
  resolver.resolveContacts(m_contacts.data(), _data.contactCount, _delta_time);
}

void PhysicsWorld::PublishEvents(bool _coverage_complete)
{
  const std::vector<CollisionEvent> events =
      m_event_generator.Update(m_contact_snapshots, _coverage_complete);
  m_event_dispatcher.Dispatch(events);
}

CollisionEventDispatcher::ListenerId PhysicsWorld::Subscribe(
    ICollisionListener& _listener)
{
  return m_event_dispatcher.Subscribe(_listener);
}

bool PhysicsWorld::Unsubscribe(
    CollisionEventDispatcher::ListenerId _listener_id)
{
  return m_event_dispatcher.Unsubscribe(_listener_id);
}

void PhysicsWorld::ClearCollisionHistory()
{
  m_event_generator.Clear();
  m_contact_snapshots.clear();
}

std::size_t PhysicsWorld::RegistrationCount() const
{
  return m_registrations.size();
}

bool PhysicsWorld::HasId(EntityId _id) const
{
  return std::any_of(
      m_registrations.begin(), m_registrations.end(),
      [_id](const PhysicsRegistration& _entry) { return _entry.id == _id; });
}

void PhysicsWorld::CapturePreparedContacts(
    const cyclone::Contact* _contacts, unsigned _count, void* _world)
{
  PhysicsWorld* world = static_cast<PhysicsWorld*>(_world);
  world->CopyPreparedContacts(_contacts, _count);
}

void PhysicsWorld::CopyPreparedContacts(
    const cyclone::Contact* _contacts, unsigned _count)
{
  for (unsigned index = 0; index < _count; ++index)
  {
    AppendPreparedContact(_contacts[index]);
  }
}

void PhysicsWorld::AppendPreparedContact(const cyclone::Contact& _contact)
{
  const std::optional<EntityId> body_id = FindEntityId(_contact.body[0]);
  const std::optional<EntityId> other_id =
      _contact.body[1] == nullptr
          ? std::optional<EntityId>(WorldGroundId())
          : FindEntityId(_contact.body[1]);
  if (!body_id.has_value() || !other_id.has_value())
  {
    return;
  }
  ContactSnapshot snapshot{
      {*body_id, *other_id}, _contact.contactPoint, _contact.contactNormal,
      std::max(0.0, _contact.getClosingSpeed())};
  if (!IsIdBefore(snapshot.pair.first_id, snapshot.pair.second_id))
  {
    std::swap(snapshot.pair.first_id, snapshot.pair.second_id);
    snapshot.normal *= -1.0;
  }
  m_contact_snapshots.push_back(snapshot);
}

std::optional<EntityId> PhysicsWorld::FindEntityId(
    const cyclone::RigidBody* _body) const
{
  const auto registration = std::find_if(
      m_registrations.begin(), m_registrations.end(),
      [_body](const PhysicsRegistration& _entry) {
        return _entry.body == _body;
      });
  if (registration == m_registrations.end())
  {
    return std::nullopt;
  }
  return registration->id;
}

bool PhysicsWorld::DetectContacts(cyclone::CollisionData& _data)
{
  bool complete = DetectSpherePairs(_data);
  complete = DetectBoxSpherePairs(_data) && complete;
  complete = DetectBoxPairs(_data) && complete;
  complete = DetectPlaneContacts(_data) && complete;
  return complete;
}

bool PhysicsWorld::DetectSpherePairs(cyclone::CollisionData& _data)
{
  bool complete = true;
  for (std::size_t one = 0; one < m_registrations.size(); ++one)
  {
    const PhysicsRegistration& first = m_registrations[one];
    if (first.sphere == nullptr)
    {
      continue;
    }
    for (std::size_t two = one + 1; two < m_registrations.size(); ++two)
    {
      const PhysicsRegistration& second = m_registrations[two];
      if (second.sphere != nullptr && CanCollide(first, second) &&
          CanGenerateContact(_data, complete))
      {
        cyclone::CollisionDetector::sphereAndSphere(
            *first.sphere, *second.sphere, &_data);
      }
    }
  }
  return complete;
}

bool PhysicsWorld::DetectBoxSpherePairs(cyclone::CollisionData& _data)
{
  bool complete = true;
  for (const PhysicsRegistration& box : m_registrations)
  {
    if (box.box == nullptr)
    {
      continue;
    }
    for (const PhysicsRegistration& sphere : m_registrations)
    {
      if (sphere.sphere != nullptr && CanCollide(box, sphere) &&
          CanGenerateContact(_data, complete))
      {
        cyclone::CollisionDetector::boxAndSphere(
            *box.box, *sphere.sphere, &_data);
      }
    }
  }
  return complete;
}

bool PhysicsWorld::DetectBoxPairs(cyclone::CollisionData& _data)
{
  bool complete = true;
  for (std::size_t one = 0; one < m_registrations.size(); ++one)
  {
    const PhysicsRegistration& first = m_registrations[one];
    if (first.box == nullptr)
    {
      continue;
    }
    for (std::size_t two = one + 1; two < m_registrations.size(); ++two)
    {
      const PhysicsRegistration& second = m_registrations[two];
      if (second.box != nullptr && CanCollide(first, second) &&
          CanGenerateContact(_data, complete))
      {
        cyclone::CollisionDetector::boxAndBox(
            *first.box, *second.box, &_data);
      }
    }
  }
  return complete;
}

bool PhysicsWorld::DetectPlaneContacts(cyclone::CollisionData& _data)
{
  bool complete = true;
  for (const PhysicsRegistration& registration : m_registrations)
  {
    for (const cyclone::CollisionPlane* plane : m_planes)
    {
      DetectPlaneContact(registration, *plane, _data, complete);
    }
  }
  return complete;
}

void PhysicsWorld::DetectPlaneContact(
    const PhysicsRegistration& _registration,
    const cyclone::CollisionPlane& _plane,
    cyclone::CollisionData& _data, bool& _complete)
{
  const bool enough_capacity =
      _registration.box != nullptr
          ? CanGenerateBoxPlaneContact(_data, _complete)
          : CanGenerateContact(_data, _complete);
  if (!enough_capacity)
  {
    return;
  }
  if (_registration.sphere != nullptr)
  {
    cyclone::CollisionDetector::sphereAndHalfSpace(
        *_registration.sphere, _plane, &_data);
  }
  else
  {
    cyclone::CollisionDetector::boxAndHalfSpace(
        *_registration.box, _plane, &_data);
  }
}

void PhysicsWorld::IntegrateBodies(float _delta_time)
{
  for (const PhysicsRegistration& registration : m_registrations)
  {
    if (registration.body->getInverseMass() > 0.0)
    {
      registration.body->integrate(_delta_time);
    }
  }
}

void PhysicsWorld::RefreshColliders()
{
  for (const PhysicsRegistration& registration : m_registrations)
  {
    registration.body->calculateDerivedData();
    if (registration.sphere != nullptr)
    {
      registration.sphere->calculateInternals();
    }
    else
    {
      registration.box->calculateInternals();
    }
  }
}

}  // namespace siege_lab
