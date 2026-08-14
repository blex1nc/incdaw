# Graph Report - phade-8b-devam-a14fcf  (2026-08-14)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 2377 nodes · 4252 edges · 148 communities (133 shown, 15 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 234 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `f9f4e2e9`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- PlaylistModel
- -applicationDidFinishLaunching
- NoteCommands.cpp
- PianoRollModel
- SimpleSynth
- Channel
- vector
- DuplicatePatternCommand
- Project
- NoteSequence
- PatternChannelContent
- Transport
- CommandRegistry
- CoreAudioDevice
- Command
- ChannelRackModel
- TempoMap
- ChannelCommands.cpp
- MetronomeNode
- MixerStripNode
- MusicalPosition
- MixerView.mm
- EntityId
- Clip
- AudioDeviceInfo
- CallbackProfiler
- MidiMessage
- CoreAudioDevice.cpp
- Json
- MidiInput
- load
- AudioBufferView
- LevelMeter
- PlaylistView.mm
- AudioBufferPool
- CompiledGraph
- AudioDevice
- RealtimeGuard.cpp
- compileArrangement
- DelayLineNode
- ProcessContext
- BasicMidiBuffer
- SystemInfo
- CoreMidiDevice
- AudioEngine
- MixerCommands.cpp
- InstrumentNode
- PluginIdentifier
- ConstantNode
- TimingProbeInstrument
- compile
- ClipCommands.cpp
- LockFreeQueue
- Smoother
- LatentProcessorNode
- AudioEngine.cpp
- GainNode
- SineOscillatorNode
- RoutingConnection
- GraphBuilder
- MidiEvent
- GraphCompileOptions
- ResizeNotesCommand
- TrackCommands.cpp
- Json.cpp
- Parser
- CompiledProjectGraph
- render
- ioProcTrampoline
- string
- EntityId
- main
- renderClickFrames
- ChannelRackView.mm
- AddPatternClipCommand
- ToggleStepCommand
- AddTrackCommand
- atomic
- MidiRecorder
- SetTrackSoloedCommand
- Instrument
- Track
- Denormals.h
- RemoveMixerNodeCommand
- AddMixerNodeCommand
- AddNoteCommand
- MixerFixture
- TimeSignatureEvent
- MidiDevice
- humanizeNoteStarts
- AudioAsset
- compileProjectGraph
- PatternListView.mm
- Options
- enumerateInputs
- ResizeClipsCommand
- MixerCommands.h
- RecordedEvent
- make-dmg.sh
- RemoveClipsCommand
- DuplicateClipsCommand
- MoveClipsCommand
- SetClipMutedCommand
- DisconnectMixerCommand
- SetMixerVolumeCommand
- SetSendGainCommand
- RemoveTrackCommand
- TimestampedMidiMessage
- MidiDeviceInfo
- SetChannelSoloedCommand
- SetMixerMutedCommand
- SetMixerPolarityCommand
- SetMixerSoloedCommand
- SetTrackMutedCommand
- Version
- ProjectMetadata
- PlaylistTests.cpp
- MidiTests.cpp
- Pattern
- Fixture
- string
- MixerTests.cpp
- EntityId
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- OrderRecordingNode
- MidiRecorder.cpp
- ScratchDirectory
- check
- TrackCommands.h
- .operator==
- FrameCount
- FramePosition
- MixerNodeType
- MidiBuffer
- Step
- friend
- MidiEventType
- Tick
- uint32_t
- uint64_t
- uint64_t

## God Nodes (most connected - your core abstractions)
1. `Project` - 162 edges
2. `EntityId` - 52 edges
3. `Json` - 48 edges
4. `Command` - 47 edges
5. `AudioEngine` - 47 edges
6. `CoreAudioDevice` - 42 edges
7. `TempoMap` - 40 edges
8. `Transport` - 39 edges
9. `SimpleSynth` - 39 edges
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

## Communities (148 total, 15 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "PlaylistModel"
Cohesion: 0.07
Nodes (37): EntityId, size_t, Tick, vector, EntityId, size_t, Tick, PlaylistModel (+29 more)

### Community 2 - "-applicationDidFinishLaunching"
Cohesion: 0.05
Nodes (47): incdaw, INCDAWChannelRackView, INCDAWPatternListView, INCDAWPianoRollView, INCDAWPlaylistView, NSApplicationDelegate, NSObject, NSScrollView (+39 more)

