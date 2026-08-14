// incdaw-pluginscan — the out-of-process plugin scanner.
//
// Loads ONE plugin library and reports its descriptors on stdout, one line
// each: PLUGIN\tid\tname\tvendor\tversion. This process is disposable by
// design (docs/PLUGIN_HOST.md §3): a plugin that crashes on load kills the
// scanner, the host records the corpse on the blacklist, and INCDAW itself
// never touched the binary.

#include "plugins/clap/ClapLibrary.h"

#include <cstdio>

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: incdaw-pluginscan <plugin path>\n");
        return 2;
    }

    incdaw::plugins::ClapLibrary library;
    std::string error;

    if (!library.open(argv[1], error)) {
        std::fprintf(stderr, "scan failed: %s\n", error.c_str());
        return 2;
    }

    for (const auto& descriptor : library.descriptors())
        std::printf("PLUGIN\t%s\t%s\t%s\t%s\n", descriptor.id.c_str(), descriptor.name.c_str(),
                    descriptor.vendor.c_str(), descriptor.version.c_str());

    return 0;
}
