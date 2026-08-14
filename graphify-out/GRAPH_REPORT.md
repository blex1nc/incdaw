# Graph Report - project-continuation-670d11  (2026-08-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 2842 nodes · 4913 edges · 203 communities (169 shown, 34 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 239 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `e5883c09`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- PatternCommands.cpp
- RemoveTrackCommand
- PianoRollModel
- CoreAudioDevice
- SimpleSynth
- Project
- Transport
- CommandRegistry
- ChannelRackModel
- MixerNode
- TempoMap
- Clip
- MetronomeNode
- AutomationLane
- Json
- AudioDeviceConfig
- AudioEngine
- CoreMidiDevice
- MusicalPosition
- Json.cpp
- CoreAudioDevice.cpp
- MixerView.mm
- MidiEvent
- MidiMessage
- NoteSequence
- AudioRecorder
- string
- MidiInput
- ProjectFile.cpp
- CallbackProfiler
- AudioDevice
- NoteCommands.cpp
- AudioBufferPool
- GraphBuilder
- InsertRecordedTakeCommand
- PlaylistView.mm
- atomic
- DelayLineNode
- CompiledGraph
- CountingCommand
- RealtimeGuard.cpp
- compileArrangement
- Pattern
- PlaylistModel
- AudioEngine.cpp
- AudioBufferView
- ProcessContext
- BasicMidiBuffer
- SystemInfo
- RecordingSession
- RemoveClipsCommand
- MixerCommands.cpp
- LevelMeter
- compile
- InstrumentNode
- PluginIdentifier
- ConstantNode
- main
- MidiRecorder
- GraphCompileOptions
- ClipCommands.cpp
- MoveNotesCommand
- PlaylistModel.cpp
- LockFreeQueue
- SampleRingBuffer
- Smoother
- GainNode
- MixerStripNode
- LoopbackResult
- LatentProcessorNode
- ioProcTrampoline
- WavStreamWriter
- ResizeNotesCommand
- vector
- WavBytes.h
- SineOscillatorNode
- EntityId
- RoutingConnection
- MixerTests.cpp
- TimingProbeInstrument
- -applicationDidFinishLaunching
- AudioClipNode
- string
- ChannelCommands.cpp
- append
- CompiledProjectGraph
- main.mm
- AddPatternClipCommand
- EntityId
- SetVelocityCommand
- ParameterRegistry
- ChannelRackView.mm
- Fixture
- Options
- renderNode
- AutomationPoint
- AutomationNode
- EntityId
- string
- ToggleStepCommand
- read
- Channel
- RemoveMixerNodeCommand
- AutomationCommands.cpp
- AddMixerNodeCommand
- Track
- Instrument
- compileProjectGraph
- WavStreamWriterTests.cpp
- Denormals.h
- SetAutomationPointsCommand
- AddNoteCommand
- captureAudioBlock
- AudioFileData
- TimelineAnchor
- MixerStripNode.cpp
- TimeSignatureEvent
- BlockSegment
- MidiDevice
- AudioAsset
- PatternListView.mm
- DuplicateClipsCommand
- DisconnectMixerCommand
- DeleteNotesCommand
- ParsedHeader
- RecordedEvent
- MidiTests.cpp
- make-dmg.sh
- RemoveChannelCommand
- Command
- SetChannelStepKeyCommand
- MoveClipsCommand
- SetChannelOutputCommand
- SetMixerPanCommand
- SetMixerVolumeCommand
- MidiDevice.h
- AutomationFixture
- ResizeClipsCommand
- MixerCommands.h
- SetMixerPolarityCommand
- SetMixerSoloedCommand
- Version
- TimestampedMidiMessage
- makeTestSignal
- InstrumentTests.cpp
- SetChannelSoloedCommand
- MixerFixture
- PatternTests.cpp
- ScratchDirectory
- ProjectGraphCompiler.h
- INCDAWMixerView
- INCDAWPianoRollView
- RenameChannelCommand
- string
- Node
- collectForRange
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- ChannelRackTests.cpp
- OrderRecordingNode
- check
- start
- AudioCaptureSink
- INCDAWChannelRackView
- INCDAWPatternListView
- INCDAWPlaylistView
- clipRect
- snapTick
- CallbackProfiler
- Channel
- FrameCount
- FramePosition
- Node
- NSMenu
- Result
- Sample
- AutomationLane
- Project
- AutomationLane
- Rect
- T
- Kind
- friend
- MidiEventType
- MixerNodeType
- PluginIdentifier
- uint32_t
- Project
- SampleRate
- NSView
- Step
- TempoMap
- Project
- Tick
- uint64_t
- uint8_t
- Viewport

