#include "doctest.h"

#include "project/Identity.h"

#include <unordered_set>

using namespace incdaw::project;

TEST_CASE("the default id is invalid and distinct from every real id")
{
    CHECK_FALSE(EntityId{}.isValid());
    CHECK(EntityId{}.value() == EntityId::invalidValue);
    CHECK(EntityId{1}.isValid());
}

TEST_CASE("a fresh generator never mints the invalid id")
{
    IdGenerator generator;
    for (int i = 0; i < 1000; ++i)
        CHECK(generator.next().isValid());
}

TEST_CASE("ids are never reused")
{
    IdGenerator                 generator;
    std::unordered_set<EntityId> seen;

    for (int i = 0; i < 10000; ++i)
        CHECK(seen.insert(generator.next()).second);
}

TEST_CASE("a generator restored from a project continues past the stored ids")
{
    // Loading a project must not mint ids that collide with ids already in the
    // file — a collision would silently alias two different entities.
    IdGenerator generator{500};
    CHECK(generator.next() == EntityId{501});
}

TEST_CASE("observing ids during load advances the generator")
{
    IdGenerator generator;

    generator.observe(EntityId{10});
    generator.observe(EntityId{7});     // out of order: must not move it backwards
    generator.observe(EntityId{42});

    const auto minted = generator.next();
    CHECK(minted == EntityId{43});
    CHECK(minted.value() > 42);
}

TEST_CASE("observing the invalid id leaves the generator alone")
{
    IdGenerator generator;
    generator.observe(EntityId{});
    CHECK(generator.next() == EntityId{1});
}

TEST_CASE("ids are ordered and hashable so they can key the model")
{
    CHECK(EntityId{1} < EntityId{2});
    CHECK(EntityId{2} > EntityId{1});
    CHECK(std::hash<EntityId>{}(EntityId{7}) == std::hash<EntityId>{}(EntityId{7}));
}
