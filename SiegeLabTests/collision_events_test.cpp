#include <gtest/gtest.h>

#include "collision_events.h"

namespace siege_lab
{

namespace
{

EntityId MakeId(std::size_t _slot, std::uint32_t _generation = 1)
{
  return EntityId{_slot, _generation};
}

ContactSnapshot MakeContact(
    EntityId _first, EntityId _second, cyclone::real _closing_speed,
    cyclone::Vector3 _point = cyclone::Vector3(1.0, 2.0, 3.0),
    cyclone::Vector3 _normal = cyclone::Vector3(0.0, 1.0, 0.0))
{
  return {
      {_first, _second}, _point, _normal, _closing_speed};
}

class RecordingListener final : public ICollisionListener
{
 public:
  void OnCollision(const CollisionEvent& _event) override
  {
    m_events.push_back(_event);
  }

  std::vector<CollisionEvent> m_events;
};

class UnsubscribingListener final : public ICollisionListener
{
 public:
  UnsubscribingListener(
      CollisionEventDispatcher& _dispatcher,
      CollisionEventDispatcher::ListenerId& _id)
      : m_dispatcher(_dispatcher), m_id(_id)
  {
  }

  void OnCollision(const CollisionEvent&) override
  {
    m_dispatcher.Unsubscribe(m_id);
    ++m_calls;
  }