## God Nodes (most connected - your core abstractions)
1. `Project` - 174 edges
2. `AudioEngine` - 60 edges
3. `CoreAudioDevice` - 59 edges
4. `Json` - 44 edges
5. `Clip` - 40 edges
6. `TempoMap` - 38 edges
7. `Transport` - 37 edges
8. `MidiEvent` - 36 edges
9. `SimpleSynth` - 36 edges
10. `PianoRollModel` - 34 edges

## Surprising Connections (you probably didn't know these)
- `Audio Engine` --semantically_similar_to--> `Audio Engine Priority`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Controller Linking` --semantically_similar_to--> `Parameter System`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Clip / Project Data Model` --semantically_similar_to--> `Core Data Model`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Development Phases (0-20)` --semantically_similar_to--> `Feature Roadmap (Phase 0-20)`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Required Documentation Set` --semantically_similar_to--> `Handoff Rule`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Approval-Gated Development Protocol** — claude_absolute_user_control_rule, claude_graphify_mandate, claude_feature_workflow, claude_scope_control, claude_dependency_policy, handoff_critical_operating_rule, handoff_feature_protocol, handoff_handoff_rule [EXTRACTED 1.00]
- **Plugin Host Pipeline** — handoff_plugin_scanner, handoff_plugin_registry, handoff_plugin_instance, handoff_parameter_system, handoff_plugin_state_system, handoff_plugin_ui_bridge, handoff_crash_isolation_strategy [EXTRACTED 1.00]
- **Master Signal Chain Convergence** — handoff_midi_signal_flow, handoff_audio_signal_flow, handoff_plugin_automation_flow, handoff_shared_transport_state, claude_mixer, claude_automation, claude_offline_render_engine, claude_core_transport [INFERRED 0.85]

## Communities (203 total, 34 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "PatternCommands.cpp"
Cohesion: 0.05
Nodes (46): AddPatternCommand, execute, index_, minted_, pattern_, undo, Command, DuplicatePatternCommand (+38 more)

### Community 2 - "RemoveTrackCommand"
Cohesion: 0.05
Nodes (43): AddTrackCommand, execute, index_, minted_, track_, undo, Command, Command (+35 more)

### Community 3 - "PianoRollModel"
Cohesion: 0.08
Nodes (34): NoteList, size_t, Tick, vector, size_t, Tick, Viewport, PianoRollModel (+26 more)

### Community 4 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 5 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 6 - "Project"
Cohesion: 0.12
Nodes (37): IdGenerator, undo, EntityId, size_t, vector, operator==(), events, totalEventCount (+29 more)

### Community 7 - "Transport"
Cohesion: 0.09
Nodes (24): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, Transport (+16 more)

### Community 8 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 9 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 10 - "MixerNode"
Cohesion: 0.08
Nodes (28): MixerNodeType, PluginIdentifier, string, MixerNode, colour, id, inserts, muted (+20 more)

### Community 11 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 12 - "Clip"
Cohesion: 0.07
Nodes (29): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+21 more)

### Community 13 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 14 - "AutomationLane"
Cohesion: 0.08
Nodes (18): AutomationCurve, friend, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint (+10 more)

### Community 15 - "Json"
Cohesion: 0.08
Nodes (18): nullptr_t, pair, int64_t, int64_t, string, vector, Json, append (+10 more)

### Community 16 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 17 - "AudioEngine"
Cohesion: 0.10
Nodes (24): atomic, AudioDevice, AudioIOCallback, MidiBuffer, MidiInput, mutex, RetiredGraph, AudioCaptureSink (+16 more)

