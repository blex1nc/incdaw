# Graph Report - phade-8b-devam-a14fcf  (2026-08-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 2505 nodes · 4441 edges · 155 communities (138 shown, 17 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 236 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `56743bdd`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- AudioEngine
- NoteSequence
- PianoRollModel
- SimpleSynth
- NoteCommands.cpp
- EntityId
- Project
- DuplicatePatternCommand
- ChannelCommands.cpp
- PlaylistModel
- Transport
- CommandRegistry
- CoreAudioDevice
- vector
- ChannelRackModel
- Command
- TempoMap
- AddAutomationLaneCommand
- PatternChannelContent
- Channel
- MetronomeNode
- CompiledProjectGraph
- MusicalPosition
- MixerView.mm
- CoreAudioDevice.cpp
- Clip
- MidiInput
- AudioDeviceInfo
- CallbackProfiler
- MidiMessage
- AudioBufferPool
- Json
- AutomationNode
- load
- AudioBufferView
- atomic
- LevelMeter
- ProcessContext
- RecordedEvent
- AudioDevice
- PlaylistView.mm
- ToggleStepCommand
- CompiledGraph
- RealtimeGuard.cpp
- EntityId
- DelayLineNode
- BasicMidiBuffer
- MixerCommands.cpp
- InstrumentNode
- SystemInfo
- PluginIdentifier
- CountingCommand
- ConstantNode
- TimingProbeInstrument
- AutomationPoint
- GraphCompileOptions
- compile
- ClipCommands.cpp
- AddNoteCommand
- LockFreeQueue
- Smoother
- MixerStripNode
- MixerTests.cpp
- LatentProcessorNode
- AutomationCommands.cpp
- GainNode
- TimestampedMidiMessage
- GraphBuilder
- -applicationDidFinishLaunching
- TrackCommands.cpp
- Json.cpp
- Parser
- main.mm
- Fixture
- string
- EntityId
- SineOscillatorNode
- renderClickFrames
- ChannelRackView.mm
- MidiEvent
- AddPatternClipCommand
- AddTrackCommand
- MixerNode
- ParameterRegistry
- MidiRecorder
- CoreMidiDevice.cpp
- CoreMidiDevice
- handlePackets
- Instrument
- RoutingConnection
- Denormals.h
- RemoveMixerNodeCommand
- AddMixerNodeCommand
- SetTrackMutedCommand
- MixerStripNode.cpp
- MixerFixture
- TimeSignatureEvent
- MidiDevice
- AudioAsset
- Track
- PatternListView.mm
- ResizeClipsCommand
- MixerCommands.h
- Pattern
- AutomationFixture
- make-dmg.sh
- RemoveClipsCommand
- SetChannelStepKeyCommand
- DuplicateClipsCommand
- MoveClipsCommand
- SetClipMutedCommand
- DisconnectMixerCommand
- SetMixerVolumeCommand
- SetSendGainCommand
- MidiDeviceInfo
- SetMixerMutedCommand
- SetMixerPolarityCommand
- SetMixerSoloedCommand
- RemoveTrackCommand
- Version
- ioProcTrampoline
- INCDAWMixerView
- INCDAWPianoRollView
- string
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- OrderRecordingNode
- vector
- MidiRecorder.cpp
- ScratchDirectory
- check
- INCDAWChannelRackView
- INCDAWPatternListView
- INCDAWPlaylistView
- .operator==
- MixerNodeType
- MidiBuffer
- Step
- T
- uint8_t
- friend
- MidiEventType
- Tick
- uint32_t
- uint64_t
- FrameCount
- Sample
- SampleRate
- NSView

## God Nodes (most connected - your core abstractions)
1. `Project` - 161 edges
2. `EntityId` - 52 edges
3. `Json` - 48 edges
4. `AudioEngine` - 47 edges
5. `Command` - 47 edges
6. `CoreAudioDevice` - 45 edges
7. `TempoMap` - 40 edges
8. `SimpleSynth` - 39 edges
9. `Transport` - 37 edges
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

## Communities (155 total, 17 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "AudioEngine"
Cohesion: 0.06
Nodes (59): CallbackProfiler, MidiInput, mutex, RetiredGraph, AudioEngine, active_, audioDeviceAboutToStart, audioDeviceStopped (+51 more)

### Community 2 - "NoteSequence"
Cohesion: 0.06
Nodes (48): Emit, NoteSequence, FrameCount, FramePosition, MidiBuffer, Tick, vector, size_t (+40 more)

