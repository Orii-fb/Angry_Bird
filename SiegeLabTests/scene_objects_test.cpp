#include <type_traits>

#include <gtest/gtest.h>

#include <cyclone/collide_fine.h>

#include "block.h"
#include "game_entity.h"
#include "physics_world.h"
#include "projectile.h"
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
    ++draw_count;
  }

  void RegisterPhysics() override
  {
  }

  int value;
  int* destruction_count;
  mutable unsigned draw_count = 0;
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

TEST(Entities, GO01_FactoryCreatesDerivedTypes)
{
  SceneObjects<4> scene_objects;
  const auto standard_id =
      scene_objects.CreateEntity<StandardProjectile>();
  const auto heavy_id = scene_objects.CreateEntity<HeavyProjectile>();

  ASSERT_TRUE(standard_id.has_value());
  ASSERT_TRUE(heavy_id.has_value());
  ASSERT_NE(*standard_id, *heavy_id);
  auto* standard =
      static_cast<StandardProjectile*>(scene_objects.Get(*standard_id));
  auto* heavy =
      static_cast<HeavyProjectile*>(scene_objects.Get(*heavy_id));
  ASSERT_NE(nullptr, standard);
  ASSERT_NE(nullptr, heavy);
  EXPECT_EQ(&standard->Body(), standard->Collider().body);
  EXPECT_EQ(&heavy->Body(), heavy->Collider().body);
  EXPECT_NE(&standard->Body(), &heavy->Body());
}

TEST(Entities, GO02_VirtualDrawDispatchWithoutRTTI)
{
  TestEntity entity(42);
  GameEntity& base = entity;

  base.Draw();
  base.Draw();

  EXPECT_EQ(2U, entity.draw_count);
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

TEST(Entities, GO05_StableColliderAddressAndSafeRemoval)
{
  SceneObjects<4> scene_objects;
  PhysicsWorld world;
  const auto first_id = scene_objects.CreateEntity<PillarBlock>();
  ASSERT_TRUE(first_id.has_value());
  auto* first = static_cast<PillarBlock*>(scene_objects.Get(*first_id));
  ASSERT_NE(nullptr, first);
  cyclone::RigidBody* body_address = &first->Body();
  cyclone::CollisionBox* collider_address = &first->Collider();
  ASSERT_TRUE(world.RegisterBox(
      *first_id, first->Body(), first->Collider()));

  ASSERT_TRUE(scene_objects.CreateEntity<PillarBlock>().has_value());
  ASSERT_TRUE(scene_objects.CreateEntity<CubeBlock>().has_value());
  ASSERT_TRUE(scene_objects.CreateEntity<CubeBlock>().has_value());
  EXPECT_EQ(body_address, &first->Body());
  EXPECT_EQ(collider_address, &first->Collider());

  EXPECT_TRUE(world.UnregisterObject(*first_id));
  EXPECT_TRUE(scene_objects.Destroy(*first_id));
  EXPECT_EQ(nullptr, scene_objects.Get(*first_id));
  EXPECT_EQ(0U, world.RegistrationCount());
}

TEST(Entities, GO06_ConstrainedFactoryRejectsUnrelatedTypes)
{
  SceneObjects<2> scene_objects;
  const auto projectile_id =
      scene_objects.CreateEntity<StandardProjectile>();
  std::size_t entity_count = 0;

  ASSERT_TRUE(projectile_id.has_value());
  EXPECT_NE(nullptr, scene_objects.Get(*projectile_id));
  scene_objects.ForEach(
      [&entity_count](EntityId, const GameEntity&) { ++entity_count; });
  EXPECT_EQ(1U, entity_count);
}

TEST(SceneObjectsTest, QueuesDuplicateRemovalUntilExplicitFlush)
{
  SceneObjects<1> scene_objects;
  int destruction_count = 0;
  const auto id = scene_objects.CreateEntity<TestEntity>(
      7, &destruction_count);
  ASSERT_TRUE(id.has_value());

  EXPECT_TRUE(scene_objects.QueueDestroy(*id));
  EXPECT_TRUE(scene_objects.QueueDestroy(*id));
  EXPECT_EQ(0, destruction_count);
  scene_objects.FlushRemovals(
      [&scene_objects](EntityId _id) {
        EXPECT_NE(nullptr, scene_objects.Get(_id));
      });

  EXPECT_EQ(1, destruction_count);
  EXPECT_EQ(nullptr, scene_objects.Get(*id));
}

}  // namespace siege_lab
