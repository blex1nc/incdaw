# Graph Report - project-continuation-670d11  (2026-08-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 2753 nodes · 4803 edges · 195 communities (150 shown, 45 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 250 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `fdbd6cf0`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- PatternCommands.cpp
- RemoveTrackCommand
- PlaylistModel
- main.mm
- PianoRollModel
- AudioEngine
- CoreAudioDevice
- SimpleSynth
- Channel
- Project
- Transport
- CommandRegistry
- ChannelRackModel
- NoteSequence
- vector
- TempoMap
- MetronomeNode
- Json
- MixerStripNode
- AudioDeviceConfig
- MusicalPosition
- Json.cpp
- CoreAudioDevice.cpp
- MidiMessage
- MixerView.mm
- Clip
- AudioRecorder
- ResizeNotesCommand
- Pattern
- CompiledProjectGraph
- ProjectFile.cpp
- CallbackProfiler
- AudioDevice
- MixerTests.cpp
- string
- AudioBufferPool
- TimingProbeInstrument
- NoteCommands.cpp
- AudioBufferView
- PlaylistView.mm
- compileArrangement
- DelayLineNode
- CompiledGraph
- Options
- CountingCommand
- RealtimeGuard.cpp
- GraphBuilder
- AudioEngine.h
- MidiInput
- SystemInfo
- MixerCommands.cpp
- LevelMeter
- InstrumentNode
- BasicMidiBuffer
- PluginIdentifier
- ConstantNode
- friend
- MidiRecorder
- GraphCompileOptions
- LockFreeQueue
- SampleRingBuffer
- Smoother
- LoopbackResult
- LatentProcessorNode
- ioProcTrampoline
- CoreMidiDevice
- WavStreamWriter
- WavBytes.h
- GainNode
- SineOscillatorNode
- EntityId
- RoutingConnection
- AddPatternClipCommand
- append
- ProcessContext
- compile
- DuplicateClipsCommand
- EntityId
- EntityId
- SetVelocityCommand
- ToggleStepCommand
- ChannelRackView.mm
- Fixture
- AutomationPoint
- ChannelCommands.cpp
- ClipCommands.cpp
- MoveClipsCommand
- string
- read
- MidiEvent
- ParameterRegistry
- CoreMidiDevice.cpp
- humanizeNoteStarts
- RemoveMixerNodeCommand
- string
- AutomationCommands.cpp
- SetClipMutedCommand
- AddMixerNodeCommand
- atomic
- Instrument
- MidiDevice
- WavStreamWriterTests.cpp
- Denormals.h
- SetAutomationPointsCommand
- RemoveClipsCommand
- AddNoteCommand
- start
- AudioFileData
- RecordedEvent
- TimeSignatureEvent
- BlockSegment
- AudioAsset
- PatternListView.mm
- DisconnectMixerCommand
- DeleteNotesCommand
- ParsedHeader
- Node
- make-dmg.sh
- RemoveChannelCommand
- AddChannelCommand
- SetChannelOutputCommand
- SetMixerPanCommand
- SetMixerVolumeCommand
- MoveNotesCommand
- AutomationFixture
- Command
- SetChannelStepKeyCommand
- RenameChannelCommand
- MixerCommands.h
- SetMixerPolarityCommand
- SetMixerSoloedCommand
- Version
- TimestampedMidiMessage
- MidiDevice.h
- ProjectMetadata
- makeTestSignal
- SetChannelMutedCommand
- PatternTests.cpp
- ScratchDirectory
- string
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- ChannelRackTests.cpp
- OrderRecordingNode
- process
- collectForBlock
- check
- AudioCaptureSink
- CallbackProfiler
- Channel
- FrameCount
- FramePosition
- INCDAWChannelRackView
- INCDAWMixerView
- INCDAWPatternListView
- INCDAWPianoRollView
- INCDAWPlaylistView
- MidiBuffer
- MidiEventType
- MixerNodeType
- Node
- NSMenu
- NSView
- PluginIdentifier
- ProcessContext
- Rect
- Result
- Sample
- SampleRate
- AutomationLane
- Project
- AutomationLane
- CompiledGraph
- CompiledGraph
- T
- Kind
- friend
- MixerStripNode
- Project
- CompiledGraph
- MixerStripNode
- Step
- TempoMap
- Project
- Tick
- uint32_t
- uint64_t
- uint8_t
- Viewport

