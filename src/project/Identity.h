#pragma once

#include <cstdint>
#include <functional>

namespace incdaw::project {

/// Stable identity for every persistent entity in a project.
///
/// Entities reference each other by id, never by pointer or index
/// (docs/ARCHITECTURE.md §5). This is what makes undo, serialization, and
/// relinking tractable: a command can record "the clip with id 42" and still
/// mean the same clip after a load, a reorder, or an undo.
///
/// Ids are unique within a project and are NEVER reused, including after
/// deletion — a recycled id would make a stale reference silently resolve to
/// the wrong entity instead of failing loudly.
class EntityId {
public:
    using Value = std::uint64_t;

    static constexpr Value invalidValue = 0;

    constexpr EntityId() noexcept = default;
    explicit constexpr EntityId(Value value) noexcept : value_(value) {}

    [[nodiscard]] constexpr Value value() const noexcept { return value_; }
    [[nodiscard]] constexpr bool  isValid() const noexcept { return value_ != invalidValue; }

    [[nodiscard]] friend constexpr bool operator==(EntityId, EntityId) noexcept = default;
    [[nodiscard]] friend constexpr auto operator<=>(EntityId, EntityId) noexcept = default;

private:
    Value value_ = invalidValue;
};

/// Allocates ids for one project.
///
/// Not thread-safe and deliberately not atomic: ids are only ever minted on the
/// thread that owns the project model, and making this atomic would invite
/// callers to mint ids from the audio thread, which must never happen.
class IdGenerator {
public:
    IdGenerator() = default;

    /// Restores the generator after loading a project, so that ids minted in
    /// this session cannot collide with ids already stored in the file.
    explicit IdGenerator(EntityId::Value highestUsedId) noexcept : next_(highestUsedId + 1) {}

    [[nodiscard]] EntityId next() noexcept { return EntityId{next_++}; }

    /// Called while loading, for every id read from the project file.
    void observe(EntityId id) noexcept
    {
        if (id.value() >= next_)
            next_ = id.value() + 1;
    }

    [[nodiscard]] EntityId::Value peekNext() const noexcept { return next_; }

private:
    EntityId::Value next_ = 1;   // 0 is reserved for "invalid"
};

} // namespace incdaw::project

template <>
struct std::hash<incdaw::project::EntityId> {
    [[nodiscard]] std::size_t operator()(incdaw::project::EntityId id) const noexcept
    {
        return std::hash<incdaw::project::EntityId::Value>{}(id.value());
    }
};
