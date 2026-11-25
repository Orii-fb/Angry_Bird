#include <memory>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "object_pool.h"

namespace siege_lab
{

namespace
{

struct PoolValue
{
  explicit PoolValue(int _value) : value(_value)
  {
  }

  int value;
};

struct TrackedBase
{
  explicit TrackedBase(int* _destruction_count)
      : destruction_count(_destruction_count)
  {
  }

  virtual ~TrackedBase()
  {
    ++*destruction_count;
  }

  int* destruction_count;
};

struct TrackedDerived : public TrackedBase
{
  using TrackedBase::TrackedBase;
};

static_assert(!std::is_copy_constructible_v<ObjectPool<PoolValue, 2>>);
static_assert(!std::is_move_constructible_v<ObjectPool<PoolValue, 2>>);

}  // namespace

TEST(Pool, P01_CapacityLimit)
{
  ObjectPool<PoolValue, 3> pool;
  const auto first_id = pool.Create(10);
  const auto second_id = pool.Create(20);
  const auto third_id = pool.Create(30);
  const auto overflow_id = pool.Create(40);

  ASSERT_TRUE(first_id.has_value());
  ASSERT_TRUE(second_id.has_value());
  ASSERT_TRUE(third_id.has_value());
  EXPECT_FALSE(overflow_id.has_value());
  EXPECT_EQ(10, pool.Get(*first_id)->value);
  EXPECT_EQ(20, pool.Get(*second_id)->value);
  EXPECT_EQ(30, pool.Get(*third_id)->value);
}

TEST(Pool, P02_RecycledSlotChangesGeneration)
{
  ObjectPool<PoolValue, 1> pool;
  const auto first_id = pool.Create(7);

  ASSERT_TRUE(first_id.has_value());
  ASSERT_TRUE(pool.Destroy(*first_id));
  const auto replacement_id = pool.Create(42);

  ASSERT_TRUE(replacement_id.has_value());
  EXPECT_EQ(first_id->slot, replacement_id->slot);
  EXPECT_NE(first_id->generation, replacement_id->generation);
  EXPECT_NE(*first_id, *replacement_id);
}

TEST(Pool, P03_StaleHandleRejected)
{
  ObjectPool<PoolValue, 1> pool;
  const auto old_id = pool.Create(1);

  ASSERT_TRUE(old_id.has_value());
  ASSERT_TRUE(pool.Destroy(*old_id));
  const auto new_id = pool.Create(2);

  ASSERT_TRUE(new_id.has_value());
  EXPECT_EQ(nullptr, pool.Get(*old_id));
  ASSERT_NE(nullptr, pool.Get(*new_id));
  EXPECT_EQ(2, pool.Get(*new_id)->value);
}

TEST(Pool, P04_LiveAddressStable)
{
  ObjectPool<PoolValue, 3> pool;
  const auto first_id = pool.Create(1);

  ASSERT_TRUE(first_id.has_value());
  PoolValue* first_address = pool.Get(*first_id);
  ASSERT_NE(nullptr, first_address);

  ASSERT_TRUE(pool.Create(2).has_value());
  ASSERT_TRUE(pool.Create(3).has_value());
  EXPECT_EQ(first_address, pool.Get(*first_id));
}

TEST(Pool, P05_ClearInvalidatesHandles)
{
  ObjectPool<PoolValue, 3> pool;
  const auto first_id = pool.Create(1);
  const auto second_id = pool.Create(2);

  ASSERT_TRUE(first_id.has_value());
  ASSERT_TRUE(second_id.has_value());
  ASSERT_TRUE(pool.Destroy(*first_id));

  std::vector<int> values;
  pool.ForEach([&values](EntityId, const PoolValue& _value) {
    values.push_back(_value.value);
  });

  ASSERT_EQ(1U, values.size());
  EXPECT_EQ(2, values.front());

  pool.Clear();
  EXPECT_EQ(nullptr, pool.Get(*second_id));
  EXPECT_TRUE(pool.Create(3).has_value());
  EXPECT_EQ(nullptr, pool.Get(*second_id));
}

TEST(ObjectPoolTest, DestroysOwnedPolymorphicObjects)
{
  int destruction_count = 0;
  ObjectPool<std::unique_ptr<TrackedBase>, 1> pool;
  auto owned_object = std::make_unique<TrackedDerived>(&destruction_count);
  const auto id = pool.Create(std::move(owned_object));

  ASSERT_TRUE(id.has_value());
  ASSERT_TRUE(pool.Destroy(*id));
  EXPECT_EQ(1, destruction_count);
}

}  // namespace siege_lab
