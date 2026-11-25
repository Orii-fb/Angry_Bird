#include "collision_events.h"

#include <algorithm>
#include <limits>

namespace siege_lab
{

namespace
{

constexpr cyclone::real kEnterSpeed = 1.0;
constexpr cyclone::real kExitSpeed = 0.2;
constexpr unsigned kQuietStepsToExit = 3;
constexpr unsigned kMissingStepsToExit = 2;

bool IsBefore(EntityId _one, EntityId _two)
{
  return _one.slot < _two.slot ||
         (_one.slot == _two.slot && _one.generation < _two.generation);
}

ContactSnapshot Canonicalize(const ContactSnapshot& _contact)
{
  ContactSnapshot result = _contact;
  if (!IsBefore(result.pair.first_id, result.pair.second_id))
  {
    std::swap(result.pair.first_id, result.pair.second_id);
    result.normal *= -1.0;
  }
  result.closing_speed = std::max(0.0, result.closing_speed);
  return result;
}

std::vector<ContactSnapshot> AggregateContacts(
    const std::vector<ContactSnapshot>& _contacts)
{
  std::vector<ContactSnapshot> result;
  for (const ContactSnapshot& contact : _contacts)
  {
    const ContactSnapshot canonical = Canonicalize(contact);
    const auto existing = std::find_if(
        result.begin(), result.end(),
        [&canonical](const ContactSnapshot& _entry) {
          return _entry.pair == canonical.pair;
        });
    if (existing == result.end())
    {
      result.push_back(canonical);
    }
    else if (canonical.closing_speed > existing->closing_speed)
    {
      *existing = canonical;
    }
  }
  return result;
}

}  // namespace

std::vector<CollisionEvent> CollisionEventGenerator::Update(
    const std::vector<ContactSnapshot>& _contacts, bool _coverage_complete)
{
  ++m_step_index;
  const std::vector<ContactSnapshot> contacts = AggregateContacts(_contacts);
  std::vector<CollisionEvent> events;
  UpdateObservedPairs(contacts, events);
  UpdateMissingPairs(contacts, _coverage_complete, events);
  return events;
}

void CollisionEventGenerator::UpdateObservedPairs(
    const std::vector<ContactSnapshot>& _contacts,
    std::vector<CollisionEvent>& _events)
{
  for (const ContactSnapshot& contact : _contacts)
  {
    CollisionPairState* state = FindState(contact.pair);
    if (state == nullptr)
    {
      if (contact.closing_speed >= kEnterSpeed)
      {
        AddState(contact, _events);
      }
      continue;
    }
    UpdateActiveState(*state, contact, _events);
  }
}

void CollisionEventGenerator::UpdateMissingPairs(
    const std::vector<ContactSnapshot>& _contacts, bool _coverage_complete,
    std::vector<CollisionEvent>& _events)
{
  if (!_coverage_complete)
  {
    return;
  }
  for (std::size_t index = 0; index < m_states.size();)
  {
    if (ContainsPair(_contacts, m_states[index].pair))
    {
      ++index;
      continue;
    }
    if (!AdvanceMissingPair(index, _events))
    {
      ++index;
    }
  }
}

bool CollisionEventGenerator::AdvanceMissingPair(
    std::size_t _index, std::vector<CollisionEvent>& _events)
{
  CollisionPairState& state = m_states[_index];
  if (++state.missing_steps < kMissingStepsToExit)
  {
    return false;
  }
  _events.push_back(
      {CollisionEventType::kExit, state.latest_contact, m_step_index});
  m_states.erase(m_states.begin() + _index);
  return true;
}

void CollisionEventGenerator::AddState(
    const ContactSnapshot& _contact, std::vector<CollisionEvent>& _events)
{
  m_states.push_back({_contact.pair, _contact, 0, 0, true});
  _events.push_back(
      {CollisionEventType::kEnter, _contact, m_step_index});
}

void CollisionEventGenerator::UpdateActiveState(
    CollisionPairState& _state, const ContactSnapshot& _contact,
    std::vector<CollisionEvent>& _events)
{
  _state.latest_contact = _contact;
  _state.missing_steps = 0;
  _state.quiet_steps =
      _contact.closing_speed <= kExitSpeed ? _state.quiet_steps + 1 : 0;
  if (_state.quiet_steps >= kQuietStepsToExit)
  {
    const CollisionPairKey pair = _state.pair;
    _events.push_back(
        {CollisionEventType::kExit, _contact, m_step_index});
    RemovePair(pair);
  }
}

CollisionPairState* CollisionEventGenerator::FindState(
    const CollisionPairKey& _pair)
{
  const auto found = std::find_if(
      m_states.begin(), m_states.end(),
      [&_pair](const CollisionPairState& _state) {
        return _state.pair == _pair;
      });
  return found == m_states.end() ? nullptr : &*found;
}

void CollisionEventGenerator::RemovePair(const CollisionPairKey& _pair)
{
  m_states.erase(
      std::remove_if(
          m_states.begin(), m_states.end(),
          [&_pair](const CollisionPairState& _state) {
            return _state.pair == _pair;
          }),
      m_states.end());
}

void CollisionEventGenerator::RemoveEntity(EntityId _id)
{
  m_states.erase(
      std::remove_if(
          m_states.begin(), m_states.end(),
          [_id](const CollisionPairState& _state) {
            return _state.pair.first_id == _id ||
                   _state.pair.second_id == _id;
          }),
      m_states.end());
}

void CollisionEventGenerator::Clear()
{
  m_states.clear();
  m_step_index = 0;
}

std::size_t CollisionEventGenerator::StateCount() const
{
  return m_states.size();
}

CollisionEventDispatcher::ListenerId CollisionEventDispatcher::Subscribe(
    ICollisionListener& _listener)
{
  const auto existing = std::find_if(
      m_listeners.begin(), m_listeners.end(),
      [&_listener](const ListenerEntry& _entry) {
        return _entry.listener == &_listener;
      });
  if (existing != m_listeners.end())
  {
    return existing->id;
  }
  ListenerId id = m_next_listener_id++;
  if (id == 0)
  {
    id = m_next_listener_id++;
  }
  m_listeners.push_back({id, &_listener});
  return id;
}

bool CollisionEventDispatcher::Unsubscribe(ListenerId _listener_id)
{
  const auto listener = std::find_if(
      m_listeners.begin(), m_listeners.end(),
      [_listener_id](const ListenerEntry& _entry) {
        return _entry.id == _listener_id;
      });
  if (listener == m_listeners.end())
  {
    return false;
  }
  m_listeners.erase(listener);
  return true;
}

void CollisionEventDispatcher::Dispatch(
    const std::vector<CollisionEvent>& _events)
{
  std::vector<ListenerId> listener_ids;
  for (const ListenerEntry& listener : m_listeners)
  {
    listener_ids.push_back(listener.id);
  }
  for (const CollisionEvent& event : _events)
  {
    for (ListenerId id : listener_ids)
    {
      ICollisionListener* listener = FindListener(id);
      if (listener != nullptr)
      {
        listener->OnCollision(event);
      }
    }
  }
}

std::size_t CollisionEventDispatcher::ListenerCount() const
{
  return m_listeners.size();
}

ICollisionListener* CollisionEventDispatcher::FindListener(
    ListenerId _listener_id) const
{
  const auto listener = std::find_if(
      m_listeners.begin(), m_listeners.end(),
      [_listener_id](const ListenerEntry& _entry) {
        return _entry.id == _listener_id;
      });
  return listener == m_listeners.end() ? nullptr : listener->listener;
}

bool CollisionEventGenerator::ContainsPair(
    const std::vector<ContactSnapshot>& _contacts,
    const CollisionPairKey& _pair) const
{
  return std::any_of(
      _contacts.begin(), _contacts.end(),
      [&_pair](const ContactSnapshot& _contact) {
        return _contact.pair == _pair;
      });
}

EntityId WorldGroundId()
{
  return {std::numeric_limits<std::size_t>::max(), 0};
}

}  // namespace siege_lab