### Community 18 - "CoreMidiDevice"
Cohesion: 0.14
Nodes (24): CFStringRef, MIDIClientRef, MIDIEndpointRef, MIDIObjectRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_ (+16 more)

### Community 19 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 20 - "Json.cpp"
Cohesion: 0.19
Nodes (22): size_t, string, escapeInto(), formatDouble(), asString, contains, dump, dumpTo (+14 more)

### Community 21 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 22 - "MixerView.mm"
Cohesion: 0.18
Nodes (25): -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect, -drawStripnode, -faderRectAt (+17 more)

### Community 23 - "MidiEvent"
Cohesion: 0.11
Nodes (24): Kind, MidiEventType, RecordedEvent, appendRecordedEvents(), MidiEventType, Tick, uint64_t, vector (+16 more)

### Community 24 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 25 - "NoteSequence"
Cohesion: 0.11
Nodes (21): Tick, vector, size_t, Tick, uint32_t, vector, NoteSequence, byEnd_ (+13 more)

### Community 26 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

### Community 27 - "string"
Cohesion: 0.12
Nodes (14): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, Command, EntityId (+6 more)

### Community 28 - "MidiInput"
Cohesion: 0.11
Nodes (19): FrameCount, MidiBuffer, SampleRate, uint64_t, atomic, queueCapacity, size_t, uint64_t (+11 more)

### Community 29 - "ProjectFile.cpp"
Cohesion: 0.23
Nodes (21): Json, automationPointFrom(), bindUnassignedContent(), EntityId, path, PluginIdentifier, Result, string (+13 more)

### Community 30 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 31 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 32 - "NoteCommands.cpp"
Cohesion: 0.16
Nodes (21): Command, EntityId, NoteIndices, size_t, vector, execute, findEvents(), canMergeWith (+13 more)

### Community 33 - "AudioBufferPool"
Cohesion: 0.13
Nodes (13): AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t, FrameCount (+5 more)

### Community 34 - "GraphBuilder"
Cohesion: 0.11
Nodes (16): Connection, NodeIndex, GraphBuilder, compensate_, connect, connections_, error_, master_ (+8 more)

### Community 35 - "InsertRecordedTakeCommand"
Cohesion: 0.11
Nodes (16): Command, EntityId, Placement, size_t, InsertRecordedTakeCommand, asset_, assetIndex_, clip_ (+8 more)

### Community 36 - "PlaylistView.mm"
Cohesion: 0.17
Nodes (20): -acceptsFirstResponder, -addTrackRect, -drawBarLinesInLaneAtheight, -drawClips, -drawRect, -drawRuler, -drawTracks, -gridPointFor (+12 more)

### Community 37 - "atomic"
Cohesion: 0.15
Nodes (4): vector, atomic, MidiBuffer, array

### Community 38 - "DelayLineNode"
Cohesion: 0.12
Nodes (16): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+8 more)

### Community 39 - "CompiledGraph"
Cohesion: 0.12
Nodes (14): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+6 more)

### Community 40 - "CountingCommand"
Cohesion: 0.12
Nodes (10): CountingCommand, counter_, delta_, Command, EntityId, string, Tick, makeProjectWithNotes() (+2 more)

### Community 41 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 42 - "compileArrangement"
Cohesion: 0.23
Nodes (19): Emit, NoteSequence, content, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto() (+11 more)

### Community 43 - "Pattern"
Cohesion: 0.13
Nodes (15): size_t, Tick, vector, noteAtStep(), execute, Pattern, automationLanes, channels (+7 more)

### Community 44 - "PlaylistModel"
Cohesion: 0.15
Nodes (11): EntityId, size_t, Tick, vector, Viewport, PlaylistModel, noClip, noTrack (+3 more)

### Community 45 - "AudioEngine.cpp"
Cohesion: 0.18
Nodes (17): audioDeviceAboutToStart, audioDeviceStopped, bufferSize, captureAudioBlock, collectRetiredGraphs, inputChannels, isRunning, maxServiceableBlockSize (+9 more)