  CollisionEventDispatcher& m_dispatcher;
  CollisionEventDispatcher::ListenerId& m_id;
  unsigned m_calls = 0;
};

void AppendUpdate(
    CollisionEventGenerator& _generator,
    const std::vector<ContactSnapshot>& _contacts,
    std::vector<CollisionEvent>& _events)
{
  const auto next = _generator.Update(_contacts, true);
  _events.insert(_events.end(), next.begin(), next.end());
}

void ExpectReimpactSequence(const std::vector<CollisionEvent>& _events)
{
  ASSERT_EQ(4U, _events.size());
  EXPECT_EQ(CollisionEventType::kEnter, _events[0].type);
  EXPECT_EQ(1U, _events[0].step_index);
  EXPECT_EQ(CollisionEventType::kExit, _events[1].type);
  EXPECT_EQ(4U, _events[1].step_index);
  EXPECT_EQ(CollisionEventType::kEnter, _events[2].type);
  EXPECT_EQ(6U, _events[2].step_index);
  EXPECT_EQ(CollisionEventType::kExit, _events[3].type);
  EXPECT_EQ(8U, _events[3].step_index);
}

}  // namespace

TEST(CollisionEvent, EV01_ContactProducesEnterAtExpectedStep)
{
  CollisionEventGenerator generator;
  const EntityId first = MakeId(1);
  const EntityId second = MakeId(2);
  for (int step = 0; step < 11; ++step)
  {
    EXPECT_TRUE(generator.Update({}, true).empty());
  }
  const ContactSnapshot contact{
      MakeContact(first, second, 4.0, {1.0, 1.0, 0.0}, {-1.0, 0.0, 0.0})};

  const auto events = generator.Update({contact}, true);

  ASSERT_EQ(1U, events.size());
  EXPECT_EQ(CollisionEventType::kEnter, events[0].type);
  EXPECT_NEAR(4.0, events[0].contact.closing_speed, 0.001);
  EXPECT_EQ(first, events[0].contact.pair.first_id);
  EXPECT_EQ(second, events[0].contact.pair.second_id);
  EXPECT_NEAR(1.0, events[0].contact.point.x, 0.001);
  EXPECT_NEAR(-1.0, events[0].contact.normal.x, 0.001);
  EXPECT_EQ(12U, events[0].step_index);
}

TEST(CollisionEvent, EV02_NoContactProducesNoEvent)
{
  CollisionEventGenerator generator;
  for (int step = 0; step < 10; ++step)
  {
    EXPECT_TRUE(generator.Update({}, true).empty());
  }

  EXPECT_EQ(0U, generator.StateCount());
}

TEST(CollisionEvent, EV03_FourContactsGiveOnePairAndKeepMaximum)
{
  CollisionEventGenerator generator;
  const EntityId first = MakeId(1);
  const EntityId ground = WorldGroundId();
  const std::vector<ContactSnapshot> contacts{
      MakeContact(first, ground, 1.0, {0.0, 1.0, 0.0}),
      MakeContact(first, ground, 2.0, {1.0, 1.0, 0.0}),
      MakeContact(first, ground, 4.0, {2.0, 1.0, 0.0}),
      MakeContact(first, ground, 3.0, {3.0, 1.0, 0.0})};
  const auto events = generator.Update(contacts, true);

  ASSERT_EQ(1U, events.size());
  EXPECT_EQ(CollisionEventType::kEnter, events[0].type);
  EXPECT_NEAR(4.0, events[0].contact.closing_speed, 0.001);
  EXPECT_NEAR(2.0, events[0].contact.point.x, 0.001);
}

TEST(CollisionEvent, EV04_EnterThenExitOnly)
{
  CollisionEventGenerator generator;
  const auto impact = MakeContact(MakeId(1), MakeId(2), 4.0);
  const auto quiet = MakeContact(MakeId(1), MakeId(2), 0.05);
  const auto enter = generator.Update({impact}, true);
  EXPECT_TRUE(generator.Update({quiet}, true).empty());
  EXPECT_TRUE(generator.Update({quiet}, true).empty());
  const auto exit = generator.Update({quiet}, true);
  EXPECT_TRUE(generator.Update({}, true).empty());
  EXPECT_TRUE(generator.Update({}, true).empty());

  ASSERT_EQ(1U, enter.size());
  ASSERT_EQ(1U, exit.size());
  EXPECT_EQ(CollisionEventType::kEnter, enter[0].type);
  EXPECT_EQ(1U, enter[0].step_index);
  EXPECT_EQ(CollisionEventType::kExit, exit[0].type);
  EXPECT_EQ(4U, exit[0].step_index);
  EXPECT_EQ(0U, generator.StateCount());
}

TEST(CollisionEvent, EV05_PairOrderingIsStable)
{
  CollisionEventGenerator generator;
  const EntityId first = MakeId(1);
  const EntityId second = MakeId(2);
  const auto enter = generator.Update(
      {MakeContact(first, second, 4.0, {}, {-1.0, 0.0, 0.0})}, true);
  const auto reversed = generator.Update(
      {MakeContact(second, first, 4.0, {}, {1.0, 0.0, 0.0})}, true);

  ASSERT_EQ(1U, enter.size());
  EXPECT_TRUE(reversed.empty());
  EXPECT_EQ(first, enter[0].contact.pair.first_id);
  EXPECT_EQ(second, enter[0].contact.pair.second_id);
  EXPECT_NEAR(-1.0, enter[0].contact.normal.x, 0.001);
}

TEST(CollisionEvent, EV07_ResetInvalidatesPreviousPairs)
{
  CollisionEventGenerator generator;
  const auto old_pair = MakeContact(MakeId(1, 1), MakeId(2, 1), 4.0);
  ASSERT_EQ(1U, generator.Update({old_pair}, true).size());
  generator.Clear();

  const auto new_pair = MakeContact(MakeId(1, 2), MakeId(2, 2), 4.0);
  const auto events = generator.Update({new_pair}, true);

  ASSERT_EQ(1U, events.size());
  EXPECT_EQ(CollisionEventType::kEnter, events[0].type);
  EXPECT_EQ(1U, events[0].step_index);
  EXPECT_EQ(0U, generator.Update({}, true).size());
}

TEST(CollisionEvent, EV13_RestingStackIsSilent)
{
  CollisionEventGenerator generator;
  const EntityId ground = WorldGroundId();
  const std::vector<ContactSnapshot> contacts{
      MakeContact(MakeId(1), ground, 0.05),
      MakeContact(MakeId(2), ground, 0.05),
      MakeContact(MakeId(1), MakeId(2), 0.05),
      MakeContact(MakeId(2), MakeId(3), 0.05)};
  std::size_t event_count = 0;
  for (int step = 0; step < 600; ++step)
  {
    event_count += generator.Update(contacts, true).size();
  }

  EXPECT_EQ(0U, event_count);
}

TEST(CollisionEvent, EV16_OneMissingStepDoesNotExit)
{
  CollisionEventGenerator generator;
  const auto impact = generator.Update(
      {MakeContact(MakeId(1), MakeId(2), 4.0)}, true);
  EXPECT_TRUE(generator.Update({}, true).empty());
  EXPECT_TRUE(
      generator.Update({MakeContact(MakeId(1), MakeId(2), 0.05)}, true)
          .empty());
  EXPECT_TRUE(generator.Update({}, true).empty());
  const auto exit = generator.Update({}, true);

  ASSERT_EQ(1U, impact.size());
  ASSERT_EQ(1U, exit.size());
  EXPECT_EQ(CollisionEventType::kExit, exit[0].type);
  EXPECT_EQ(5U, exit[0].step_index);
}

TEST(CollisionEvent, EV19_IncompleteStepDoesNotCauseFalseExit)
{
  CollisionEventGenerator generator;
  const auto impact = generator.Update(
      {MakeContact(MakeId(1), MakeId(2), 4.0)}, true);
  EXPECT_TRUE(generator.Update({}, false).empty());
  EXPECT_TRUE(generator.Update({}, false).empty());
  const auto quiet = generator.Update(
      {MakeContact(MakeId(1), MakeId(2), 0.05)}, true);

  ASSERT_EQ(1U, impact.size());
  EXPECT_TRUE(quiet.empty());
  EXPECT_EQ(1U, generator.StateCount());
}

TEST(CollisionEventGeneratorTest, UnregisteringEitherGenerationClearsPairState)
{
  CollisionEventGenerator generator;
  const EntityId first = MakeId(1);
  ASSERT_EQ(
      1U, generator.Update(
                  {MakeContact(first, MakeId(2), 1.0)}, true).size());

  generator.RemoveEntity(MakeId(1, 2));
  EXPECT_EQ(1U, generator.StateCount());
  generator.RemoveEntity(first);

  EXPECT_EQ(0U, generator.StateCount());
}

TEST(CollisionEvent, EV15_ReimpactWhileStillTouching)
{
  CollisionEventGenerator generator;
  const ContactSnapshot impact = MakeContact(MakeId(1), MakeId(2), 1.0);
  const ContactSnapshot quiet = MakeContact(MakeId(1), MakeId(2), 0.05);
  std::vector<CollisionEvent> events = generator.Update({impact}, true);
  for (int step = 0; step < 4; ++step)
  {
    AppendUpdate(generator, {quiet}, events);
  }
  AppendUpdate(
      generator, {MakeContact(MakeId(1), MakeId(2), 5.0)}, events);
  for (int step = 0; step < 2; ++step)
  {
    AppendUpdate(generator, {}, events);
  }

  ExpectReimpactSequence(events);
}

TEST(CollisionEvent, EV17_IndependentPairs)
{
  CollisionEventGenerator generator;
  const EntityId a = MakeId(1);
  const EntityId b = MakeId(2);
  const EntityId c = MakeId(3);
  const auto events = generator.Update(
      {MakeContact(a, b, 4.0),
       MakeContact(b, WorldGroundId(), 0.1),
       MakeContact(a, c, 2.0)},
      true);

  ASSERT_EQ(2U, events.size());
  EXPECT_EQ(CollisionEventType::kEnter, events[0].type);
  EXPECT_EQ(CollisionEventType::kEnter, events[1].type);
  EXPECT_EQ(2U, generator.StateCount());
}

TEST(CollisionEvent, EV14_SubthresholdContactNeverExits)
{
  CollisionEventGenerator generator;
  const auto quiet = MakeContact(MakeId(1), MakeId(2), 0.99);
  EXPECT_TRUE(generator.Update({quiet}, true).empty());
  EXPECT_TRUE(generator.Update({quiet}, true).empty());
  EXPECT_TRUE(generator.Update({quiet}, true).empty());
  EXPECT_TRUE(generator.Update({}, true).empty());
  EXPECT_TRUE(generator.Update({}, true).empty());
}

TEST(CollisionEventDispatcherTest, UnsubscribeDuringDispatchIsSafe)
{
  CollisionEventDispatcher dispatcher;
  CollisionEventDispatcher::ListenerId listener_id = 0;
  UnsubscribingListener first(dispatcher, listener_id);
  RecordingListener second;
  listener_id = dispatcher.Subscribe(first);
  ASSERT_NE(0U, listener_id);
  ASSERT_NE(0U, dispatcher.Subscribe(second));
  const std::vector<CollisionEvent> events{
      {CollisionEventType::kEnter,
       MakeContact(MakeId(1), MakeId(2), 1.0)}};

  dispatcher.Dispatch(events);
  dispatcher.Dispatch(events);

  EXPECT_EQ(1U, first.m_calls);
  EXPECT_EQ(2U, second.m_events.size());
}

}  // namespace siege_lab
