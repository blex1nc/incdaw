#include "project/Identity.h"

// EntityId and IdGenerator are header-only by design: they sit in the inner
// loop of model traversal and must inline. This translation unit exists to
// give the layer a compiled artifact and to statically verify the invariants
// that the rest of the project relies on.

namespace incdaw::project {

static_assert(EntityId{}.value() == EntityId::invalidValue,
              "A default-constructed EntityId must be invalid.");
static_assert(!EntityId{}.isValid(),
              "The zero id must never be treated as a real entity.");
static_assert(EntityId{7}.value() == 7);
static_assert(EntityId{7} == EntityId{7});
static_assert(EntityId{7} != EntityId{8});

} // namespace incdaw::project
