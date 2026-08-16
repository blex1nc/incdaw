#include "app/AudioAssetImport.h"

#include "engine/audio/WavFile.h"

namespace incdaw::app {

bool importAudioAsset(project::Project& project, const std::string& path,
                      AudioAssetImport& imported)
{
    engine::AudioFileData header;

    if (!engine::WavFile::probe(path, header))
        return false;

    for (const project::AudioAsset& existing : project.audioAssets()) {
        if (existing.absolutePath == path) {
            imported.id      = existing.id;
            imported.asset   = existing;
            imported.created = false;
            imported.index   = project.indexOfAudioAsset(existing.id);
            return true;
        }
    }

    project::AudioAsset& added = project.addAudioAsset(path);
    added.sampleRate           = header.sampleRate;
    added.frameCount           = header.frameCount;
    added.channelCount         = header.channelCount;

    imported.id      = added.id;
    imported.asset   = added;
    imported.created = true;
    imported.index   = project.audioAssets().size() - 1;

    return true;
}

void restoreImportedAsset(project::Project& project, const AudioAssetImport& imported)
{
    if (!imported.created || project.indexOfAudioAsset(imported.id) != project::Project::notFound)
        return;

    project.insertAudioAsset(imported.index, imported.asset);
}

} // namespace incdaw::app