### Community 3 - "PianoRollModel"
Cohesion: 0.07
Nodes (39): NoteList, size_t, Tick, vector, size_t, Tick, vector, PianoRollModel (+31 more)

### Community 4 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 5 - "NoteCommands.cpp"
Cohesion: 0.07
Nodes (38): NoteIndices, size_t, string, vector, DeleteNotesCommand, channel_, execute, indices_ (+30 more)

### Community 6 - "EntityId"
Cohesion: 0.07
Nodes (26): NoteIndices, Tick, MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, canMergeWith, channel_, indices_ (+18 more)

### Community 7 - "Project"
Cohesion: 0.11
Nodes (39): IdGenerator, EntityId, size_t, vector, operator==(), content, events, totalEventCount (+31 more)

### Community 8 - "DuplicatePatternCommand"
Cohesion: 0.06
Nodes (24): AddPatternCommand, index_, minted_, pattern_, DuplicatePatternCommand, execute, index_, minted_ (+16 more)

### Community 9 - "ChannelCommands.cpp"
Cohesion: 0.07
Nodes (27): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, execute, index_ (+19 more)

### Community 10 - "PlaylistModel"
Cohesion: 0.13
Nodes (29): EntityId, size_t, Tick, vector, EntityId, size_t, Tick, PlaylistModel (+21 more)

### Community 11 - "Transport"
Cohesion: 0.09
Nodes (24): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, Transport (+16 more)

### Community 12 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 13 - "CoreAudioDevice"
Cohesion: 0.08
Nodes (22): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputLatency_, name_ (+14 more)

### Community 14 - "vector"
Cohesion: 0.07
Nodes (14): vector, AddChannelCommand, channel_, execute, index_, minted_, undo, size_t (+6 more)

### Community 15 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, Rect, ChannelRackModel, contentHeight, hitTest, layout_, muteRect (+17 more)

### Community 16 - "Command"
Cohesion: 0.09
Nodes (24): Command, execute, id, name, undo, execute, undo, Tick (+16 more)

### Community 17 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 18 - "AddAutomationLaneCommand"
Cohesion: 0.11
Nodes (19): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, AutomationLane, Command (+11 more)

### Community 19 - "PatternChannelContent"
Cohesion: 0.08
Nodes (18): AutomationCurve, friend, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint (+10 more)

### Community 20 - "Channel"
Cohesion: 0.08
Nodes (28): PluginIdentifier, Channel, colour, id, instrument, instrumentStateFile, muted, name (+20 more)

### Community 21 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 22 - "CompiledProjectGraph"
Cohesion: 0.09
Nodes (27): Channel, CompiledProjectGraph, automation, channels, channelStripFor, channelStrips, error, graph (+19 more)

### Community 23 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 24 - "MixerView.mm"
Cohesion: 0.18
Nodes (25): NSMenu, -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect, -drawStripnode (+17 more)

### Community 25 - "CoreAudioDevice.cpp"
Cohesion: 0.26
Nodes (23): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+15 more)

### Community 26 - "Clip"
Cohesion: 0.08
Nodes (25): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+17 more)

### Community 27 - "MidiInput"
Cohesion: 0.11
Nodes (19): FrameCount, MidiBuffer, SampleRate, uint64_t, atomic, queueCapacity, size_t, uint64_t (+11 more)

### Community 28 - "AudioDeviceInfo"
Cohesion: 0.09
Nodes (20): AudioDeviceConfig, bufferSize, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate, AudioDeviceInfo, identifier (+12 more)

### Community 29 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 30 - "MidiMessage"
Cohesion: 0.11
Nodes (9): FrameCount, uint8_t, MidiMessage, data1, data2, frameOffset, status, vector (+1 more)

### Community 31 - "AudioBufferPool"
Cohesion: 0.13
Nodes (13): FramePosition, AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t (+5 more)

### Community 32 - "Json"
Cohesion: 0.10
Nodes (14): nullptr_t, pair, int64_t, string, vector, Json, asBool, boolean_ (+6 more)

### Community 33 - "AutomationNode"
Cohesion: 0.14
Nodes (12): Binding, Node, ProcessContext, vector, AutomationNode, bindings_, tempoMap_, size_t (+4 more)

### Community 34 - "load"
Cohesion: 0.23
Nodes (19): Result, append, automationPointFrom(), bindUnassignedContent(), string, idFrom(), midiEventFrom(), pluginFrom() (+11 more)

### Community 35 - "AudioBufferView"
Cohesion: 0.19
Nodes (9): renderAudioBlock, uint64_t, AudioBufferView, channels_, frames_, offset_, FrameCount, Sample (+1 more)