## God Nodes (most connected - your core abstractions)
1. `Project` - 170 edges
2. `CoreAudioDevice` - 59 edges
3. `AudioEngine` - 56 edges
4. `Json` - 44 edges
5. `TempoMap` - 38 edges
6. `Transport` - 37 edges
7. `Clip` - 37 edges
8. `SimpleSynth` - 36 edges
9. `MidiEvent` - 36 edges
10. `MixerStripNode` - 35 edges

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

## Communities (195 total, 45 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "PatternCommands.cpp"
Cohesion: 0.05
Nodes (46): AddPatternCommand, execute, index_, minted_, pattern_, undo, Command, DuplicatePatternCommand (+38 more)

### Community 2 - "RemoveTrackCommand"
Cohesion: 0.05
Nodes (43): AddTrackCommand, execute, index_, minted_, track_, undo, Command, Command (+35 more)

### Community 3 - "PlaylistModel"
Cohesion: 0.07
Nodes (43): EntityId, Rect, size_t, Tick, vector, EntityId, size_t, Tick (+35 more)

### Community 4 - "main.mm"
Cohesion: 0.06
Nodes (43): incdaw, NSApplicationDelegate, NSObject, NSScrollView, NSSegmentedControl, NSSplitView, NSString, NSTextField (+35 more)

### Community 5 - "PianoRollModel"
Cohesion: 0.08
Nodes (34): NoteList, size_t, Tick, vector, size_t, Tick, Viewport, PianoRollModel (+26 more)

### Community 6 - "AudioEngine"
Cohesion: 0.09
Nodes (42): RetiredGraph, AudioEngine, active_, audioDeviceAboutToStart, audioDeviceStopped, availableDevices, blockCounter_, blockMidi_ (+34 more)

### Community 7 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 8 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 9 - "Channel"
Cohesion: 0.06
Nodes (37): AutomationLane, id, parameterKey, points, targetEntity, Channel, colour, id (+29 more)

### Community 10 - "Project"
Cohesion: 0.12
Nodes (36): IdGenerator, EntityId, size_t, vector, TempoMap, operator==(), totalEventCount, Project (+28 more)

### Community 11 - "Transport"
Cohesion: 0.09
Nodes (24): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, Transport (+16 more)

### Community 12 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 13 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 14 - "NoteSequence"
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, MidiBuffer, Tick, vector, size_t, Tick, uint32_t (+17 more)

### Community 15 - "vector"
Cohesion: 0.10
Nodes (16): Binding, Instrument, vector, AutomationNode, bindings_, tempoMap_, Node, ProcessContext (+8 more)

### Community 16 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 17 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 18 - "Json"
Cohesion: 0.08
Nodes (18): nullptr_t, pair, int64_t, int64_t, string, vector, Json, append (+10 more)

### Community 19 - "MixerStripNode"
Cohesion: 0.11
Nodes (21): FrameCount, ProcessContext, Sample, SampleRate, atomic, Node, Sample, MixerStripNode (+13 more)

### Community 20 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 21 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 22 - "Json.cpp"
Cohesion: 0.19
Nodes (22): size_t, string, escapeInto(), formatDouble(), asString, contains, dump, dumpTo (+14 more)

### Community 23 - "CoreAudioDevice.cpp"
Cohesion: 0.29
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 24 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 25 - "MixerView.mm"
Cohesion: 0.18
Nodes (25): -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect, -drawStripnode, -faderRectAt (+17 more)

### Community 26 - "Clip"
Cohesion: 0.08
Nodes (25): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+17 more)

### Community 27 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

