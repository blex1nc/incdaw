# Graph Report - phade-8b-devam-a14fcf  (2026-08-14)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 1794 nodes · 3207 edges · 102 communities (99 shown, 3 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 197 edges (avg confidence: 0.83)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `1972c70d`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- TempoMap
- AudioEngine
- NoteSequence
- PianoRollModel
- SystemInfo
- Project
- CommandRegistry
- DuplicatePatternCommand
- EntityId
- ChannelRackModel
- CoreAudioDevice
- Command
- ChannelCommands.cpp
- Clip
- AudioBufferView
- SimpleSynth
- MidiMessage
- CoreAudioDevice.cpp
- string
- MidiInput
- AudioDeviceInfo
- CallbackProfiler
- MetronomeNode
- Json
- load
- ProcessContext
- Transport
- AudioBufferPool
- AudioDevice
- RealtimeGuard.cpp
- MixerNode
- CompiledGraph
- vector
- InstrumentNode
- BasicMidiBuffer
- PluginIdentifier
- TimingProbeInstrument
- LockFreeQueue
- compileProjectGraph
- GraphCompileOptions
- GainNode
- TimestampedMidiMessage
- Pattern
- Json.cpp
- Parser
- MidiEvent
- AutomationLane
- NoteCommands.cpp
- SineOscillatorNode
- Transport.cpp
- ChannelRackView.mm
- QuantizeNotesCommand
- ToggleStepCommand
- Node
- MidiRecorder
- CoreMidiDevice.cpp
- GraphBuilder
- CoreMidiDevice
- handlePackets
- SetVelocityCommand
- Instrument
- AudioAsset
- Channel
- Denormals.h
- RemoveChannelCommand
- AddNoteCommand
- MoveNotesCommand
- BlockSegment
- MidiDevice
- humanizeNoteStarts
- Track
- PatternListView.mm
- DeleteNotesCommand
- RecordedEvent
- make-dmg.sh
- SetChannelStepKeyCommand
- ResizeNotesCommand
- RemovePatternCommand
- MidiDeviceInfo
- Model.h
- CompiledProjectGraph
- ConstantNode
- Version
- friend
- ProjectMetadata
- noteAtStep
- ioProcTrampoline
- string
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- OrderRecordingNode
- prepare
- MidiRecorder.cpp
- ChannelRackTests.cpp
- ScratchDirectory
- check

## God Nodes (most connected - your core abstractions)
1. `Project` - 95 edges
2. `EntityId` - 82 edges
3. `AudioEngine` - 48 edges
4. `Json` - 48 edges
5. `Command` - 47 edges
6. `TempoMap` - 44 edges
7. `CoreAudioDevice` - 42 edges
8. `SimpleSynth` - 40 edges
9. `Transport` - 39 edges
10. `MidiEvent` - 35 edges

## Surprising Connections (you probably didn't know these)
- `main()` --calls--> `close`  [INFERRED]
  tools/audiocheck/main.cpp → src/platform/MidiDevice.h
