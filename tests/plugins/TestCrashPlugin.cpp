// The misbehaviour matrix's first row: a plugin that crashes on load.
//
// docs/PLUGIN_HOST.md §2 — third-party plugins are hostile input. This one
// segfaults inside its entry init, which is exactly what the out-of-process
// scanner exists to survive: the scanner dies, the host records it, INCDAW
// never feels it.

#include <clap/clap.h>

namespace {

bool entryInit(const char*)
{
    volatile int* hostile = nullptr;
    *hostile = 1;   // the crash the isolation strategy is for
    return true;
}

void entryDeinit() {}
const void* entryGetFactory(const char*) { return nullptr; }

} // namespace

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION_INIT,
    entryInit,
    entryDeinit,
    entryGetFactory,
};