### Community 36 - "atomic"
Cohesion: 0.13
Nodes (5): atomic, MidiBuffer, array, Node, process

### Community 37 - "LevelMeter"
Cohesion: 0.13
Nodes (13): atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond (+5 more)

### Community 38 - "ProcessContext"
Cohesion: 0.10
Nodes (16): process, FrameCount, FramePosition, MidiBuffer, SampleRate, size_t, ProcessContext, frameCount (+8 more)

### Community 39 - "RecordedEvent"
Cohesion: 0.12
Nodes (20): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+12 more)

### Community 40 - "AudioDevice"
Cohesion: 0.10
Nodes (20): AudioDevice, actualBufferSize, actualOutputChannels, actualSampleRate, close, create, deviceName, enumerateDevices (+12 more)

### Community 41 - "PlaylistView.mm"
Cohesion: 0.17
Nodes (20): -acceptsFirstResponder, -addTrackRect, -drawBarLinesInLaneAtheight, -drawClips, -drawRect, -drawRuler, -drawTracks, -gridPointFor (+12 more)

### Community 42 - "ToggleStepCommand"
Cohesion: 0.12
Nodes (14): size_t, Tick, vector, size_t, Step, string, noteAtStep(), ToggleStepCommand (+6 more)

### Community 43 - "CompiledGraph"
Cohesion: 0.12
Nodes (14): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+6 more)

### Community 44 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 45 - "EntityId"
Cohesion: 0.12
Nodes (11): Command, EntityId, RenameTrackCommand, previousName_, trackId_, undo, SetTrackSoloedCommand, execute (+3 more)

### Community 46 - "DelayLineNode"
Cohesion: 0.13
Nodes (15): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+7 more)

### Community 47 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 48 - "MixerCommands.cpp"
Cohesion: 0.16
Nodes (14): execute, Command, SetMixerPanCommand, canMergeWith, execute, mergeWith, nodeId_, pan_ (+6 more)

### Community 49 - "InstrumentNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, unique_ptr, InstrumentNode, blockMidi_ (+6 more)

### Community 50 - "SystemInfo"
Cohesion: 0.14
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 51 - "PluginIdentifier"
Cohesion: 0.15
Nodes (12): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+4 more)

### Community 52 - "CountingCommand"
Cohesion: 0.12
Nodes (8): CountingCommand, counter_, delta_, string, Tick, makeProjectWithNotes(), NoOpCommand, note()

### Community 53 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 54 - "TimingProbeInstrument"
Cohesion: 0.14
Nodes (10): Applied, FrameCount, MidiBuffer, Sample, SampleRate, vector, renderSynth(), TimingProbeInstrument (+2 more)

### Community 55 - "AutomationPoint"
Cohesion: 0.15
Nodes (13): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+5 more)

### Community 56 - "GraphCompileOptions"
Cohesion: 0.12
Nodes (17): FrameCount, PlaybackSource, Sample, SampleRate, GraphCompileOptions, channelCount, instrumentFactory, masterGain (+9 more)

### Community 57 - "compile"
Cohesion: 0.17
Nodes (16): MidiBuffer, process, AudioBufferView, FrameCount, FramePosition, Node, NodeIndex, SampleRate (+8 more)

### Community 58 - "ClipCommands.cpp"
Cohesion: 0.18
Nodes (15): execute, undo, Command, EntityId, execute, undo, canMergeWith, execute (+7 more)

### Community 59 - "AddNoteCommand"
Cohesion: 0.12
Nodes (9): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, size_t (+1 more)

### Community 60 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 61 - "Smoother"
Cohesion: 0.18
Nodes (10): atomic, AudioBufferView, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds (+2 more)

### Community 62 - "MixerStripNode"
Cohesion: 0.12
Nodes (11): ProcessContext, atomic, Node, Sample, MixerStripNode, left_, meter_, muted_ (+3 more)

### Community 63 - "MixerTests.cpp"
Cohesion: 0.15
Nodes (12): TempoMap, AudioBufferPool, AudioBufferView, ProcessContext, Sample, size_t, vector, channel (+4 more)

### Community 64 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 65 - "AutomationCommands.cpp"
Cohesion: 0.21
Nodes (15): execute, undo, AutomationLane, Command, EntityId, Project, vector, findLane() (+7 more)

### Community 66 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 67 - "TimestampedMidiMessage"
Cohesion: 0.13
Nodes (15): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status (+7 more)