### Community 46 - "AudioBufferView"
Cohesion: 0.20
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 47 - "ProcessContext"
Cohesion: 0.12
Nodes (15): FrameCount, FramePosition, MidiBuffer, SampleRate, size_t, ProcessContext, frameCount, inputCount (+7 more)

### Community 48 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 49 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 50 - "RecordingSession"
Cohesion: 0.14
Nodes (12): AudioRecorder, string, Placement, string, FrameCount, uint64_t, RecordingSession, arm (+4 more)

### Community 51 - "RemoveClipsCommand"
Cohesion: 0.14
Nodes (14): ClipIds, string, Command, RemovedClip, vector, RemoveClipsCommand, clips_, name (+6 more)

### Community 52 - "MixerCommands.cpp"
Cohesion: 0.16
Nodes (14): undo, Command, canMergeWith, mergeWith, canMergeWith, mergeWith, SetSendGainCommand, canMergeWith (+6 more)

### Community 53 - "LevelMeter"
Cohesion: 0.14
Nodes (13): atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond (+5 more)

### Community 54 - "compile"
Cohesion: 0.16
Nodes (17): process, AudioBufferView, FrameCount, FramePosition, MidiBuffer, Node, SampleRate, size_t (+9 more)

### Community 55 - "InstrumentNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, unique_ptr, InstrumentNode, blockMidi_ (+6 more)

### Community 56 - "PluginIdentifier"
Cohesion: 0.14
Nodes (13): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+5 more)

### Community 57 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 58 - "main"
Cohesion: 0.15
Nodes (16): AudioDeviceInfo, availableDevices, midiInput_, profiler_, sampleRate, setGraph, transport_, CompiledGraph (+8 more)

### Community 59 - "MidiRecorder"
Cohesion: 0.14
Nodes (14): CapturedMessage, FramePosition, MidiBuffer, atomic, queueCapacity, size_t, uint64_t, MidiRecorder (+6 more)

### Community 60 - "GraphCompileOptions"
Cohesion: 0.12
Nodes (17): PlaybackSource, SampleRate, GraphCompileOptions, channelCount, instrumentFactory, masterGain, maxBlockSize, parameters (+9 more)

### Community 61 - "ClipCommands.cpp"
Cohesion: 0.18
Nodes (15): undo, Command, EntityId, execute, undo, canMergeWith, execute, mergeWith (+7 more)

### Community 62 - "MoveNotesCommand"
Cohesion: 0.18
Nodes (11): EntityId, NoteIndices, Tick, MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, channel_, indices_ (+3 more)

### Community 63 - "PlaylistModel.cpp"
Cohesion: 0.21
Nodes (16): EntityId, size_t, vector, addToSelection, clipAtPoint, clipsInRectangle, collectVisibleClips, isOverResizeHandle (+8 more)

### Community 64 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 65 - "SampleRingBuffer"
Cohesion: 0.21
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 66 - "Smoother"
Cohesion: 0.18
Nodes (10): atomic, AudioBufferView, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds (+2 more)

### Community 67 - "GainNode"
Cohesion: 0.15
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 68 - "MixerStripNode"
Cohesion: 0.12
Nodes (11): ProcessContext, atomic, Node, Sample, MixerStripNode, left_, meter_, muted_ (+3 more)

### Community 69 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 70 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 71 - "ioProcTrampoline"
Cohesion: 0.23
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 72 - "WavStreamWriter"
Cohesion: 0.13
Nodes (14): ofstream, Format, FrameCount, path, size_t, uint8_t, vector, WavStreamWriter (+6 more)

### Community 73 - "ResizeNotesCommand"
Cohesion: 0.13
Nodes (13): Command, QuantizeNotesCommand, channel_, grid_, pattern_, previousEvents_, strength_, ResizeNotesCommand (+5 more)

### Community 74 - "vector"
Cohesion: 0.18
Nodes (7): vector, Node, AudioBufferPool, AutomationPoint, Tick, enginePoint(), modelPoint()

