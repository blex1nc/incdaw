#include "app/commands/PluginCommands.h"

#include <algorithm>

namespace incdaw::app {
namespace {

project::MixerNode* findNode(Project& project, EntityId id)
{
    for (project::MixerNode& node : project.mixerNodes())
        if (node.id == id)
            return &node;

    return nullptr;
}

} // namespace

bool AddInsertCommand::execute(Project& project)
{
    project::MixerNode* node = findNode(project, mixerNode_);
    if (node == nullptr || !plugin_.isValid())
        return false;

    if (!minted_) {
        slot_.id     = project.ids().next();
        slot_.plugin = plugin_;
        minted_      = true;
    }

    const std::size_t at = std::min(index_ == append ? node->inserts.size() : index_,
                                    node->inserts.size());

    node->inserts.insert(node->inserts.begin() + static_cast<std::ptrdiff_t>(at), slot_);
    return true;
}

void AddInsertCommand::undo(Project& project)
{
    project::MixerNode* node = findNode(project, mixerNode_);
    if (node == nullptr)
        return;

    node->inserts.erase(std::remove_if(node->inserts.begin(), node->inserts.end(),
                                       [this](const PluginSlot& slot) {
                                           return slot.id == slot_.id;
                                       }),
                        node->inserts.end());
}

bool RemoveInsertCommand::execute(Project& project)
{
    project::MixerNode* node = findNode(project, mixerNode_);
    if (node == nullptr)
        return false;

    for (std::size_t index = 0; index < node->inserts.size(); ++index) {
        if (node->inserts[index].id != slotId_)
            continue;

        removed_ = node->inserts[index];
        index_   = index;
        node->inserts.erase(node->inserts.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    return false;
}

void RemoveInsertCommand::undo(Project& project)
{
    project::MixerNode* node = findNode(project, mixerNode_);
    if (node == nullptr)
        return;

    // Back where it was: chain position is audible order, not decoration.
    const std::size_t at = std::min(index_, node->inserts.size());
    node->inserts.insert(node->inserts.begin() + static_cast<std::ptrdiff_t>(at), removed_);
}

bool SetInsertBypassedCommand::execute(Project& project)
{
    project::MixerNode* node = findNode(project, mixerNode_);
    if (node == nullptr)
        return false;

    for (PluginSlot& slot : node->inserts) {
        if (slot.id != slotId_)
            continue;

        if (slot.bypassed == bypassed_)
            return false;   // a no-op must not occupy an undo step

        previous_     = slot.bypassed;
        slot.bypassed = bypassed_;
        return true;
    }

    return false;
}

void SetInsertBypassedCommand::undo(Project& project)
{
    project::MixerNode* node = findNode(project, mixerNode_);
    if (node == nullptr)
        return;

    for (PluginSlot& slot : node->inserts)
        if (slot.id == slotId_)
            slot.bypassed = previous_;
}

bool MoveInsertCommand::execute(Project& project)
{
    project::MixerNode* node = findNode(project, mixerNode_);
    if (node == nullptr || (direction_ != -1 && direction_ != 1))
        return false;

    auto& inserts = node->inserts;

    for (std::size_t index = 0; index < inserts.size(); ++index) {
        if (inserts[index].id != slotId_)
            continue;

        const std::ptrdiff_t target =
            static_cast<std::ptrdiff_t>(index) + direction_;
        if (target < 0 || target >= static_cast<std::ptrdiff_t>(inserts.size()))
            return false;   // already at its end of the chain

        movedFrom_ = index;
        std::swap(inserts[index], inserts[static_cast<std::size_t>(target)]);
        return true;
    }

    return false;
}

void MoveInsertCommand::undo(Project& project)
{
    project::MixerNode* node = findNode(project, mixerNode_);
    if (node == nullptr)
        return;

    auto& inserts = node->inserts;

    const std::size_t target = static_cast<std::size_t>(
        static_cast<std::ptrdiff_t>(movedFrom_) + direction_);
    if (movedFrom_ < inserts.size() && target < inserts.size())
        std::swap(inserts[movedFrom_], inserts[target]);
}

} // namespace incdaw::app