### Community 28 - "ResizeNotesCommand"
Cohesion: 0.12
Nodes (17): EntityId, NoteIndices, Tick, vector, QuantizeNotesCommand, channel_, execute, grid_ (+9 more)

### Community 29 - "Pattern"
Cohesion: 0.11
Nodes (22): size_t, Tick, vector, noteAtStep(), execute, undo, vector, Pattern (+14 more)

### Community 30 - "CompiledProjectGraph"
Cohesion: 0.11
Nodes (23): CompiledProjectGraph, automation, channels, channelStripFor, channelStrips, error, graph, instrumentFor (+15 more)

### Community 31 - "ProjectFile.cpp"
Cohesion: 0.23
Nodes (21): Json, automationPointFrom(), bindUnassignedContent(), EntityId, path, PluginIdentifier, Result, string (+13 more)

### Community 32 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 33 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 34 - "MixerTests.cpp"
Cohesion: 0.11
Nodes (17): AudioBufferPool, AudioBufferView, EntityId, ProcessContext, Sample, size_t, TempoMap, vector (+9 more)

### Community 35 - "string"
Cohesion: 0.13
Nodes (13): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, EntityId, size_t (+5 more)

### Community 36 - "AudioBufferPool"
Cohesion: 0.13
Nodes (13): AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t, FrameCount (+5 more)

### Community 37 - "TimingProbeInstrument"
Cohesion: 0.11
Nodes (14): Applied, SimpleSynth, AudioBufferPool, AudioBufferView, FrameCount, MidiBuffer, MidiMessage, Sample (+6 more)

### Community 38 - "NoteCommands.cpp"
Cohesion: 0.17
Nodes (20): Command, EntityId, NoteIndices, size_t, vector, execute, findEvents(), canMergeWith (+12 more)

### Community 39 - "AudioBufferView"
Cohesion: 0.19
Nodes (9): renderAudioBlock, uint64_t, AudioBufferView, channels_, frames_, offset_, FrameCount, Sample (+1 more)

### Community 40 - "PlaylistView.mm"
Cohesion: 0.17
Nodes (20): -acceptsFirstResponder, -addTrackRect, -drawBarLinesInLaneAtheight, -drawClips, -drawRect, -drawRuler, -drawTracks, -gridPointFor (+12 more)

### Community 41 - "compileArrangement"
Cohesion: 0.23
Nodes (18): Emit, NoteSequence, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto(), EntityId (+10 more)

### Community 42 - "DelayLineNode"
Cohesion: 0.12
Nodes (16): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+8 more)

### Community 43 - "CompiledGraph"
Cohesion: 0.12
Nodes (14): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+6 more)

### Community 44 - "Options"
Cohesion: 0.11
Nodes (19): InstrumentNode, CallbackProfiler, int64_t, string, Options, amplitude, buffer, device (+11 more)

### Community 45 - "CountingCommand"
Cohesion: 0.12
Nodes (10): CountingCommand, counter_, delta_, Command, EntityId, string, Tick, makeProjectWithNotes() (+2 more)

### Community 46 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 47 - "GraphBuilder"
Cohesion: 0.12
Nodes (14): Connection, GraphBuilder, compensate_, connections_, error_, master_, nodes_, Node (+6 more)

### Community 48 - "AudioEngine.h"
Cohesion: 0.12
Nodes (15): MidiInput, mutex, AudioCaptureSink, CallbackProfiler, MidiBuffer, FrameCount, MidiMessage, SampleRate (+7 more)

### Community 49 - "MidiInput"
Cohesion: 0.14
Nodes (14): atomic, queueCapacity, size_t, uint64_t, MidiInput, dropped_, hasPending_, late_ (+6 more)

### Community 50 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 51 - "MixerCommands.cpp"
Cohesion: 0.16
Nodes (14): undo, Command, canMergeWith, mergeWith, canMergeWith, mergeWith, SetSendGainCommand, canMergeWith (+6 more)