- `Development Phases (0-20)` --semantically_similar_to--> `Feature Roadmap (Phase 0-20)`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Required Documentation Set` --semantically_similar_to--> `Handoff Rule`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Absolute User Control Rule` --semantically_similar_to--> `Critical Operating Rule`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Definition of Done` --semantically_similar_to--> `PROTOTYPE / MOCK / PRODUCTION Labeling`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Approval-Gated Development Protocol** — claude_absolute_user_control_rule, claude_graphify_mandate, claude_feature_workflow, claude_scope_control, claude_dependency_policy, handoff_critical_operating_rule, handoff_feature_protocol, handoff_handoff_rule [EXTRACTED 1.00]
- **Plugin Host Pipeline** — handoff_plugin_scanner, handoff_plugin_registry, handoff_plugin_instance, handoff_parameter_system, handoff_plugin_state_system, handoff_plugin_ui_bridge, handoff_crash_isolation_strategy [EXTRACTED 1.00]
- **Master Signal Chain Convergence** — handoff_midi_signal_flow, handoff_audio_signal_flow, handoff_plugin_automation_flow, handoff_shared_transport_state, claude_mixer, claude_automation, claude_offline_render_engine, claude_core_transport [INFERRED 0.85]

## Communities (102 total, 3 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "TempoMap"
Cohesion: 0.05
Nodes (55): Segment, Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick (+47 more)

### Community 2 - "AudioEngine"
Cohesion: 0.06
Nodes (52): RetiredGraph, AudioEngine, active_, audioDeviceAboutToStart, audioDeviceStopped, availableDevices, blockCounter_, blockMidi_ (+44 more)

### Community 3 - "NoteSequence"
Cohesion: 0.06
Nodes (44): Emit, FrameCount, FramePosition, MidiBuffer, Tick, vector, size_t, Tick (+36 more)

### Community 4 - "PianoRollModel"
Cohesion: 0.09
Nodes (33): NoteList, size_t, Tick, vector, size_t, Tick, vector, PianoRollModel (+25 more)

### Community 5 - "SystemInfo"
Cohesion: 0.06
Nodes (37): NSApplicationDelegate, NSObject, NSScrollView, NSSplitView, NSString, NSTextField, NSWindow, size_t (+29 more)

### Community 6 - "Project"
Cohesion: 0.07
Nodes (24): operator==(), Project, audioAssets_, automation_, channels_, clips_, ids_, master_ (+16 more)

### Community 7 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 8 - "DuplicatePatternCommand"
Cohesion: 0.07
Nodes (19): AddPatternCommand, execute, index_, minted_, pattern_, undo, DuplicatePatternCommand, execute (+11 more)

### Community 9 - "EntityId"
Cohesion: 0.11
Nodes (21): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, size_t, vector (+13 more)

### Community 10 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, Rect, ChannelRackModel, contentHeight, hitTest, layout_, muteRect (+17 more)

### Community 11 - "CoreAudioDevice"
Cohesion: 0.08
Nodes (21): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, callback_, close, deviceID_, inputLatency_, name_ (+13 more)

### Community 12 - "Command"
Cohesion: 0.09
Nodes (22): Command, execute, id, name, undo, Tick, SetPatternLengthCommand, canMergeWith (+14 more)

### Community 13 - "ChannelCommands.cpp"
Cohesion: 0.08
Nodes (20): undo, undo, SetChannelMutedCommand, channelId_, execute, muted_, undo, SetChannelSoloedCommand (+12 more)

### Community 14 - "Clip"
Cohesion: 0.08
Nodes (25): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+17 more)

### Community 15 - "AudioBufferView"
Cohesion: 0.15
Nodes (13): renderAudioBlock, uint64_t, AudioBufferView, channels_, frames_, offset_, FrameCount, Sample (+5 more)

### Community 16 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 17 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 18 - "CoreAudioDevice.cpp"
Cohesion: 0.26
Nodes (22): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), enumerateDevices (+14 more)

### Community 19 - "string"
Cohesion: 0.10
Nodes (12): AddChannelCommand, channel_, execute, index_, minted_, size_t, string, RenameChannelCommand (+4 more)

### Community 20 - "MidiInput"
Cohesion: 0.11
Nodes (19): FrameCount, MidiBuffer, SampleRate, uint64_t, atomic, queueCapacity, size_t, uint64_t (+11 more)

### Community 21 - "AudioDeviceInfo"
Cohesion: 0.09
Nodes (20): AudioDeviceConfig, bufferSize, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate, AudioDeviceInfo, identifier (+12 more)

### Community 22 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 23 - "MetronomeNode"
Cohesion: 0.09
Nodes (17): atomic, FrameCount, Sample, SampleRate, size_t, vector, MetronomeNode, amplitude_ (+9 more)

### Community 24 - "Json"
Cohesion: 0.10
Nodes (14): nullptr_t, pair, int64_t, string, vector, Json, asBool, boolean_ (+6 more)

### Community 25 - "load"
Cohesion: 0.23
Nodes (19): Result, append, automationPointFrom(), bindUnassignedContent(), string, idFrom(), midiEventFrom(), pluginFrom() (+11 more)

### Community 26 - "ProcessContext"
Cohesion: 0.10
Nodes (16): process, FrameCount, FramePosition, MidiBuffer, SampleRate, size_t, ProcessContext, frameCount (+8 more)

### Community 27 - "Transport"
Cohesion: 0.13
Nodes (11): atomic, FramePosition, size_t, Tick, Transport, loopEnabled_, maxSegmentsPerBlock, seekRequested_ (+3 more)

### Community 28 - "AudioBufferPool"
Cohesion: 0.14
Nodes (12): AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t, FrameCount (+4 more)

### Community 29 - "AudioDevice"
Cohesion: 0.10
Nodes (19): AudioDevice, actualBufferSize, actualOutputChannels, actualSampleRate, close, create, deviceName, enumerateDevices (+11 more)

### Community 30 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 31 - "MixerNode"
Cohesion: 0.11
Nodes (18): MixerNodeType, string, MixerNode, colour, id, inserts, muted, name (+10 more)

### Community 32 - "CompiledGraph"
Cohesion: 0.13
Nodes (13): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+5 more)

### Community 33 - "vector"
Cohesion: 0.16
Nodes (3): mutex, vector, MidiBuffer

### Community 34 - "InstrumentNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, unique_ptr, InstrumentNode, blockMidi_ (+6 more)

### Community 35 - "BasicMidiBuffer"
Cohesion: 0.13
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 36 - "PluginIdentifier"
Cohesion: 0.15
Nodes (12): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+4 more)

### Community 37 - "TimingProbeInstrument"
Cohesion: 0.14
Nodes (10): Applied, FrameCount, MidiBuffer, Sample, SampleRate, vector, renderSynth(), TimingProbeInstrument (+2 more)

### Community 38 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 39 - "compileProjectGraph"
Cohesion: 0.17
Nodes (15): NodeIndex, SampleRate, size_t, unique_ptr, addNode, compile, connect, instrumentFor (+7 more)

### Community 40 - "GraphCompileOptions"
Cohesion: 0.12
Nodes (16): PlaybackSource, GraphCompileOptions, channelCount, instrumentFactory, masterGain, maxBlockSize, pattern, randomSeed (+8 more)

### Community 41 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 42 - "TimestampedMidiMessage"
Cohesion: 0.13
Nodes (15): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status (+7 more)

### Community 43 - "Pattern"
Cohesion: 0.13
Nodes (15): Tick, uint32_t, Pattern, automationLanes, channels, colour, id, length (+7 more)

### Community 44 - "Json.cpp"
Cohesion: 0.22
Nodes (13): int64_t, size_t, string, escapeInto(), formatDouble(), asDouble, asInt, asString (+5 more)

### Community 45 - "Parser"
Cohesion: 0.30
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 46 - "MidiEvent"
Cohesion: 0.13
Nodes (15): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+7 more)

### Community 47 - "AutomationLane"
Cohesion: 0.14
Nodes (12): AutomationCurve, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint, curve (+4 more)

### Community 48 - "NoteCommands.cpp"
Cohesion: 0.27
Nodes (13): undo, NoteIndices, size_t, vector, execute, findEvents(), execute, undo (+5 more)

### Community 49 - "SineOscillatorNode"
Cohesion: 0.16
Nodes (8): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, sampleRate_

### Community 50 - "Transport.cpp"
Cohesion: 0.18
Nodes (13): FrameCount, FramePosition, size_t, applyPendingSeek, pause, play, processBlock, seek (+5 more)

### Community 51 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 52 - "QuantizeNotesCommand"
Cohesion: 0.17
Nodes (9): Tick, QuantizeNotesCommand, channel_, execute, grid_, pattern_, previousEvents_, strength_ (+1 more)

### Community 53 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (8): size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_, step_

### Community 54 - "Node"
Cohesion: 0.18
Nodes (4): atomic, array, Node, process

### Community 55 - "MidiRecorder"
Cohesion: 0.20
Nodes (10): CapturedMessage, atomic, queueCapacity, size_t, uint64_t, MidiRecorder, captured_, dropped_ (+2 more)

### Community 56 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 57 - "GraphBuilder"
Cohesion: 0.18
Nodes (10): Connection, GraphBuilder, connections_, error_, master_, nodes_, NodeIndex, string (+2 more)

### Community 58 - "CoreMidiDevice"
Cohesion: 0.18
Nodes (10): MIDIClientRef, MIDIPortRef, CoreMidiDevice, callback_, client_, close, inputPort_, outputEndpoint_ (+2 more)

### Community 59 - "handlePackets"
Cohesion: 0.24
Nodes (9): MIDIPacketList, handlePackets, readProc, uint64_t, hostTimeNowNanos(), hostTimeToNanos(), Timebase, denominator (+1 more)

### Community 60 - "SetVelocityCommand"
Cohesion: 0.18
Nodes (9): NoteIndices, SetVelocityCommand, canMergeWith, channel_, indices_, mergeWith, pattern_, previousVelocities_ (+1 more)

### Community 61 - "Instrument"
Cohesion: 0.18
Nodes (9): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+1 more)

### Community 62 - "AudioAsset"
Cohesion: 0.17
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 63 - "Channel"
Cohesion: 0.17
Nodes (12): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+4 more)

### Community 64 - "Denormals.h"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 65 - "RemoveChannelCommand"
Cohesion: 0.18
Nodes (8): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, execute, index_

### Community 66 - "AddNoteCommand"
Cohesion: 0.20
Nodes (7): AddNoteCommand, channel_, execute, index_, note_, pattern_, size_t

### Community 67 - "MoveNotesCommand"
Cohesion: 0.18
Nodes (10): MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, canMergeWith, channel_, indices_, keyDelta_, mergeWith (+2 more)

### Community 68 - "BlockSegment"
Cohesion: 0.18
Nodes (9): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, size_t (+1 more)

### Community 69 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 70 - "humanizeNoteStarts"
Cohesion: 0.29
Nodes (10): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+2 more)

### Community 71 - "Track"
Cohesion: 0.18
Nodes (11): Track, colour, height, id, muted, name, outputMixerNode, parent (+3 more)

### Community 72 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 73 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (9): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo (+1 more)

### Community 75 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 76 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 77 - "SetChannelStepKeyCommand"
Cohesion: 0.22
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 78 - "ResizeNotesCommand"
Cohesion: 0.22
Nodes (8): ResizeNotesCommand, canMergeWith, channel_, durationDelta_, indices_, mergeWith, pattern_, previousDurations_

### Community 79 - "RemovePatternCommand"
Cohesion: 0.22
Nodes (7): size_t, RemovePatternCommand, execute, index_, pattern_, patternId_, undo

### Community 80 - "MidiDeviceInfo"
Cohesion: 0.22
Nodes (7): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback, midiMessageReceived

### Community 81 - "Model.h"
Cohesion: 0.22
Nodes (8): RoutingConnection, destination, gain, id, isSend, preFader, sidechain, source

### Community 82 - "CompiledProjectGraph"
Cohesion: 0.22
Nodes (8): CompiledProjectGraph, channels, error, graph, instruments, string, unique_ptr, vector

### Community 83 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 85 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 88 - "ProjectMetadata"
Cohesion: 0.25
Nodes (8): ProjectMetadata, artist, comment, created, createdWith, lastSavedWith, modified, title

### Community 89 - "noteAtStep"
Cohesion: 0.33
Nodes (6): size_t, Tick, vector, noteAtStep(), execute, undo

### Community 90 - "ioProcTrampoline"
Cohesion: 0.40
Nodes (6): AudioBufferList, AudioTimeStamp, OSStatus, ioProcTrampoline, renderInto, uint64_t

### Community 92 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 93 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 94 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 95 - "prepare"
Cohesion: 0.40
Nodes (4): FrameCount, SampleRate, prepare, triggerClick

### Community 96 - "MidiRecorder.cpp"
Cohesion: 0.40
Nodes (4): FramePosition, MidiBuffer, capture, reset

### Community 97 - "ChannelRackTests.cpp"
Cohesion: 0.40
Nodes (4): Step, Tick, note(), stepAt()

### Community 99 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

## Ambiguous Edges - Review These
- `Content / Sound Library` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Pattern System` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Piano Roll` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Step Sequencer` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Clip / Project Data Model` → `Undo / Redo`  [AMBIGUOUS]
  CLAUDE.md · relation: shares_data_with
- `Time Stretching / Pitch Architecture` → `Open Decisions`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to

## Knowledge Gaps
- **495 isolated node(s):** `bar`, `beat`, `tick`, `denominator`, `numerator` (+490 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **3 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `Content / Sound Library` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Pattern System` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Piano Roll` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Step Sequencer` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Clip / Project Data Model` and `Undo / Redo`?**
  _Edge tagged AMBIGUOUS (relation: shares_data_with) - confidence is low._
- **What is the exact relationship between `Time Stretching / Pitch Architecture` and `Open Decisions`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **Why does `TempoMap` connect `TempoMap` to `vector`, `InstrumentNode`, `NoteSequence`, `BlockSegment`, `ChannelRackTests.cpp`, `Project`, `compileProjectGraph`, `TimestampedMidiMessage`, `MidiMessage`, `Transport.cpp`, `Model.h`, `CompiledProjectGraph`, `Node`, `Transport`?**
  _High betweenness centrality (0.156) - this node is a cross-community bridge._