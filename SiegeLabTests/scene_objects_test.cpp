#include <type_traits>

#include <gtest/gtest.h>

#include "game_entity.h"
#include "scene_objects.h"

namespace siege_lab
{

namespace
{

class TestEntity : public GameEntity
{
 public:
  explicit TestEntity(int _value, int* _destruction_count = nullptr)
      : value(_value), destruction_count(_destruction_count)
  {
  }

  ~TestEntity() override
  {
    if (destruction_count != nullptr)
    {
      ++*destruction_count;
    }
  }

  void Draw() const override
  {
  }

  void RegisterPhysics() override
  {
  }

  int value;
  int* destruction_count;
};

class AbstractTestEntity : public GameEntity
{
};

struct UnrelatedType
{
};

template <typename EntityType>
concept CanCreateEntity = requires(SceneObjects<2>& _scene_objects) {
  _scene_objects.template CreateEntity<EntityType>(42);
};

static_assert(CanCreateEntity<TestEntity>);
static_assert(!CanCreateEntity<AbstractTestEntity>);
static_assert(!CanCreateEntity<UnrelatedType>);
static_assert(std::has_virtual_destructor_v<GameEntity>);

}  // namespace

TEST(SceneObjectsTest, CreatesAndGetsConcreteEntities)
{
  SceneObjects<2> scene_objects;
  const auto id = scene_objects.CreateEntity<TestEntity>(42);

  ASSERT_TRUE(id.has_value());
  ASSERT_NE(nullptr, scene_objects.Get(*id));
  EXPECT_EQ(42, static_cast<TestEntity*>(scene_objects.Get(*id))->value);
}

TEST(SceneObjectsTest, DestroysEntitiesAndRejectsStaleIds)
{
  SceneObjects<1> scene_objects;
  int destruction_count = 0;
  const auto id = scene_objects.CreateEntity<TestEntity>(
      7, &destruction_count);

  ASSERT_TRUE(id.has_value());
  ASSERT_TRUE(scene_objects.Destroy(*id));
  EXPECT_EQ(nullptr, scene_objects.Get(*id));
  EXPECT_FALSE(scene_objects.Destroy(*id));
  EXPECT_EQ(1, destruction_count);
}

}  // namespace siege_lab