### Community 68 - "GraphBuilder"
Cohesion: 0.14
Nodes (12): Connection, GraphBuilder, compensate_, connections_, error_, master_, nodes_, Node (+4 more)

### Community 69 - "-applicationDidFinishLaunching"
Cohesion: 0.19
Nodes (15): INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, INCDAWPlaylistView, NSApplicationDelegate, NSObject, NSScrollView (+7 more)

### Community 70 - "TrackCommands.cpp"
Cohesion: 0.17
Nodes (13): execute, Command, execute, undo, execute, SetTrackHeightCommand, canMergeWith, execute (+5 more)

### Community 71 - "Json.cpp"
Cohesion: 0.22
Nodes (13): int64_t, size_t, string, escapeInto(), formatDouble(), asDouble, asInt, asString (+5 more)

### Community 72 - "Parser"
Cohesion: 0.30
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 73 - "main.mm"
Cohesion: 0.19
Nodes (14): -editorChanged, -seekToTick, -selectChannel, -selectPattern, -showEditorAtSegment, -showMixer, -showPianoRoll, -showPlaylist (+6 more)

### Community 74 - "Fixture"
Cohesion: 0.15
Nodes (13): TempoMap, EntityId, SequencedNote, Tick, vector, Fixture, channel, pattern (+5 more)

### Community 75 - "string"
Cohesion: 0.16
Nodes (6): string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 76 - "EntityId"
Cohesion: 0.16
Nodes (11): ConnectMixerCommand, connection_, destination_, gain_, index_, isSend_, minted_, preFader_ (+3 more)

### Community 77 - "SineOscillatorNode"
Cohesion: 0.16
Nodes (8): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, sampleRate_

### Community 78 - "renderClickFrames"
Cohesion: 0.15
Nodes (12): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, FramePosition (+4 more)

### Community 79 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 80 - "MidiEvent"
Cohesion: 0.15
Nodes (13): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+5 more)

### Community 81 - "AddPatternClipCommand"
Cohesion: 0.18
Nodes (10): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+2 more)

### Community 82 - "AddTrackCommand"
Cohesion: 0.18
Nodes (7): AddTrackCommand, index_, minted_, track_, undo, size_t, string

### Community 83 - "MixerNode"
Cohesion: 0.15
Nodes (13): MixerNodeType, MixerNode, colour, id, inserts, muted, name, pan (+5 more)

### Community 84 - "ParameterRegistry"
Cohesion: 0.22
Nodes (11): Entry, string, Entry, size_t, vector, ParameterRegistry, entries_, find (+3 more)

### Community 85 - "MidiRecorder"
Cohesion: 0.20
Nodes (10): CapturedMessage, atomic, queueCapacity, size_t, uint64_t, MidiRecorder, captured_, dropped_ (+2 more)

### Community 86 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 87 - "CoreMidiDevice"
Cohesion: 0.18
Nodes (10): MIDIClientRef, MIDIPortRef, CoreMidiDevice, callback_, client_, close, inputPort_, outputEndpoint_ (+2 more)

### Community 88 - "handlePackets"
Cohesion: 0.24
Nodes (9): MIDIPacketList, handlePackets, readProc, uint64_t, hostTimeNowNanos(), hostTimeToNanos(), Timebase, denominator (+1 more)

### Community 89 - "Instrument"
Cohesion: 0.18
Nodes (9): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+1 more)

### Community 90 - "RoutingConnection"
Cohesion: 0.17
Nodes (10): EntityId, findRouting, RoutingConnection, destination, gain, id, isSend, preFader (+2 more)

### Community 91 - "Denormals.h"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 92 - "RemoveMixerNodeCommand"
Cohesion: 0.18
Nodes (9): RemovedRouting, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_, routing_ (+1 more)

### Community 93 - "AddMixerNodeCommand"
Cohesion: 0.20
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 94 - "SetTrackMutedCommand"
Cohesion: 0.18
Nodes (6): SetTrackMutedCommand, execute, muted_, trackId_, undo, vector

### Community 95 - "MixerStripNode.cpp"
Cohesion: 0.31
Nodes (10): FrameCount, Sample, SampleRate, panGains, prepare, refreshTargets, setGain, setMuted (+2 more)

### Community 96 - "MixerFixture"
Cohesion: 0.18
Nodes (8): buildParallelPaths(), EntityId, TempoMap, unique_ptr, MixerFixture, pattern, project, tempo

### Community 97 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 98 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 99 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 100 - "Track"
Cohesion: 0.18
Nodes (11): Track, colour, height, id, muted, name, outputMixerNode, parent (+3 more)