### Community 52 - "LevelMeter"
Cohesion: 0.14
Nodes (13): atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond (+5 more)

### Community 53 - "InstrumentNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, unique_ptr, InstrumentNode, blockMidi_ (+6 more)

### Community 54 - "BasicMidiBuffer"
Cohesion: 0.13
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 55 - "PluginIdentifier"
Cohesion: 0.14
Nodes (13): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+5 more)

### Community 56 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 57 - "friend"
Cohesion: 0.12
Nodes (8): AutomationCurve, friend, AutomationPoint, curve, tension, tick, value, Tick

### Community 58 - "MidiRecorder"
Cohesion: 0.14
Nodes (14): CapturedMessage, FramePosition, MidiBuffer, atomic, queueCapacity, size_t, uint64_t, MidiRecorder (+6 more)

### Community 59 - "GraphCompileOptions"
Cohesion: 0.12
Nodes (17): PlaybackSource, GraphCompileOptions, channelCount, instrumentFactory, masterGain, maxBlockSize, parameters, pattern (+9 more)

### Community 60 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 61 - "SampleRingBuffer"
Cohesion: 0.21
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 62 - "Smoother"
Cohesion: 0.18
Nodes (10): atomic, AudioBufferView, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds (+2 more)

### Community 63 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 64 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 65 - "ioProcTrampoline"
Cohesion: 0.23
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 66 - "CoreMidiDevice"
Cohesion: 0.15
Nodes (14): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+6 more)

### Community 67 - "WavStreamWriter"
Cohesion: 0.13
Nodes (14): ofstream, Format, FrameCount, path, size_t, uint8_t, vector, WavStreamWriter (+6 more)

### Community 68 - "WavBytes.h"
Cohesion: 0.30
Nodes (15): appendCanonicalHeader(), bitsFor(), codeFor(), encodeSample(), Format, Sample, size_t, uint16_t (+7 more)

### Community 69 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 70 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 71 - "EntityId"
Cohesion: 0.21
Nodes (7): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, Value

### Community 72 - "RoutingConnection"
Cohesion: 0.12
Nodes (14): EntityId, findRouting, ids_, metadata_, tempoMap_, RoutingConnection, destination, gain (+6 more)

### Community 73 - "AddPatternClipCommand"
Cohesion: 0.15
Nodes (12): AddPatternClipCommand, clip_, execute, index_, length_, minted_, pattern_, start_ (+4 more)

### Community 74 - "append"
Cohesion: 0.28
Nodes (12): Format, FrameCount, path, Result, Sample, SampleRate, size_t, append (+4 more)

### Community 75 - "ProcessContext"
Cohesion: 0.14
Nodes (13): FramePosition, MidiBuffer, size_t, ProcessContext, frameCount, inputCount, inputs, liveMidi (+5 more)

### Community 76 - "compile"
Cohesion: 0.20
Nodes (14): Node, NodeIndex, SampleRate, size_t, unique_ptr, addNode, analyse, compile (+6 more)

### Community 77 - "DuplicateClipsCommand"
Cohesion: 0.19
Nodes (10): ClipIds, DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_ (+2 more)

### Community 78 - "EntityId"
Cohesion: 0.15
Nodes (6): EntityId, SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 79 - "EntityId"
Cohesion: 0.16
Nodes (11): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+3 more)

### Community 80 - "SetVelocityCommand"
Cohesion: 0.14
Nodes (8): string, SetVelocityCommand, channel_, indices_, pattern_, previousVelocities_, undo, velocity_

### Community 81 - "ToggleStepCommand"
Cohesion: 0.15
Nodes (9): Command, size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_ (+1 more)

### Community 82 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 83 - "Fixture"
Cohesion: 0.16
Nodes (12): EntityId, SequencedNote, Tick, vector, Fixture, channel, pattern, project (+4 more)

### Community 84 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 85 - "ChannelCommands.cpp"
Cohesion: 0.21
Nodes (10): Command, execute, SetChannelVolumeCommand, canMergeWith, channelId_, execute, mergeWith, previousVolume_ (+2 more)

