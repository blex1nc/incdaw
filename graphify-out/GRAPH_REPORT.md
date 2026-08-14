# Graph Report - phade-8b-devam-a14fcf  (2026-08-14)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 2050 nodes · 3681 edges · 124 communities (115 shown, 9 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 218 edges (avg confidence: 0.83)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `e888a585`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- PianoRollModel
- Command
- -applicationDidFinishLaunching
- SimpleSynth
- DuplicatePatternCommand
- AudioEngine
- NoteSequence
- ChannelCommands.cpp
- PlaylistModel
- Transport
- CommandRegistry
- ChannelRackModel
- CoreAudioDevice
- TempoMap
- MetronomeNode
- compileProjectGraph
- InstrumentNode
- MusicalPosition
- Clip
- CoreAudioDevice.cpp
- string
- MidiInput
- AudioDeviceInfo
- load
- PatternCommands.cpp
- CallbackProfiler
- MidiMessage
- Json
- MixerNode
- SetVelocityCommand
- PlaylistView.mm
- AudioBufferPool
- AudioDevice
- RealtimeGuard.cpp
- AudioBufferView
- ProcessContext
- CompiledGraph
- BasicMidiBuffer
- Model.cpp
- compileArrangement
- GraphBuilder
- SystemInfo
- CountingCommand
- ConstantNode
- TimingProbeInstrument
- vector
- NoteCommands.cpp
- LockFreeQueue
- PluginIdentifier
- DuplicateClipsCommand
- Project
- GraphCompileOptions
- GainNode
- SineOscillatorNode
- TimestampedMidiMessage
- EntityId
- AddPatternClipCommand
- Json.cpp
- Parser
- Fixture
- TrackCommands.cpp
- Node
- PatternChannelContent
- ChannelRackView.mm
- MidiEvent
- ClipCommands.cpp
- MoveClipsCommand
- ToggleStepCommand
- RenameTrackCommand
- MidiRecorder
- CoreMidiDevice.cpp
- CoreMidiDevice
- ioProcTrampoline
- EntityId
- Instrument
- Channel
- Track
- Denormals.h
- AudioAsset
- RemoveChannelCommand
- string
- content
- TimeSignatureEvent
- BlockSegment
- MidiDevice
- humanizeNoteStarts
- PatternListView.mm
- AddNoteCommand
- RecordedEvent
- Pattern
- make-dmg.sh
- RemoveClipsCommand
- MidiDeviceInfo
- RoutingConnection
- AutomationPoint
- friend
- SetClipMutedCommand
- AddTrackCommand
- RemoveTrackCommand
- SetTrackMutedCommand
- Version
- ProjectMetadata
- CompiledProjectGraph
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- OrderRecordingNode
- MidiRecorder.cpp
- ScratchDirectory
- check
- process
- .operator==
- FrameCount
- FramePosition
- friend
- MidiEventType
- uint32_t
- uint64_t
- MidiTests.cpp

## God Nodes (most connected - your core abstractions)
1. `Project` - 132 edges
2. `EntityId` - 55 edges
3. `Json` - 48 edges
4. `Command` - 47 edges
5. `AudioEngine` - 47 edges
6. `CoreAudioDevice` - 42 edges
7. `TempoMap` - 42 edges
8. `SimpleSynth` - 40 edges
9. `Transport` - 39 edges
10. `Clip` - 37 edges

## Surprising Connections (you probably didn't know these)
- `main()` --calls--> `close`  [INFERRED]
  tools/audiocheck/main.cpp → src/platform/MidiDevice.h