### Community 3 - "NoteCommands.cpp"
Cohesion: 0.06
Nodes (47): NoteIndices, size_t, string, vector, DeleteNotesCommand, channel_, execute, indices_ (+39 more)

### Community 4 - "PianoRollModel"
Cohesion: 0.08
Nodes (35): NoteList, size_t, Tick, vector, size_t, Tick, vector, PianoRollModel (+27 more)

### Community 5 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 6 - "Channel"
Cohesion: 0.07
Nodes (32): PluginIdentifier, Channel, colour, id, instrument, instrumentStateFile, muted, name (+24 more)

### Community 7 - "vector"
Cohesion: 0.06
Nodes (21): RemovedContent, vector, AddChannelCommand, channel_, execute, index_, minted_, undo (+13 more)

### Community 8 - "DuplicatePatternCommand"
Cohesion: 0.06
Nodes (23): AddPatternCommand, index_, minted_, pattern_, DuplicatePatternCommand, execute, index_, minted_ (+15 more)

### Community 9 - "Project"
Cohesion: 0.12
Nodes (36): IdGenerator, undo, EntityId, size_t, vector, operator==(), events, totalEventCount (+28 more)

### Community 10 - "NoteSequence"
Cohesion: 0.08
Nodes (30): FrameCount, FramePosition, MidiBuffer, Tick, vector, size_t, Tick, uint32_t (+22 more)

### Community 11 - "PatternChannelContent"
Cohesion: 0.08
Nodes (18): AutomationCurve, friend, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint (+10 more)

### Community 12 - "Transport"
Cohesion: 0.09
Nodes (24): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, Transport (+16 more)

### Community 13 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 14 - "CoreAudioDevice"
Cohesion: 0.08
Nodes (22): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, callback_, close, deviceID_, inputLatency_, name_ (+14 more)

### Community 15 - "Command"
Cohesion: 0.08
Nodes (25): Command, execute, id, name, undo, execute, undo, Tick (+17 more)

### Community 16 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, Rect, ChannelRackModel, contentHeight, hitTest, layout_, muteRect (+17 more)

### Community 17 - "TempoMap"
Cohesion: 0.11
Nodes (28): Segment, process, triggerClick, clampTempo(), FramePosition, SampleRate, Tick, vector (+20 more)

### Community 18 - "ChannelCommands.cpp"
Cohesion: 0.08
Nodes (21): execute, undo, SetChannelMutedCommand, channelId_, execute, muted_, undo, SetChannelStepKeyCommand (+13 more)

### Community 19 - "MetronomeNode"
Cohesion: 0.08
Nodes (20): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+12 more)

### Community 20 - "MixerStripNode"
Cohesion: 0.11
Nodes (21): FrameCount, ProcessContext, Sample, SampleRate, atomic, Node, Sample, MixerStripNode (+13 more)

### Community 21 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 22 - "MixerView.mm"
Cohesion: 0.18
Nodes (25): NSMenu, -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect, -drawStripnode (+17 more)

### Community 23 - "EntityId"
Cohesion: 0.12
Nodes (11): NoteIndices, Tick, EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId> (+3 more)

### Community 24 - "Clip"
Cohesion: 0.08
Nodes (25): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+17 more)

### Community 25 - "AudioDeviceInfo"
Cohesion: 0.09
Nodes (20): AudioDeviceConfig, bufferSize, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate, AudioDeviceInfo, identifier (+12 more)

### Community 26 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 27 - "MidiMessage"
Cohesion: 0.11
Nodes (9): FrameCount, uint8_t, MidiMessage, data1, data2, frameOffset, status, vector (+1 more)

### Community 28 - "CoreAudioDevice.cpp"
Cohesion: 0.30
Nodes (21): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), enumerateDevices (+13 more)

### Community 29 - "Json"
Cohesion: 0.10
Nodes (14): nullptr_t, pair, int64_t, string, vector, Json, asBool, boolean_ (+6 more)

### Community 30 - "MidiInput"
Cohesion: 0.11
Nodes (18): FrameCount, MidiBuffer, SampleRate, uint64_t, atomic, queueCapacity, size_t, uint64_t (+10 more)

### Community 31 - "load"
Cohesion: 0.23
Nodes (19): Result, append, automationPointFrom(), bindUnassignedContent(), string, idFrom(), midiEventFrom(), pluginFrom() (+11 more)

### Community 32 - "AudioBufferView"
Cohesion: 0.19
Nodes (9): renderAudioBlock, uint64_t, AudioBufferView, channels_, frames_, offset_, FrameCount, Sample (+1 more)