### Community 86 - "ClipCommands.cpp"
Cohesion: 0.23
Nodes (11): Command, canMergeWith, mergeWith, ResizeClipsCommand, canMergeWith, clips_, execute, lengthDelta_ (+3 more)

### Community 87 - "MoveClipsCommand"
Cohesion: 0.19
Nodes (11): EntityId, execute, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, execute, tickDelta_ (+3 more)

### Community 88 - "string"
Cohesion: 0.18
Nodes (7): Command, string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 89 - "read"
Cohesion: 0.36
Nodes (12): Format, path, Result, uint8_t, vector, fillMetadata(), loadAndParse(), parseHeader() (+4 more)

### Community 90 - "MidiEvent"
Cohesion: 0.15
Nodes (13): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+5 more)

### Community 91 - "ParameterRegistry"
Cohesion: 0.22
Nodes (11): Entry, string, Entry, size_t, vector, ParameterRegistry, entries_, find (+3 more)

### Community 92 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 93 - "humanizeNoteStarts"
Cohesion: 0.26
Nodes (11): Kind, RecordedEvent, appendRecordedEvents(), MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts() (+3 more)

### Community 94 - "RemoveMixerNodeCommand"
Cohesion: 0.17
Nodes (10): RemovedRouting, vector, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_ (+2 more)

### Community 96 - "AutomationCommands.cpp"
Cohesion: 0.21
Nodes (11): execute, undo, AutomationPoint, EntityId, vector, findLane(), execute, undo (+3 more)

### Community 97 - "SetClipMutedCommand"
Cohesion: 0.17
Nodes (8): Command, vector, SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 98 - "AddMixerNodeCommand"
Cohesion: 0.18
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 100 - "Instrument"
Cohesion: 0.18
Nodes (9): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+1 more)

### Community 101 - "MidiDevice"
Cohesion: 0.17
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 102 - "WavStreamWriterTests.cpp"
Cohesion: 0.18
Nodes (10): FrameCount, path, size_t, uint8_t, vector, fileBytes(), makeTestSignal(), ScratchFile (+2 more)

### Community 103 - "Denormals.h"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 104 - "SetAutomationPointsCommand"
Cohesion: 0.24
Nodes (9): Command, AutomationPoint, vector, SetAutomationPointsCommand, canMergeWith, laneId_, mergeWith, points_ (+1 more)

### Community 105 - "RemoveClipsCommand"
Cohesion: 0.18
Nodes (9): string, Command, RemovedClip, RemoveClipsCommand, clips_, execute, name, removed_ (+1 more)

### Community 106 - "AddNoteCommand"
Cohesion: 0.20
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, size_t

### Community 107 - "start"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 108 - "AudioFileData"
Cohesion: 0.18
Nodes (10): AudioFileData, channelCount, channels, frameCount, sampleRate, FrameCount, Sample, SampleRate (+2 more)

### Community 109 - "RecordedEvent"
Cohesion: 0.18
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 110 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 111 - "BlockSegment"
Cohesion: 0.18
Nodes (9): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, size_t (+1 more)

### Community 112 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 113 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 114 - "DisconnectMixerCommand"
Cohesion: 0.20
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 115 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (9): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo (+1 more)

### Community 116 - "ParsedHeader"
Cohesion: 0.20
Nodes (10): size_t, uint16_t, uint32_t, ParsedHeader, bitsPerSample, channels, dataOffset, dataSize (+2 more)

### Community 117 - "Node"
Cohesion: 0.22
Nodes (4): FrameCount, SampleRate, Node, process

### Community 118 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 119 - "RemoveChannelCommand"
Cohesion: 0.22
Nodes (8): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, index_, undo

### Community 120 - "AddChannelCommand"
Cohesion: 0.22
Nodes (7): AddChannelCommand, channel_, execute, index_, minted_, undo, size_t

