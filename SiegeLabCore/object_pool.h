#ifndef SIEGE_LAB_CORE_OBJECT_POOL_H_
#define SIEGE_LAB_CORE_OBJECT_POOL_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <utility>

namespace siege_lab
{

struct EntityId
{
  std::size_t slot = std::numeric_limits<std::size_t>::max();
  std::uint32_t generation = 0;

  bool operator==(const EntityId&) const = default;
};

template <typename ValueType, std::size_t Capacity>
class ObjectPool
{
  static_assert(Capacity > 0, "ObjectPool capacity must be positive.");

 private:
  struct Slot
  {
    std::optional<ValueType> value;
    std::uint32_t generation = 1;
    bool retired = false;
  };

 public:
  ObjectPool() = default;
  ObjectPool(const ObjectPool&) = delete;
  ObjectPool& operator=(const ObjectPool&) = delete;
  ObjectPool(ObjectPool&&) = delete;
  ObjectPool& operator=(ObjectPool&&) = delete;

  template <typename... ArgumentTypes>
  std::optional<EntityId> Create(ArgumentTypes&&... _arguments)
  {
    for (std::size_t index = 0; index < Capacity; ++index)
    {
      Slot& slot = m_slots[index];
      if (!slot.retired && !slot.value.has_value())
      {
        slot.value.emplace(std::forward<ArgumentTypes>(_arguments)...);
        return EntityId{index, slot.generation};
      }
    }

    return std::nullopt;
  }

  ValueType* Get(EntityId _id)
  {
    Slot* slot = FindSlot(_id);
    return slot == nullptr ? nullptr : &*slot->value;
  }

  const ValueType* Get(EntityId _id) const
  {
    const Slot* slot = FindSlot(_id);
    return slot == nullptr ? nullptr : &*slot->value;
  }

  bool Destroy(EntityId _id)
  {
    Slot* slot = FindSlot(_id);
    if (slot == nullptr)
    {
      return false;
    }

    slot->value.reset();
    AdvanceGeneration(*slot);
    return true;
  }

  template <typename FunctionType>
  void ForEach(FunctionType&& _function)
  {
    for (std::size_t index = 0; index < Capacity; ++index)
    {
      Slot& slot = m_slots[index];
      if (slot.value.has_value())
      {
        std::invoke(_function, EntityId{index, slot.generation}, *slot.value);
      }
    }
  }

  template <typename FunctionType>
  void ForEach(FunctionType&& _function) const
  {
    for (std::size_t index = 0; index < Capacity; ++index)
    {
      const Slot& slot = m_slots[index];
      if (slot.value.has_value())
      {
        std::invoke(_function, EntityId{index, slot.generation}, *slot.value);
      }
    }
  }

  void Clear()
  {
    for (Slot& slot : m_slots)
    {
      if (slot.value.has_value())
      {
        slot.value.reset();
        AdvanceGeneration(slot);
      }
    }
  }

 private:
  Slot* FindSlot(EntityId _id)
  {
    if (_id.generation == 0 || _id.slot >= Capacity)
    {
      return nullptr;
    }

    Slot& slot = m_slots[_id.slot];
    if (slot.generation != _id.generation || !slot.value.has_value())
    {
      return nullptr;
    }

    return &slot;
  }

  const Slot* FindSlot(EntityId _id) const
  {
    if (_id.generation == 0 || _id.slot >= Capacity)
    {
      return nullptr;
    }

    const Slot& slot = m_slots[_id.slot];
    if (slot.generation != _id.generation || !slot.value.has_value())
    {
      return nullptr;
    }

    return &slot;
  }

  static void AdvanceGeneration(Slot& _slot)
  {
    if (_slot.generation == std::numeric_limits<std::uint32_t>::max())
    {
      _slot.retired = true;
      return;
    }

    ++_slot.generation;
  }

  std::array<Slot, Capacity> m_slots;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_OBJECT_POOL_H_