### Community 75 - "WavBytes.h"
Cohesion: 0.30
Nodes (15): appendCanonicalHeader(), bitsFor(), codeFor(), encodeSample(), Format, Sample, size_t, uint16_t (+7 more)

### Community 76 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 77 - "EntityId"
Cohesion: 0.21
Nodes (7): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, Value

### Community 78 - "RoutingConnection"
Cohesion: 0.12
Nodes (14): EntityId, findRouting, ids_, metadata_, tempoMap_, RoutingConnection, destination, gain (+6 more)

### Community 79 - "MixerTests.cpp"
Cohesion: 0.17
Nodes (11): AudioBufferPool, AudioBufferView, ProcessContext, Sample, size_t, vector, channel, onsets() (+3 more)

### Community 80 - "TimingProbeInstrument"
Cohesion: 0.15
Nodes (9): Applied, AudioBufferView, FrameCount, MidiMessage, SampleRate, vector, TimingProbeInstrument, applied (+1 more)

### Community 81 - "-applicationDidFinishLaunching"
Cohesion: 0.19
Nodes (15): INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, INCDAWPlaylistView, NSApplicationDelegate, NSObject, NSScrollView (+7 more)

### Community 82 - "AudioClipNode"
Cohesion: 0.15
Nodes (11): ProcessContext, AudioClipNode, addClip, clips_, process, PlacedClip, Node, PlacedClip (+3 more)

### Community 83 - "string"
Cohesion: 0.15
Nodes (8): AddChannelCommand, channel_, execute, index_, minted_, undo, size_t, string

### Community 84 - "ChannelCommands.cpp"
Cohesion: 0.17
Nodes (11): Command, execute, execute, SetChannelVolumeCommand, canMergeWith, channelId_, execute, mergeWith (+3 more)

### Community 85 - "append"
Cohesion: 0.28
Nodes (12): Format, FrameCount, path, Result, Sample, SampleRate, size_t, append (+4 more)

### Community 86 - "CompiledProjectGraph"
Cohesion: 0.13
Nodes (15): CompiledProjectGraph, automation, channels, channelStrips, error, graph, instruments, mixerNodes (+7 more)

### Community 87 - "main.mm"
Cohesion: 0.19
Nodes (14): -editorChanged, -seekToTick, -selectChannel, -selectPattern, -showEditorAtSegment, -showMixer, -showPianoRoll, -showPlaylist (+6 more)

### Community 88 - "AddPatternClipCommand"
Cohesion: 0.16
Nodes (11): AddPatternClipCommand, clip_, execute, index_, length_, minted_, pattern_, start_ (+3 more)

### Community 89 - "EntityId"
Cohesion: 0.16
Nodes (11): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+3 more)

### Community 90 - "SetVelocityCommand"
Cohesion: 0.14
Nodes (8): string, SetVelocityCommand, channel_, indices_, pattern_, previousVelocities_, undo, velocity_

### Community 91 - "ParameterRegistry"
Cohesion: 0.20
Nodes (11): Entry, string, Entry, size_t, vector, ParameterRegistry, entries_, find (+3 more)

### Community 92 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 93 - "Fixture"
Cohesion: 0.16
Nodes (12): EntityId, SequencedNote, Tick, vector, Fixture, channel, pattern, project (+4 more)

### Community 94 - "Options"
Cohesion: 0.14
Nodes (14): int64_t, Options, amplitude, buffer, device, frequency, input, listOnly (+6 more)

### Community 95 - "renderNode"
Cohesion: 0.21
Nodes (11): AudioFileData, shared_ptr, FrameCount, Node, Sample, size_t, vector, makeAudio() (+3 more)

### Community 96 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 97 - "AutomationNode"
Cohesion: 0.18
Nodes (8): Binding, AutomationNode, bindings_, tempoMap_, ProcessContext, size_t, TempoMap, vector

### Community 98 - "EntityId"
Cohesion: 0.17
Nodes (6): EntityId, SetChannelMutedCommand, channelId_, execute, muted_, undo

### Community 99 - "string"
Cohesion: 0.18
Nodes (7): Command, string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 100 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (9): Command, size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_ (+1 more)