### Community 101 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 102 - "ResizeClipsCommand"
Cohesion: 0.22
Nodes (7): Command, ResizeClipsCommand, clips_, execute, lengthDelta_, previousLengths_, undo

### Community 103 - "MixerCommands.h"
Cohesion: 0.22
Nodes (7): Command, SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 104 - "Pattern"
Cohesion: 0.20
Nodes (10): Pattern, automationLanes, channels, colour, id, length, name, swing (+2 more)

### Community 105 - "AutomationFixture"
Cohesion: 0.20
Nodes (8): AutomationFixture, channel, pattern, project, tempo, EntityId, Project, TempoMap

### Community 106 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 107 - "RemoveClipsCommand"
Cohesion: 0.25
Nodes (7): ClipIds, string, RemovedClip, RemoveClipsCommand, clips_, name, removed_

### Community 108 - "SetChannelStepKeyCommand"
Cohesion: 0.22
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 109 - "DuplicateClipsCommand"
Cohesion: 0.22
Nodes (7): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_

### Community 110 - "MoveClipsCommand"
Cohesion: 0.25
Nodes (7): Tick, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, tickDelta_, trackDelta_

### Community 111 - "SetClipMutedCommand"
Cohesion: 0.22
Nodes (7): vector, SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 112 - "DisconnectMixerCommand"
Cohesion: 0.22
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 113 - "SetMixerVolumeCommand"
Cohesion: 0.22
Nodes (6): SetMixerVolumeCommand, execute, nodeId_, previous_, undo, volume_

### Community 114 - "SetSendGainCommand"
Cohesion: 0.22
Nodes (6): SetSendGainCommand, connectionId_, execute, gain_, previous_, undo

### Community 115 - "MidiDeviceInfo"
Cohesion: 0.22
Nodes (7): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback, midiMessageReceived

### Community 116 - "SetMixerMutedCommand"
Cohesion: 0.25
Nodes (5): SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 117 - "SetMixerPolarityCommand"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 118 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 119 - "RemoveTrackCommand"
Cohesion: 0.25
Nodes (7): RemovedClip, vector, RemoveTrackCommand, clips_, index_, track_, trackId_

### Community 120 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 121 - "ioProcTrampoline"
Cohesion: 0.40
Nodes (6): AudioBufferList, AudioTimeStamp, OSStatus, ioProcTrampoline, renderInto, uint64_t

### Community 122 - "INCDAWMixerView"
Cohesion: 0.33
Nodes (5): incdaw, NSView, INCDAWMixerView, -initWithFrameprojectregistry, stripLookup

### Community 123 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 125 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 126 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 127 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 128 - "vector"
Cohesion: 0.50
Nodes (3): vector, Node, AudioBufferPool

### Community 129 - "MidiRecorder.cpp"
Cohesion: 0.40
Nodes (4): FramePosition, MidiBuffer, capture, reset

### Community 131 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 132 - "INCDAWChannelRackView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWChannelRackView, -initWithFrameprojectregistry

### Community 133 - "INCDAWPatternListView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWPatternListView, -initWithFrameprojectregistry

### Community 134 - "INCDAWPlaylistView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry

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
- **663 isolated node(s):** `amplitude`, `buffer`, `device`, `frequency`, `listOnly` (+658 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **17 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `NoteSequence`, `NoteCommands.cpp`, `EntityId`, `DuplicatePatternCommand`, `ChannelCommands.cpp`, `PlaylistModel`, `CommandRegistry`, `vector`, `Command`, `PatternChannelContent`, `Channel`, `Clip`, `load`, `ToggleStepCommand`, `EntityId`, `MixerCommands.cpp`, `CountingCommand`, `ClipCommands.cpp`, `AddNoteCommand`, `MixerTests.cpp`, `TrackCommands.cpp`, `Fixture`, `string`, `EntityId`, `AddTrackCommand`, `MixerNode`, `RoutingConnection`, `RemoveMixerNodeCommand`, `AddMixerNodeCommand`, `SetTrackMutedCommand`, `MixerFixture`, `AudioAsset`, `Track`, `ResizeClipsCommand`, `MixerCommands.h`, `Pattern`, `SetChannelStepKeyCommand`, `SetClipMutedCommand`, `DisconnectMixerCommand`, `SetMixerVolumeCommand`, `SetSendGainCommand`, `SetMixerMutedCommand`, `SetMixerPolarityCommand`, `SetMixerSoloedCommand`?**
  _High betweenness centrality (0.318) - this node is a cross-community bridge._