### Community 33 - "LevelMeter"
Cohesion: 0.13
Nodes (13): atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond (+5 more)

### Community 34 - "PlaylistView.mm"
Cohesion: 0.17
Nodes (20): -acceptsFirstResponder, -addTrackRect, -drawBarLinesInLaneAtheight, -drawClips, -drawRect, -drawRuler, -drawTracks, -gridPointFor (+12 more)

### Community 35 - "AudioBufferPool"
Cohesion: 0.14
Nodes (12): AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t, FrameCount (+4 more)

### Community 36 - "CompiledGraph"
Cohesion: 0.12
Nodes (14): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+6 more)

### Community 37 - "AudioDevice"
Cohesion: 0.10
Nodes (19): AudioDevice, actualBufferSize, actualOutputChannels, actualSampleRate, close, create, deviceName, enumerateDevices (+11 more)

### Community 38 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 39 - "compileArrangement"
Cohesion: 0.23
Nodes (19): Emit, NoteSequence, content, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto() (+11 more)

### Community 40 - "DelayLineNode"
Cohesion: 0.13
Nodes (15): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+7 more)

### Community 41 - "ProcessContext"
Cohesion: 0.12
Nodes (15): FrameCount, FramePosition, MidiBuffer, SampleRate, size_t, ProcessContext, frameCount, inputCount (+7 more)

### Community 42 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 43 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 44 - "CoreMidiDevice"
Cohesion: 0.17
Nodes (15): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+7 more)

### Community 45 - "AudioEngine"
Cohesion: 0.13
Nodes (15): mutex, RetiredGraph, AudioEngine, active_, blockCounter_, blockMidi_, device_, nonFiniteBlocks_ (+7 more)

### Community 46 - "MixerCommands.cpp"
Cohesion: 0.16
Nodes (14): execute, Command, SetMixerPanCommand, canMergeWith, execute, mergeWith, nodeId_, pan_ (+6 more)

### Community 47 - "InstrumentNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, unique_ptr, InstrumentNode, blockMidi_ (+6 more)

### Community 48 - "PluginIdentifier"
Cohesion: 0.15
Nodes (12): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+4 more)

### Community 49 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 50 - "TimingProbeInstrument"
Cohesion: 0.14
Nodes (10): Applied, FrameCount, MidiBuffer, Sample, SampleRate, vector, renderSynth(), TimingProbeInstrument (+2 more)

### Community 51 - "compile"
Cohesion: 0.17
Nodes (16): MidiBuffer, process, AudioBufferView, FrameCount, FramePosition, Node, NodeIndex, SampleRate (+8 more)

### Community 52 - "ClipCommands.cpp"
Cohesion: 0.18
Nodes (15): execute, undo, Command, EntityId, execute, undo, canMergeWith, execute (+7 more)

### Community 53 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 54 - "Smoother"
Cohesion: 0.18
Nodes (10): atomic, AudioBufferView, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds (+2 more)

### Community 55 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 56 - "AudioEngine.cpp"
Cohesion: 0.17
Nodes (14): audioDeviceAboutToStart, audioDeviceStopped, bufferSize, collectRetiredGraphs, isRunning, outputChannels, retiredGraphCount, sampleRate (+6 more)

### Community 57 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 58 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 59 - "RoutingConnection"
Cohesion: 0.12
Nodes (14): EntityId, findRouting, ids_, metadata_, tempoMap_, RoutingConnection, destination, gain (+6 more)

### Community 60 - "GraphBuilder"
Cohesion: 0.14
Nodes (12): Connection, GraphBuilder, compensate_, connections_, error_, master_, nodes_, Node (+4 more)

### Community 61 - "MidiEvent"
Cohesion: 0.13
Nodes (15): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+7 more)

### Community 62 - "GraphCompileOptions"
Cohesion: 0.13
Nodes (15): PlaybackSource, GraphCompileOptions, channelCount, instrumentFactory, masterGain, maxBlockSize, pattern, randomSeed (+7 more)

### Community 63 - "ResizeNotesCommand"
Cohesion: 0.13
Nodes (9): string, ResizeNotesCommand, canMergeWith, channel_, durationDelta_, indices_, mergeWith, pattern_ (+1 more)

### Community 64 - "TrackCommands.cpp"
Cohesion: 0.17
Nodes (12): undo, Command, execute, SetTrackHeightCommand, canMergeWith, execute, height_, mergeWith (+4 more)