- `Audio Engine` --semantically_similar_to--> `Audio Engine Priority`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Controller Linking` --semantically_similar_to--> `Parameter System`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Clip / Project Data Model` --semantically_similar_to--> `Core Data Model`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Development Phases (0-20)` --semantically_similar_to--> `Feature Roadmap (Phase 0-20)`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Approval-Gated Development Protocol** — claude_absolute_user_control_rule, claude_graphify_mandate, claude_feature_workflow, claude_scope_control, claude_dependency_policy, handoff_critical_operating_rule, handoff_feature_protocol, handoff_handoff_rule [EXTRACTED 1.00]
- **Plugin Host Pipeline** — handoff_plugin_scanner, handoff_plugin_registry, handoff_plugin_instance, handoff_parameter_system, handoff_plugin_state_system, handoff_plugin_ui_bridge, handoff_crash_isolation_strategy [EXTRACTED 1.00]
- **Master Signal Chain Convergence** — handoff_midi_signal_flow, handoff_audio_signal_flow, handoff_plugin_automation_flow, handoff_shared_transport_state, claude_mixer, claude_automation, claude_offline_render_engine, claude_core_transport [INFERRED 0.85]

## Communities (124 total, 9 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "PianoRollModel"
Cohesion: 0.07
Nodes (39): NoteList, size_t, Tick, vector, size_t, Tick, vector, PianoRollModel (+31 more)

### Community 2 - "Command"
Cohesion: 0.06
Nodes (31): Command, execute, id, name, undo, string, Tick, MoveNotesCommand (+23 more)

### Community 3 - "-applicationDidFinishLaunching"
Cohesion: 0.06
Nodes (38): INCDAWChannelRackView, INCDAWPatternListView, INCDAWPianoRollView, NSApplicationDelegate, NSObject, NSScrollView, NSSegmentedControl, NSSplitView (+30 more)

### Community 4 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 5 - "DuplicatePatternCommand"
Cohesion: 0.06
Nodes (25): AddPatternCommand, execute, index_, minted_, pattern_, undo, DuplicatePatternCommand, execute (+17 more)

### Community 6 - "AudioEngine"
Cohesion: 0.06
Nodes (54): RetiredGraph, AudioEngine, active_, audioDeviceAboutToStart, audioDeviceStopped, availableDevices, blockCounter_, blockMidi_ (+46 more)

### Community 7 - "NoteSequence"
Cohesion: 0.08
Nodes (30): FrameCount, FramePosition, MidiBuffer, Tick, vector, size_t, Tick, uint32_t (+22 more)

### Community 8 - "ChannelCommands.cpp"
Cohesion: 0.07
Nodes (24): SetChannelMutedCommand, channelId_, execute, muted_, undo, SetChannelSoloedCommand, channelId_, execute (+16 more)

### Community 9 - "PlaylistModel"
Cohesion: 0.13
Nodes (29): EntityId, size_t, Tick, vector, EntityId, size_t, Tick, PlaylistModel (+21 more)

### Community 10 - "Transport"
Cohesion: 0.09
Nodes (24): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, Transport (+16 more)

### Community 11 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 12 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, Rect, ChannelRackModel, contentHeight, hitTest, layout_, muteRect (+17 more)

### Community 13 - "CoreAudioDevice"
Cohesion: 0.08
Nodes (22): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, callback_, close, deviceID_, inputLatency_, name_ (+14 more)

### Community 14 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 15 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 16 - "compileProjectGraph"
Cohesion: 0.17
Nodes (15): NodeIndex, SampleRate, size_t, unique_ptr, addNode, compile, connect, instrumentFor (+7 more)

### Community 17 - "InstrumentNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, unique_ptr, InstrumentNode, blockMidi_ (+6 more)

### Community 18 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 19 - "Clip"
Cohesion: 0.08
Nodes (25): ClipType, FramePosition, Clip, colour, fadeInFrames, fadeOutFrames, gain, id (+17 more)

### Community 20 - "CoreAudioDevice.cpp"
Cohesion: 0.30
Nodes (21): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), enumerateDevices (+13 more)

### Community 21 - "string"
Cohesion: 0.10
Nodes (13): AddChannelCommand, channel_, execute, index_, minted_, undo, size_t, string (+5 more)

### Community 22 - "MidiInput"
Cohesion: 0.11
Nodes (18): FrameCount, MidiBuffer, SampleRate, uint64_t, atomic, queueCapacity, size_t, uint64_t (+10 more)

### Community 23 - "AudioDeviceInfo"
Cohesion: 0.09
Nodes (20): AudioDeviceConfig, bufferSize, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate, AudioDeviceInfo, identifier (+12 more)

### Community 24 - "load"
Cohesion: 0.25
Nodes (17): Result, automationPointFrom(), bindUnassignedContent(), string, idFrom(), midiEventFrom(), pluginFrom(), pluginSlotFrom() (+9 more)

### Community 25 - "PatternCommands.cpp"
Cohesion: 0.11
Nodes (18): undo, Tick, SetPatternLengthCommand, canMergeWith, execute, length_, mergeWith, patternId_ (+10 more)

### Community 26 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 27 - "MidiMessage"
Cohesion: 0.11
Nodes (9): FrameCount, uint8_t, MidiMessage, data1, data2, frameOffset, status, vector (+1 more)

### Community 28 - "Json"
Cohesion: 0.10
Nodes (14): nullptr_t, pair, int64_t, string, vector, Json, asBool, boolean_ (+6 more)

### Community 29 - "MixerNode"
Cohesion: 0.11
Nodes (19): MixerNodeType, PluginIdentifier, string, MixerNode, colour, id, inserts, muted (+11 more)

### Community 30 - "SetVelocityCommand"
Cohesion: 0.11
Nodes (17): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, NoteIndices (+9 more)

### Community 31 - "PlaylistView.mm"
Cohesion: 0.17
Nodes (20): -acceptsFirstResponder, -addTrackRect, -drawBarLinesInLaneAtheight, -drawClips, -drawRect, -drawRuler, -drawTracks, -gridPointFor (+12 more)

### Community 32 - "AudioBufferPool"
Cohesion: 0.14
Nodes (12): AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t, FrameCount (+4 more)

### Community 33 - "AudioDevice"
Cohesion: 0.10
Nodes (19): AudioDevice, actualBufferSize, actualOutputChannels, actualSampleRate, close, create, deviceName, enumerateDevices (+11 more)

### Community 34 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 35 - "AudioBufferView"
Cohesion: 0.20
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 36 - "ProcessContext"
Cohesion: 0.12
Nodes (15): FrameCount, FramePosition, MidiBuffer, SampleRate, size_t, ProcessContext, frameCount, inputCount (+7 more)

### Community 37 - "CompiledGraph"
Cohesion: 0.13
Nodes (13): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+5 more)

### Community 38 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 39 - "Model.cpp"
Cohesion: 0.22
Nodes (17): EntityId, size_t, operator==(), totalEventCount, findChannel, findClip, findMixerNode, findPattern (+9 more)

### Community 40 - "compileArrangement"
Cohesion: 0.26
Nodes (17): Emit, NoteSequence, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto(), EntityId (+9 more)

### Community 41 - "GraphBuilder"
Cohesion: 0.18
Nodes (10): Connection, GraphBuilder, connections_, error_, master_, nodes_, NodeIndex, string (+2 more)

### Community 42 - "SystemInfo"
Cohesion: 0.14
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 43 - "CountingCommand"
Cohesion: 0.12
Nodes (8): CountingCommand, counter_, delta_, string, Tick, makeProjectWithNotes(), NoOpCommand, note()

### Community 44 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 45 - "TimingProbeInstrument"
Cohesion: 0.14
Nodes (10): Applied, FrameCount, MidiBuffer, Sample, SampleRate, vector, renderSynth(), TimingProbeInstrument (+2 more)

### Community 46 - "vector"
Cohesion: 0.14
Nodes (3): mutex, vector, MidiBuffer

### Community 47 - "NoteCommands.cpp"
Cohesion: 0.23
Nodes (16): undo, NoteIndices, size_t, vector, execute, undo, findEvents(), execute (+8 more)

### Community 48 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 49 - "PluginIdentifier"
Cohesion: 0.13
Nodes (14): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+6 more)

### Community 50 - "DuplicateClipsCommand"
Cohesion: 0.16
Nodes (10): ClipIds, DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_ (+2 more)

### Community 51 - "Project"
Cohesion: 0.15
Nodes (16): IdGenerator, Project, audioAssets_, automation_, channels_, clips_, ids_, master_ (+8 more)

### Community 52 - "GraphCompileOptions"
Cohesion: 0.12
Nodes (16): PlaybackSource, GraphCompileOptions, channelCount, instrumentFactory, masterGain, maxBlockSize, pattern, randomSeed (+8 more)

### Community 53 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 54 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 55 - "TimestampedMidiMessage"
Cohesion: 0.22
Nodes (9): midiMessageReceived, sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos (+1 more)

### Community 56 - "EntityId"
Cohesion: 0.21
Nodes (7): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, Value

### Community 57 - "AddPatternClipCommand"
Cohesion: 0.15
Nodes (12): AddPatternClipCommand, clip_, execute, index_, length_, minted_, pattern_, start_ (+4 more)

### Community 58 - "Json.cpp"
Cohesion: 0.22
Nodes (13): int64_t, size_t, string, escapeInto(), formatDouble(), asDouble, asInt, asString (+5 more)

### Community 59 - "Parser"
Cohesion: 0.30
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 60 - "Fixture"
Cohesion: 0.15
Nodes (13): TempoMap, EntityId, SequencedNote, Tick, vector, Fixture, channel, pattern (+5 more)

### Community 61 - "TrackCommands.cpp"
Cohesion: 0.17
Nodes (12): Command, execute, undo, undo, SetTrackHeightCommand, canMergeWith, execute, height_ (+4 more)

### Community 62 - "Node"
Cohesion: 0.21
Nodes (4): atomic, array, Node, process

### Community 63 - "PatternChannelContent"
Cohesion: 0.15
Nodes (11): AutomationLane, id, parameterKey, points, targetEntity, EntityId, vector, PatternChannelContent (+3 more)

### Community 64 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 65 - "MidiEvent"
Cohesion: 0.15
Nodes (13): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+5 more)

### Community 66 - "ClipCommands.cpp"
Cohesion: 0.23
Nodes (11): Command, canMergeWith, mergeWith, ResizeClipsCommand, canMergeWith, clips_, execute, lengthDelta_ (+3 more)

### Community 67 - "MoveClipsCommand"
Cohesion: 0.19
Nodes (11): EntityId, execute, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, execute, tickDelta_ (+3 more)

### Community 68 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (8): size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_, step_

### Community 69 - "RenameTrackCommand"
Cohesion: 0.20
Nodes (6): Command, string, RenameTrackCommand, execute, previousName_, trackId_

### Community 70 - "MidiRecorder"
Cohesion: 0.20
Nodes (10): CapturedMessage, atomic, queueCapacity, size_t, uint64_t, MidiRecorder, captured_, dropped_ (+2 more)

### Community 71 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 72 - "CoreMidiDevice"
Cohesion: 0.16
Nodes (13): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+5 more)

### Community 73 - "ioProcTrampoline"
Cohesion: 0.18
Nodes (12): AudioBufferList, AudioTimeStamp, OSStatus, ioProcTrampoline, renderInto, uint64_t, uint64_t, hostTimeNowNanos() (+4 more)

### Community 74 - "EntityId"
Cohesion: 0.18
Nodes (6): EntityId, SetTrackSoloedCommand, execute, soloed_, trackId_, undo

### Community 75 - "Instrument"
Cohesion: 0.17
Nodes (9): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+1 more)

### Community 76 - "Channel"
Cohesion: 0.17
Nodes (12): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+4 more)

### Community 77 - "Track"
Cohesion: 0.17
Nodes (12): Track, colour, height, id, muted, name, outputMixerNode, parent (+4 more)

### Community 78 - "Denormals.h"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 79 - "AudioAsset"
Cohesion: 0.18
Nodes (11): FrameCount, AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id (+3 more)

### Community 80 - "RemoveChannelCommand"
Cohesion: 0.18
Nodes (9): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, execute, index_ (+1 more)

### Community 81 - "string"
Cohesion: 0.22
Nodes (3): Command, string, vector

### Community 82 - "content"
Cohesion: 0.22
Nodes (10): size_t, Tick, vector, noteAtStep(), execute, undo, vector, content (+2 more)

### Community 83 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 84 - "BlockSegment"
Cohesion: 0.18
Nodes (9): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, size_t (+1 more)

### Community 85 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 86 - "humanizeNoteStarts"
Cohesion: 0.29
Nodes (10): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+2 more)

### Community 87 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 88 - "AddNoteCommand"
Cohesion: 0.22
Nodes (7): AddNoteCommand, channel_, execute, index_, note_, pattern_, size_t

### Community 89 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 90 - "Pattern"
Cohesion: 0.20
Nodes (10): Pattern, automationLanes, channels, colour, id, length, name, swing (+2 more)

### Community 91 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 92 - "RemoveClipsCommand"
Cohesion: 0.22
Nodes (8): string, RemovedClip, RemoveClipsCommand, clips_, execute, name, removed_, undo

### Community 93 - "MidiDeviceInfo"
Cohesion: 0.22
Nodes (7): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback, midiMessageReceived

### Community 94 - "RoutingConnection"
Cohesion: 0.22
Nodes (8): RoutingConnection, destination, gain, id, isSend, preFader, sidechain, source

### Community 95 - "AutomationPoint"
Cohesion: 0.25
Nodes (7): AutomationCurve, AutomationPoint, curve, tension, tick, value, Tick

### Community 97 - "SetClipMutedCommand"
Cohesion: 0.25
Nodes (7): vector, SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 98 - "AddTrackCommand"
Cohesion: 0.25
Nodes (7): AddTrackCommand, execute, index_, minted_, track_, undo, size_t

### Community 99 - "RemoveTrackCommand"
Cohesion: 0.25
Nodes (7): RemovedClip, vector, RemoveTrackCommand, clips_, index_, track_, trackId_

### Community 100 - "SetTrackMutedCommand"
Cohesion: 0.25
Nodes (5): SetTrackMutedCommand, execute, muted_, trackId_, undo

### Community 101 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 102 - "ProjectMetadata"
Cohesion: 0.25
Nodes (8): ProjectMetadata, artist, comment, created, createdWith, lastSavedWith, modified, title

### Community 103 - "CompiledProjectGraph"
Cohesion: 0.22
Nodes (8): CompiledProjectGraph, channels, error, graph, instruments, string, unique_ptr, vector

### Community 104 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 105 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 106 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 107 - "MidiRecorder.cpp"
Cohesion: 0.40
Nodes (4): FramePosition, MidiBuffer, capture, reset

### Community 109 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 110 - "process"
Cohesion: 0.50
Nodes (4): process, FrameCount, FramePosition, MidiBuffer

### Community 123 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped()

## Ambiguous Edges - Review These
- `Content / Sound Library` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Clip / Project Data Model` → `Undo / Redo`  [AMBIGUOUS]
  CLAUDE.md · relation: shares_data_with