### Community 101 - "read"
Cohesion: 0.36
Nodes (12): Format, path, Result, uint8_t, vector, fillMetadata(), loadAndParse(), parseHeader() (+4 more)

### Community 102 - "Channel"
Cohesion: 0.15
Nodes (13): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+5 more)

### Community 103 - "RemoveMixerNodeCommand"
Cohesion: 0.17
Nodes (10): RemovedRouting, vector, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_ (+2 more)

### Community 104 - "AutomationCommands.cpp"
Cohesion: 0.21
Nodes (11): execute, undo, AutomationPoint, EntityId, vector, findLane(), execute, undo (+3 more)

### Community 105 - "AddMixerNodeCommand"
Cohesion: 0.18
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 106 - "Track"
Cohesion: 0.17
Nodes (12): trackHeight, Track, colour, height, id, muted, name, outputMixerNode (+4 more)

### Community 107 - "Instrument"
Cohesion: 0.18
Nodes (9): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+1 more)

### Community 108 - "compileProjectGraph"
Cohesion: 0.24
Nodes (11): channelStripFor, instrumentFor, stripFor, compileProjectGraph(), EntityId, InstrumentFactory, InstrumentNode, MixerStripNode (+3 more)

### Community 109 - "WavStreamWriterTests.cpp"
Cohesion: 0.18
Nodes (10): FrameCount, path, size_t, uint8_t, vector, fileBytes(), makeTestSignal(), ScratchFile (+2 more)

### Community 110 - "Denormals.h"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 111 - "SetAutomationPointsCommand"
Cohesion: 0.24
Nodes (9): Command, AutomationPoint, vector, SetAutomationPointsCommand, canMergeWith, laneId_, mergeWith, points_ (+1 more)

### Community 112 - "AddNoteCommand"
Cohesion: 0.20
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, size_t

### Community 113 - "captureAudioBlock"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 114 - "AudioFileData"
Cohesion: 0.18
Nodes (10): AudioFileData, channelCount, channels, frameCount, sampleRate, FrameCount, Sample, SampleRate (+2 more)

### Community 115 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 116 - "MixerStripNode.cpp"
Cohesion: 0.31
Nodes (10): FrameCount, Sample, SampleRate, panGains, prepare, refreshTargets, setGain, setMuted (+2 more)

### Community 117 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 118 - "BlockSegment"
Cohesion: 0.18
Nodes (9): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, size_t (+1 more)

### Community 119 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 120 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 121 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 122 - "DuplicateClipsCommand"
Cohesion: 0.22
Nodes (8): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_, Tick

### Community 123 - "DisconnectMixerCommand"
Cohesion: 0.20
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 124 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (9): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo (+1 more)

### Community 125 - "ParsedHeader"
Cohesion: 0.20
Nodes (10): size_t, uint16_t, uint32_t, ParsedHeader, bitsPerSample, channels, dataOffset, dataSize (+2 more)

### Community 126 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 127 - "MidiTests.cpp"
Cohesion: 0.22
Nodes (9): FrameCount, MidiMessage, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped() (+1 more)

### Community 128 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 129 - "RemoveChannelCommand"
Cohesion: 0.22
Nodes (8): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, index_, undo

### Community 130 - "Command"
Cohesion: 0.22
Nodes (5): Command, execute, id, name, undo

### Community 131 - "SetChannelStepKeyCommand"
Cohesion: 0.22
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 132 - "MoveClipsCommand"
Cohesion: 0.22
Nodes (6): MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, tickDelta_, trackDelta_

### Community 133 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 134 - "SetMixerPanCommand"
Cohesion: 0.22
Nodes (6): SetMixerPanCommand, execute, nodeId_, pan_, previous_, undo

### Community 135 - "SetMixerVolumeCommand"
Cohesion: 0.22
Nodes (6): SetMixerVolumeCommand, execute, nodeId_, previous_, undo, volume_

### Community 136 - "MidiDevice.h"
Cohesion: 0.22
Nodes (7): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback, midiMessageReceived

### Community 137 - "AutomationFixture"
Cohesion: 0.22
Nodes (7): AutomationFixture, channel, pattern, project, tempo, EntityId, TempoMap