### Community 65 - "Json.cpp"
Cohesion: 0.22
Nodes (13): int64_t, size_t, string, escapeInto(), formatDouble(), asDouble, asInt, asString (+5 more)

### Community 66 - "Parser"
Cohesion: 0.30
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 67 - "CompiledProjectGraph"
Cohesion: 0.14
Nodes (14): TempoMap, CompiledProjectGraph, channels, channelStrips, error, graph, instruments, mixerNodes (+6 more)

### Community 68 - "render"
Cohesion: 0.16
Nodes (11): AudioBufferPool, AudioBufferView, ProcessContext, Sample, size_t, vector, channel, onsets() (+3 more)

### Community 69 - "ioProcTrampoline"
Cohesion: 0.20
Nodes (12): AudioBufferList, AudioTimeStamp, OSStatus, ioProcTrampoline, renderInto, uint64_t, uint64_t, hostTimeNowNanos() (+4 more)

### Community 70 - "string"
Cohesion: 0.16
Nodes (6): string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 71 - "EntityId"
Cohesion: 0.16
Nodes (11): ConnectMixerCommand, connection_, destination_, gain_, index_, isSend_, minted_, preFader_ (+3 more)

### Community 72 - "main"
Cohesion: 0.20
Nodes (13): availableDevices, deviceName, midiInput_, profiler_, start, transport_, string, vector (+5 more)

### Community 73 - "renderClickFrames"
Cohesion: 0.15
Nodes (12): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, FramePosition (+4 more)

### Community 74 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 75 - "AddPatternClipCommand"
Cohesion: 0.18
Nodes (10): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+2 more)

### Community 76 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (8): size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_, step_

### Community 77 - "AddTrackCommand"
Cohesion: 0.18
Nodes (7): AddTrackCommand, execute, index_, minted_, track_, size_t, string

### Community 78 - "atomic"
Cohesion: 0.22
Nodes (4): atomic, array, Node, process

### Community 79 - "MidiRecorder"
Cohesion: 0.20
Nodes (10): CapturedMessage, atomic, queueCapacity, size_t, uint64_t, MidiRecorder, captured_, dropped_ (+2 more)

### Community 80 - "SetTrackSoloedCommand"
Cohesion: 0.25
Nodes (5): SetTrackSoloedCommand, execute, soloed_, trackId_, undo

### Community 81 - "Instrument"
Cohesion: 0.18
Nodes (9): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+1 more)

### Community 82 - "Track"
Cohesion: 0.17
Nodes (12): findTrack, Track, colour, height, id, muted, name, outputMixerNode (+4 more)

### Community 83 - "Denormals.h"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 84 - "RemoveMixerNodeCommand"
Cohesion: 0.18
Nodes (9): RemovedRouting, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_, routing_ (+1 more)

### Community 85 - "AddMixerNodeCommand"
Cohesion: 0.20
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 86 - "AddNoteCommand"
Cohesion: 0.20
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, size_t

### Community 87 - "MixerFixture"
Cohesion: 0.18
Nodes (8): buildParallelPaths(), EntityId, TempoMap, unique_ptr, MixerFixture, pattern, project, tempo

### Community 88 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 89 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 90 - "humanizeNoteStarts"
Cohesion: 0.29
Nodes (10): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+2 more)

### Community 91 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 92 - "compileProjectGraph"
Cohesion: 0.25
Nodes (10): channelStripFor, instrumentFor, stripFor, compileProjectGraph(), EntityId, InstrumentFactory, InstrumentNode, TempoMap (+2 more)

### Community 93 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 94 - "Options"
Cohesion: 0.18
Nodes (11): int64_t, Options, amplitude, buffer, device, frequency, listOnly, midi (+3 more)

### Community 95 - "enumerateInputs"
Cohesion: 0.31
Nodes (10): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, string, vector, endpointIdentifier() (+2 more)

### Community 96 - "ResizeClipsCommand"
Cohesion: 0.22
Nodes (7): Command, ResizeClipsCommand, clips_, execute, lengthDelta_, previousLengths_, undo

### Community 97 - "MixerCommands.h"
Cohesion: 0.22
Nodes (7): Command, SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 98 - "RecordedEvent"
Cohesion: 0.12
Nodes (11): MidiBuffer, Kind, Tick, RecordedEvent, channel, duration, key, kind (+3 more)