- `Pattern System` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Piano Roll` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Step Sequencer` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Time Stretching / Pitch Architecture` → `Open Decisions`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to

## Knowledge Gaps
- **555 isolated node(s):** `noNote`, `resizeHandleWidth`, `selection_`, `viewport_`, `-acceptsFirstResponder` (+550 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **9 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `Content / Sound Library` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Clip / Project Data Model` and `Undo / Redo`?**
  _Edge tagged AMBIGUOUS (relation: shares_data_with) - confidence is low._
- **What is the exact relationship between `Pattern System` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Piano Roll` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Step Sequencer` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Time Stretching / Pitch Architecture` and `Open Decisions`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **Why does `Project` connect `Project` to `DuplicatePatternCommand`, `ChannelCommands.cpp`, `PlaylistModel`, `CommandRegistry`, `compileProjectGraph`, `Clip`, `string`, `load`, `PatternCommands.cpp`, `MixerNode`, `Model.cpp`, `compileArrangement`, `CountingCommand`, `NoteCommands.cpp`, `PluginIdentifier`, `DuplicateClipsCommand`, `AddPatternClipCommand`, `Fixture`, `TrackCommands.cpp`, `PatternChannelContent`, `ClipCommands.cpp`, `MoveClipsCommand`, `RenameTrackCommand`, `EntityId`, `Channel`, `Track`, `AudioAsset`, `RemoveChannelCommand`, `content`, `AddNoteCommand`, `Pattern`, `RemoveClipsCommand`, `RoutingConnection`, `SetClipMutedCommand`, `AddTrackCommand`, `SetTrackMutedCommand`, `ProjectMetadata`?**
  _High betweenness centrality (0.318) - this node is a cross-community bridge._