### Community 138 - "ResizeClipsCommand"
Cohesion: 0.25
Nodes (6): ResizeClipsCommand, clips_, execute, lengthDelta_, previousLengths_, undo

### Community 139 - "MixerCommands.h"
Cohesion: 0.25
Nodes (5): SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 140 - "SetMixerPolarityCommand"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 141 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 142 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 143 - "TimestampedMidiMessage"
Cohesion: 0.25
Nodes (8): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status

### Community 144 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 145 - "InstrumentTests.cpp"
Cohesion: 0.33
Nodes (6): SimpleSynth, InstrumentNode, AudioBufferPool, MidiBuffer, Sample, renderSynth()

### Community 146 - "SetChannelSoloedCommand"
Cohesion: 0.29
Nodes (6): Command, SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 147 - "MixerFixture"
Cohesion: 0.29
Nodes (6): EntityId, TempoMap, MixerFixture, pattern, project, tempo

### Community 148 - "PatternTests.cpp"
Cohesion: 0.48
Nodes (6): SequencedNote, Tick, vector, note(), shapeOf(), startsOf()

### Community 149 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 150 - "ProjectGraphCompiler.h"
Cohesion: 0.33
Nodes (5): AutomationNode, Instrument, ParameterRegistry, TempoMap, MixerStripNode

### Community 151 - "INCDAWMixerView"
Cohesion: 0.33
Nodes (5): incdaw, NSView, INCDAWMixerView, -initWithFrameprojectregistry, stripLookup

### Community 152 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 153 - "RenameChannelCommand"
Cohesion: 0.33
Nodes (4): RenameChannelCommand, channelId_, previousName_, undo

### Community 156 - "collectForRange"
Cohesion: 0.33
Nodes (4): FrameCount, FramePosition, MidiBuffer, collectForRange

### Community 157 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 158 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 159 - "ChannelRackTests.cpp"
Cohesion: 0.33
Nodes (5): EntityId, Step, Tick, note(), stepAt()

### Community 160 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 161 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 162 - "start"
Cohesion: 0.50
Nodes (4): AudioDeviceConfig, deviceName, start, string

### Community 164 - "INCDAWChannelRackView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWChannelRackView, -initWithFrameprojectregistry

### Community 165 - "INCDAWPatternListView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWPatternListView, -initWithFrameprojectregistry

### Community 166 - "INCDAWPlaylistView"
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
- **737 isolated node(s):** `index_`, `minted_`, `pattern_`, `index_`, `minted_` (+732 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **34 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `RemoveChannelCommand`, `PatternCommands.cpp`, `SetChannelStepKeyCommand`, `RemoveTrackCommand`, `SetChannelOutputCommand`, `SetMixerPanCommand`, `SetMixerVolumeCommand`, `CommandRegistry`, `AutomationFixture`, `ResizeClipsCommand`, `MixerCommands.h`, `SetMixerPolarityCommand`, `SetMixerSoloedCommand`, `MixerNode`, `AutomationLane`, `Clip`, `SetChannelSoloedCommand`, `MixerFixture`, `ProjectGraphCompiler.h`, `RenameChannelCommand`, `ProjectFile.cpp`, `NoteCommands.cpp`, `InsertRecordedTakeCommand`, `clipRect`, `CountingCommand`, `compileArrangement`, `Pattern`, `RemoveClipsCommand`, `MixerCommands.cpp`, `ClipCommands.cpp`, `PlaylistModel.cpp`, `RoutingConnection`, `string`, `ChannelCommands.cpp`, `AddPatternClipCommand`, `EntityId`, `SetVelocityCommand`, `Fixture`, `EntityId`, `string`, `Channel`, `RemoveMixerNodeCommand`, `AutomationCommands.cpp`, `AddMixerNodeCommand`, `Track`, `compileProjectGraph`, `AddNoteCommand`, `AudioAsset`, `DisconnectMixerCommand`, `DeleteNotesCommand`?**
  _High betweenness centrality (0.333) - this node is a cross-community bridge._