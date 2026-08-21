#include "app/StandardActions.h"

#include "app/CommandRegistry.h"
#include "app/commands/ChannelCommands.h"
#include "app/commands/PatternCommands.h"
#include "app/commands/TrackCommands.h"
#include "project/Model.h"

#include <memory>
#include <string>

namespace incdaw::app {

void registerStandardActions(CommandRegistry& registry)
{
    CommandRegistry* target = &registry;

    // The names are generated when the action runs, not when it is registered:
    // "Channel 4" must be right at the moment it is added, and the count has
    // changed several times by then.
    registry.registerAction({"channel.add", "Add Channel", "Channel Rack", "", [target] {
        return std::make_unique<AddChannelCommand>(
            "Channel " + std::to_string(target->project().channels().size() + 1));
    }});

    registry.registerAction({"pattern.add", "Add Pattern", "Patterns", "", [target] {
        return std::make_unique<AddPatternCommand>(
            "Pattern " + std::to_string(target->project().patterns().size() + 1));
    }});

    registry.registerAction({"track.add", "Add Track", "Playlist", "", [target] {
        return std::make_unique<AddTrackCommand>(
            "Track " + std::to_string(target->project().tracks().size() + 1));
    }});
}

} // namespace incdaw::app