### Community 99 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 100 - "RemoveClipsCommand"
Cohesion: 0.25
Nodes (7): ClipIds, string, RemovedClip, RemoveClipsCommand, clips_, name, removed_

### Community 101 - "DuplicateClipsCommand"
Cohesion: 0.22
Nodes (7): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_

### Community 102 - "MoveClipsCommand"
Cohesion: 0.25
Nodes (7): Tick, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, tickDelta_, trackDelta_

### Community 103 - "SetClipMutedCommand"
Cohesion: 0.22
Nodes (7): vector, SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 104 - "DisconnectMixerCommand"
Cohesion: 0.22
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 105 - "SetMixerVolumeCommand"
Cohesion: 0.22
Nodes (6): SetMixerVolumeCommand, execute, nodeId_, previous_, undo, volume_

### Community 106 - "SetSendGainCommand"
Cohesion: 0.22
Nodes (6): SetSendGainCommand, connectionId_, execute, gain_, previous_, undo

### Community 107 - "RemoveTrackCommand"
Cohesion: 0.22
Nodes (8): RemovedClip, vector, RemoveTrackCommand, clips_, index_, track_, trackId_, undo

### Community 108 - "TimestampedMidiMessage"
Cohesion: 0.25
Nodes (8): midiMessageReceived, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status

### Community 109 - "MidiDeviceInfo"
Cohesion: 0.22
Nodes (7): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback, midiMessageReceived

### Community 110 - "SetChannelSoloedCommand"
Cohesion: 0.25
Nodes (5): SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 111 - "SetMixerMutedCommand"
Cohesion: 0.25
Nodes (5): SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 112 - "SetMixerPolarityCommand"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 113 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 114 - "SetTrackMutedCommand"
Cohesion: 0.33
Nodes (5): Command, SetTrackMutedCommand, muted_, trackId_, undo

### Community 115 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 116 - "ProjectMetadata"
Cohesion: 0.25
Nodes (8): ProjectMetadata, artist, comment, created, createdWith, lastSavedWith, modified, title

### Community 117 - "PlaylistTests.cpp"
Cohesion: 0.29
Nodes (6): TempoMap, SequencedNote, Tick, vector, note(), shapeFrom()

### Community 118 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped()

### Community 119 - "Pattern"
Cohesion: 0.14
Nodes (14): size_t, Tick, vector, noteAtStep(), execute, Pattern, automationLanes, channels (+6 more)

### Community 120 - "Fixture"
Cohesion: 0.29
Nodes (7): EntityId, Fixture, channel, pattern, project, trackA, trackB

### Community 122 - "MixerTests.cpp"
Cohesion: 0.47
Nodes (3): vector, Node, AudioBufferPool

### Community 123 - "EntityId"
Cohesion: 0.18
Nodes (6): EntityId, RenameTrackCommand, execute, previousName_, trackId_, undo

### Community 124 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 125 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 126 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 127 - "MidiRecorder.cpp"
Cohesion: 0.40
Nodes (4): FramePosition, MidiBuffer, capture, reset

### Community 129 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

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
- **636 isolated node(s):** `counter_`, `delta_`, `noClip`, `noTrack`, `resizeHandleWidth` (+631 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **15 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `PlaylistModel`, `NoteCommands.cpp`, `Channel`, `vector`, `DuplicatePatternCommand`, `PatternChannelContent`, `CommandRegistry`, `Command`, `ChannelCommands.cpp`, `Clip`, `load`, `compileArrangement`, `MixerCommands.cpp`, `ClipCommands.cpp`, `RoutingConnection`, `TrackCommands.cpp`, `CompiledProjectGraph`, `string`, `EntityId`, `AddTrackCommand`, `SetTrackSoloedCommand`, `Track`, `RemoveMixerNodeCommand`, `AddMixerNodeCommand`, `AddNoteCommand`, `MixerFixture`, `AudioAsset`, `compileProjectGraph`, `ResizeClipsCommand`, `MixerCommands.h`, `SetClipMutedCommand`, `DisconnectMixerCommand`, `SetMixerVolumeCommand`, `SetSendGainCommand`, `RemoveTrackCommand`, `SetChannelSoloedCommand`, `SetMixerMutedCommand`, `SetMixerPolarityCommand`, `SetMixerSoloedCommand`, `SetTrackMutedCommand`, `ProjectMetadata`, `Pattern`, `Fixture`, `EntityId`?**
  _High betweenness centrality (0.327) - this node is a cross-community bridge._