### Community 121 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 122 - "SetMixerPanCommand"
Cohesion: 0.22
Nodes (6): SetMixerPanCommand, execute, nodeId_, pan_, previous_, undo

### Community 123 - "SetMixerVolumeCommand"
Cohesion: 0.22
Nodes (6): SetMixerVolumeCommand, execute, nodeId_, previous_, undo, volume_

### Community 124 - "MoveNotesCommand"
Cohesion: 0.22
Nodes (8): MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, channel_, indices_, keyDelta_, pattern_, tickDelta_

### Community 125 - "AutomationFixture"
Cohesion: 0.22
Nodes (7): AutomationFixture, channel, pattern, project, tempo, EntityId, TempoMap

### Community 126 - "Command"
Cohesion: 0.25
Nodes (5): Command, execute, id, name, undo

### Community 127 - "SetChannelStepKeyCommand"
Cohesion: 0.25
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 128 - "RenameChannelCommand"
Cohesion: 0.25
Nodes (5): RenameChannelCommand, channelId_, execute, previousName_, undo

### Community 129 - "MixerCommands.h"
Cohesion: 0.25
Nodes (5): SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 130 - "SetMixerPolarityCommand"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 131 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 132 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 133 - "TimestampedMidiMessage"
Cohesion: 0.25
Nodes (8): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status

### Community 134 - "MidiDevice.h"
Cohesion: 0.25
Nodes (6): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback

### Community 135 - "ProjectMetadata"
Cohesion: 0.25
Nodes (8): ProjectMetadata, artist, comment, created, createdWith, lastSavedWith, modified, title

### Community 136 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 137 - "SetChannelMutedCommand"
Cohesion: 0.29
Nodes (6): Command, SetChannelMutedCommand, channelId_, execute, muted_, undo

### Community 138 - "PatternTests.cpp"
Cohesion: 0.48
Nodes (6): SequencedNote, Tick, vector, note(), shapeOf(), startsOf()

### Community 139 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 141 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 142 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 143 - "ChannelRackTests.cpp"
Cohesion: 0.33
Nodes (5): EntityId, Step, Tick, note(), stepAt()

### Community 144 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 145 - "process"
Cohesion: 0.40
Nodes (5): process, AudioBufferView, FrameCount, FramePosition, MidiBuffer

### Community 146 - "collectForBlock"
Cohesion: 0.40
Nodes (5): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock

### Community 147 - "check"
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
- **719 isolated node(s):** `index_`, `minted_`, `pattern_`, `index_`, `minted_` (+714 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **45 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `RenameChannelCommand`, `MixerCommands.h`, `SetMixerPolarityCommand`, `SetMixerSoloedCommand`, `PatternCommands.cpp`, `RemoveTrackCommand`, `PlaylistModel`, `ProjectMetadata`, `SetChannelMutedCommand`, `Channel`, `CommandRegistry`, `Clip`, `ResizeNotesCommand`, `Pattern`, `CompiledProjectGraph`, `ProjectFile.cpp`, `MixerTests.cpp`, `NoteCommands.cpp`, `compileArrangement`, `CountingCommand`, `MixerCommands.cpp`, `RoutingConnection`, `AddPatternClipCommand`, `DuplicateClipsCommand`, `EntityId`, `EntityId`, `SetVelocityCommand`, `Fixture`, `ChannelCommands.cpp`, `ClipCommands.cpp`, `MoveClipsCommand`, `string`, `RemoveMixerNodeCommand`, `AutomationCommands.cpp`, `SetClipMutedCommand`, `AddMixerNodeCommand`, `RemoveClipsCommand`, `AddNoteCommand`, `AudioAsset`, `DisconnectMixerCommand`, `DeleteNotesCommand`, `RemoveChannelCommand`, `AddChannelCommand`, `SetChannelOutputCommand`, `SetMixerPanCommand`, `SetMixerVolumeCommand`, `AutomationFixture`, `SetChannelStepKeyCommand`?**
  _High betweenness centrality (0.269) - this node is a cross-community bridge._