# Graph Report - project-continuation-670d11  (2026-08-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 3177 nodes · 5431 edges · 221 communities (187 shown, 34 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 257 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `295b1fed`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- RemoveTrackCommand
- Project
- PianoRollModel
- CoreAudioDevice
- PlaylistModel
- SimpleSynth
- EntityId
- InputMonitorNode
- AudioEngine
- MixerCommands.cpp
- Transport
- CommandRegistry
- ChannelRackModel
- EditAssetRegionCommand
- WavStreamWriter
- TempoMap
- InstrumentNode
- Clip
- MetronomeNode
- Json
- AudioDeviceConfig
- MixerView.mm
- AudioStream
- MusicalPosition
- Json.cpp
- CoreAudioDevice.cpp
- NoteSequence
- atomic
- MidiMessage
- PlaylistView.mm
- AudioRecorder
- WavBytes.h
- GraphCompileOptions
- ProjectFile.cpp
- ParsedHeader
- CallbackProfiler
- AudioDevice
- friend
- AddMixerNodeCommand
- AudioFileData
- AudioBufferPool
- CompiledGraph
- NoteCommands.cpp
- InsertRecordedTakeCommand
- LevelMeter
- compileArrangement
- AudioEngine.cpp
- AddAutomationLaneCommand
- RoutingConnection
- DelayLineNode
- CountingCommand
- RealtimeGuard.cpp
- AutomationNode
- WavStreamReader
- Region
- BasicMidiBuffer
- MidiInput
- SystemInfo
- EditFixture
- WavFile
- main
- RecordingSession
- Model.h
- RemoveClipsCommand
- AutomationWriteSession
- WriteAutomationCommand
- AudioBufferView
- PluginIdentifier
- Pattern
- ConstantNode
- MidiRecorder
- ClipCommands.cpp
- renderArrangement
- LockFreeQueue
- SampleRingBuffer
- Smoother
- MixerStripNode
- compile
- LoopbackResult
- LatentProcessorNode
- ioProcTrampoline
- MoveClipsCommand
- -applicationDidFinishLaunching
- CoreMidiDevice
- AutomationCommands.cpp
- ChannelCommands.cpp
- SetVelocityCommand
- GainNode
- SineOscillatorNode
- EntityId
- MixerTests.cpp
- TimingProbeInstrument
- GraphBuilder
- MidiEvent
- string
- PatternCommands.cpp
- ProcessContext
- vector
- QuantizeNotesCommand
- AddPatternCommand
- DuplicatePatternCommand
- renderClickFrames
- ParameterRegistry
- CompiledProjectGraph
- ChannelRackView.mm
- Fixture
- Options
- AutomationPoint
- RemoveMixerNodeCommand
- SetAutomationPointsCommand
- EntityId
- AddPatternClipCommand
- ToggleStepCommand
- main.mm
- CoreMidiDevice.cpp
- humanizeNoteStarts
- MixerNode
- WaveformOverview
- ResizeClipsCommand
- Channel
- Track
- renderNode
- Denormals.h
- TimelineAnchor
- AddNoteCommand
- vector
- captureAudioBlock
- MixerStripNode.cpp
- MixerFixture
- TimeSignatureEvent
- MidiDevice
- AudioAsset
- PatternListView.mm
- build
- DeleteNotesCommand
- DiskStreamer
- RecordedEvent
- Fixture
- AutomationProbe
- MidiTests.cpp
- make-dmg.sh
- ProjectGraphCompiler.cpp
- RemoveChannelCommand
- string
- DuplicateClipsCommand
- SetChannelOutputCommand
- MoveNotesCommand
- RemovePatternCommand
- AutomationFixture
- SetChannelSoloedCommand
- ResizeNotesCommand
- SetPatternSwingCommand
- Version
- TimestampedMidiMessage
- MidiDevice.h
- makeTestSignal
- compileProjectGraph
- SetChannelStepKeyCommand
- RenamePatternCommand
- AudioEditorView.mm
- PatternTests.cpp
- ScratchDirectory
- INCDAWMixerView
- INCDAWPianoRollView
- InstrumentTests.cpp
- RenameChannelCommand
- string
- noteAtStep
- collectForRange
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- OrderRecordingNode
- readAt
- WavStreamReader.cpp
- collectForBlock
- INCDAWAudioEditorView
- INCDAWPlaylistView
- check
- start
- ProjectGraphCompiler.h
- AudioCaptureSink
- INCDAWChannelRackView
- INCDAWPatternListView
- CompiledGraph
- CompiledProjectGraph
- FrameCount
- GraphCompileOptions
- InstrumentFactory
- InstrumentNode
- Sample
- RemovedClip
- Rect
- PlacedClip
- int64_t
- CallbackProfiler
- FramePosition
- ProcessContext
- T
- Kind
- friend
- MidiEventType
- MixerNodeType
- PluginIdentifier
- uint32_t
- Project
- TempoMap
- NSView
- NSMenu
- Step
- Project
- Tick
- Track
- uint64_t
- uint8_t
- Viewport

## God Nodes (most connected - your core abstractions)
1. `Project` - 149 edges
2. `AudioEngine` - 67 edges
3. `CoreAudioDevice` - 59 edges
4. `Json` - 44 edges
5. `TempoMap` - 40 edges
6. `Transport` - 37 edges
7. `Clip` - 37 edges
8. `SimpleSynth` - 36 edges
9. `MidiEvent` - 36 edges
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

## Communities (221 total, 34 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "RemoveTrackCommand"
Cohesion: 0.05
Nodes (43): AddTrackCommand, execute, index_, minted_, track_, undo, Command, Command (+35 more)

### Community 2 - "Project"
Cohesion: 0.10
Nodes (43): IdGenerator, undo, EntityId, size_t, vector, operator==(), content, events (+35 more)

### Community 3 - "PianoRollModel"
Cohesion: 0.09
Nodes (32): NoteList, size_t, Tick, vector, size_t, Tick, Viewport, PianoRollModel (+24 more)

### Community 4 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 5 - "PlaylistModel"
Cohesion: 0.09
Nodes (35): Rect, Clip, EntityId, Project, size_t, Tick, Track, vector (+27 more)

### Community 6 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 7 - "EntityId"
Cohesion: 0.06
Nodes (27): ConnectMixerCommand, connection_, destination_, gain_, index_, isSend_, minted_, preFader_ (+19 more)

### Community 8 - "InputMonitorNode"
Cohesion: 0.06
Nodes (29): PlacedClip, ProcessContext, AudioClipNode, addClip, clips_, fetchScratch_, prepare, process (+21 more)

### Community 9 - "AudioEngine"
Cohesion: 0.08
Nodes (29): atomic, AudioDevice, AudioIOCallback, CallbackProfiler, MidiBuffer, MidiInput, mutex, RetiredGraph (+21 more)

### Community 10 - "MixerCommands.cpp"
Cohesion: 0.08
Nodes (26): execute, Command, SetMixerPanCommand, canMergeWith, execute, mergeWith, nodeId_, pan_ (+18 more)

### Community 11 - "Transport"
Cohesion: 0.09
Nodes (24): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, Transport (+16 more)

### Community 12 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 13 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 14 - "EditAssetRegionCommand"
Cohesion: 0.09
Nodes (24): AudioEditOp, Command, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_ (+16 more)

### Community 15 - "WavStreamWriter"
Cohesion: 0.11
Nodes (26): ofstream, Format, FrameCount, path, Result, Sample, SampleRate, size_t (+18 more)

### Community 16 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 17 - "InstrumentNode"
Cohesion: 0.08
Nodes (23): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+15 more)

### Community 18 - "Clip"
Cohesion: 0.07
Nodes (29): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+21 more)

### Community 19 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 20 - "Json"
Cohesion: 0.08
Nodes (18): nullptr_t, pair, int64_t, int64_t, string, vector, Json, append (+10 more)

### Community 21 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 22 - "MixerView.mm"
Cohesion: 0.17
Nodes (26): MixerStripNode, NSMenu, -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect (+18 more)

### Community 23 - "AudioStream"
Cohesion: 0.11
Nodes (22): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+14 more)

### Community 24 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 25 - "Json.cpp"
Cohesion: 0.19
Nodes (22): size_t, string, escapeInto(), formatDouble(), asString, contains, dump, dumpTo (+14 more)

### Community 26 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 27 - "NoteSequence"
Cohesion: 0.11
Nodes (21): Tick, vector, size_t, Tick, uint32_t, vector, NoteSequence, byEnd_ (+13 more)

### Community 28 - "atomic"
Cohesion: 0.14
Nodes (5): atomic, MidiBuffer, array, Node, process

### Community 29 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 30 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (24): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+16 more)

### Community 31 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

### Community 32 - "WavBytes.h"
Cohesion: 0.21
Nodes (23): appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample, channels (+15 more)

### Community 33 - "GraphCompileOptions"
Cohesion: 0.09
Nodes (23): DiskStreamer, PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, instrumentFactory, masterGain, maxBlockSize (+15 more)

### Community 34 - "ProjectFile.cpp"
Cohesion: 0.23
Nodes (21): Json, automationPointFrom(), bindUnassignedContent(), EntityId, path, PluginIdentifier, Result, string (+13 more)

### Community 35 - "ParsedHeader"
Cohesion: 0.17
Nodes (22): AudioFileData, Format, path, Result, size_t, uint16_t, uint32_t, uint8_t (+14 more)

### Community 36 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 37 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 38 - "friend"
Cohesion: 0.10
Nodes (13): AutomationCurve, friend, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint (+5 more)

### Community 39 - "AddMixerNodeCommand"
Cohesion: 0.11
Nodes (14): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType (+6 more)

### Community 40 - "AudioFileData"
Cohesion: 0.10
Nodes (20): AudioFileData, channelCount, channels, frameCount, sampleRate, FrameCount, Sample, SampleRate (+12 more)

### Community 41 - "AudioBufferPool"
Cohesion: 0.13
Nodes (13): AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t, FrameCount (+5 more)

### Community 42 - "CompiledGraph"
Cohesion: 0.11
Nodes (15): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+7 more)

### Community 43 - "NoteCommands.cpp"
Cohesion: 0.17
Nodes (20): undo, Command, EntityId, NoteIndices, size_t, vector, execute, findEvents() (+12 more)

### Community 44 - "InsertRecordedTakeCommand"
Cohesion: 0.11
Nodes (16): Command, EntityId, Placement, size_t, InsertRecordedTakeCommand, asset_, assetIndex_, clip_ (+8 more)

### Community 45 - "LevelMeter"
Cohesion: 0.12
Nodes (14): Node, atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_ (+6 more)

### Community 46 - "compileArrangement"
Cohesion: 0.23
Nodes (18): Emit, NoteSequence, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto(), EntityId (+10 more)

### Community 47 - "AudioEngine.cpp"
Cohesion: 0.17
Nodes (17): int64_t, audioDeviceAboutToStart, audioDeviceStopped, bufferSize, captureAudioBlock, collectRetiredGraphs, inputChannels, isRunning (+9 more)

### Community 48 - "AddAutomationLaneCommand"
Cohesion: 0.13
Nodes (13): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, AutomationLane, EntityId (+5 more)

### Community 49 - "RoutingConnection"
Cohesion: 0.10
Nodes (16): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t, findRouting (+8 more)

### Community 50 - "DelayLineNode"
Cohesion: 0.12
Nodes (16): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+8 more)

### Community 51 - "CountingCommand"
Cohesion: 0.12
Nodes (10): CountingCommand, counter_, delta_, Command, EntityId, string, Tick, makeProjectWithNotes() (+2 more)

### Community 52 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 53 - "AutomationNode"
Cohesion: 0.15
Nodes (12): Binding, Node, AutomationNode, bindings_, tempoMap_, size_t, TempoMap, vector (+4 more)

### Community 54 - "WavStreamReader"
Cohesion: 0.12
Nodes (16): ifstream, FrameCount, path, SampleRate, size_t, uint16_t, uint64_t, uint8_t (+8 more)

### Community 55 - "Region"
Cohesion: 0.30
Nodes (16): applyGain(), applyRamp(), clampedRegion(), AudioFileData, Sample, fadeIn(), fadeOut(), FrameCount (+8 more)

### Community 56 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 57 - "MidiInput"
Cohesion: 0.14
Nodes (14): atomic, queueCapacity, size_t, uint64_t, MidiInput, dropped_, hasPending_, late_ (+6 more)

### Community 58 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 59 - "EditFixture"
Cohesion: 0.14
Nodes (15): AudioFileData, EntityId, FrameCount, Project, Sample, size_t, EditFixture, assetId (+7 more)

### Community 60 - "WavFile"
Cohesion: 0.25
Nodes (17): AudioAsset, assetFilePath(), AudioFileData, EntityId, Project, Sample, string, vector (+9 more)

### Community 61 - "main"
Cohesion: 0.14
Nodes (17): AudioDeviceInfo, availableDevices, midiInput_, profiler_, sampleRate, setGraph, transport_, CompiledGraph (+9 more)

### Community 62 - "RecordingSession"
Cohesion: 0.14
Nodes (12): AudioRecorder, string, Placement, string, FrameCount, uint64_t, RecordingSession, arm (+4 more)

### Community 63 - "Model.h"
Cohesion: 0.12
Nodes (16): PluginIdentifier, string, TempoMap, PluginSlot, bypassed, id, plugin, stateFile (+8 more)

### Community 64 - "RemoveClipsCommand"
Cohesion: 0.14
Nodes (13): RemovedClip, string, Command, vector, RemoveClipsCommand, clips_, name, removed_ (+5 more)

### Community 65 - "AutomationWriteSession"
Cohesion: 0.13
Nodes (14): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, Command, EntityId (+6 more)

### Community 66 - "WriteAutomationCommand"
Cohesion: 0.11
Nodes (17): Clip, Track, WriteAutomationCommand, clip_, clipIndex_, key_, laneAfter_, laneCreated_ (+9 more)

### Community 67 - "AudioBufferView"
Cohesion: 0.22
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 68 - "PluginIdentifier"
Cohesion: 0.14
Nodes (13): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+5 more)

### Community 69 - "Pattern"
Cohesion: 0.12
Nodes (16): EntityId, vector, Pattern, automationLanes, channels, colour, id, length (+8 more)

### Community 70 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 71 - "MidiRecorder"
Cohesion: 0.14
Nodes (14): CapturedMessage, FramePosition, MidiBuffer, atomic, queueCapacity, size_t, uint64_t, MidiRecorder (+6 more)

### Community 72 - "ClipCommands.cpp"
Cohesion: 0.24
Nodes (15): execute, undo, EntityId, Project, execute, undo, execute, undo (+7 more)

### Community 73 - "renderArrangement"
Cohesion: 0.15
Nodes (14): vector, string, AudioFileData, FrameCount, path, Project, Sample, size_t (+6 more)

### Community 74 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 75 - "SampleRingBuffer"
Cohesion: 0.21
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 76 - "Smoother"
Cohesion: 0.18
Nodes (10): atomic, AudioBufferView, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds (+2 more)

### Community 77 - "MixerStripNode"
Cohesion: 0.12
Nodes (11): ProcessContext, atomic, Node, Sample, MixerStripNode, left_, meter_, muted_ (+3 more)

### Community 78 - "compile"
Cohesion: 0.17
Nodes (16): process, AudioBufferView, FrameCount, FramePosition, MidiBuffer, Node, NodeIndex, SampleRate (+8 more)

### Community 79 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 80 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 81 - "ioProcTrampoline"
Cohesion: 0.23
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 82 - "MoveClipsCommand"
Cohesion: 0.16
Nodes (10): ClipIds, MovedAudioClip, Tick, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, movedAudio_ (+2 more)

### Community 83 - "-applicationDidFinishLaunching"
Cohesion: 0.18
Nodes (16): INCDAWAudioEditorView, INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, INCDAWPlaylistView, NSApplicationDelegate, NSObject (+8 more)

### Community 84 - "CoreMidiDevice"
Cohesion: 0.15
Nodes (14): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+6 more)

### Community 85 - "AutomationCommands.cpp"
Cohesion: 0.22
Nodes (15): execute, undo, AutomationLane, AutomationPoint, EntityId, Project, vector, findLane() (+7 more)

### Community 86 - "ChannelCommands.cpp"
Cohesion: 0.17
Nodes (11): Command, execute, execute, SetChannelVolumeCommand, canMergeWith, channelId_, execute, mergeWith (+3 more)

### Community 87 - "SetVelocityCommand"
Cohesion: 0.18
Nodes (10): EntityId, NoteIndices, Tick, SetVelocityCommand, channel_, indices_, pattern_, previousVelocities_ (+2 more)

### Community 88 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 89 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 90 - "EntityId"
Cohesion: 0.21
Nodes (7): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, Value

### Community 91 - "MixerTests.cpp"
Cohesion: 0.17
Nodes (11): AudioBufferPool, AudioBufferView, ProcessContext, Sample, size_t, vector, channel, onsets() (+3 more)

### Community 92 - "TimingProbeInstrument"
Cohesion: 0.15
Nodes (9): Applied, AudioBufferView, FrameCount, MidiMessage, SampleRate, vector, TimingProbeInstrument, applied (+1 more)

### Community 93 - "GraphBuilder"
Cohesion: 0.14
Nodes (12): Connection, GraphBuilder, compensate_, connections_, error_, master_, nodes_, Node (+4 more)

### Community 94 - "MidiEvent"
Cohesion: 0.13
Nodes (15): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+7 more)

### Community 95 - "string"
Cohesion: 0.15
Nodes (8): AddChannelCommand, channel_, execute, index_, minted_, undo, size_t, string

### Community 96 - "PatternCommands.cpp"
Cohesion: 0.20
Nodes (12): Command, Tick, SetPatternLengthCommand, canMergeWith, execute, length_, mergeWith, patternId_ (+4 more)

### Community 97 - "ProcessContext"
Cohesion: 0.12
Nodes (15): FrameCount, FramePosition, MidiBuffer, SampleRate, size_t, ProcessContext, frameCount, inputCount (+7 more)

### Community 98 - "vector"
Cohesion: 0.15
Nodes (6): Command, execute, id, name, undo, vector

### Community 99 - "QuantizeNotesCommand"
Cohesion: 0.14
Nodes (8): string, QuantizeNotesCommand, channel_, grid_, pattern_, previousEvents_, strength_, undo

### Community 100 - "AddPatternCommand"
Cohesion: 0.16
Nodes (8): AddPatternCommand, execute, index_, minted_, pattern_, undo, size_t, string

### Community 101 - "DuplicatePatternCommand"
Cohesion: 0.16
Nodes (8): DuplicatePatternCommand, execute, index_, minted_, pattern_, source_, undo, EntityId

### Community 102 - "renderClickFrames"
Cohesion: 0.15
Nodes (12): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, FramePosition (+4 more)

### Community 103 - "ParameterRegistry"
Cohesion: 0.20
Nodes (11): Entry, string, Entry, size_t, vector, ParameterRegistry, entries_, find (+3 more)

### Community 104 - "CompiledProjectGraph"
Cohesion: 0.14
Nodes (14): CompiledProjectGraph, automation, channels, channelStrips, error, graph, instruments, mixerNodes (+6 more)

### Community 105 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 106 - "Fixture"
Cohesion: 0.16
Nodes (12): EntityId, SequencedNote, Tick, vector, Fixture, channel, pattern, project (+4 more)

### Community 107 - "Options"
Cohesion: 0.14
Nodes (14): int64_t, Options, amplitude, buffer, device, frequency, input, listOnly (+6 more)

### Community 108 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 109 - "RemoveMixerNodeCommand"
Cohesion: 0.15
Nodes (10): RemovedRouting, vector, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_ (+2 more)

### Community 110 - "SetAutomationPointsCommand"
Cohesion: 0.21
Nodes (9): Command, AutomationPoint, vector, SetAutomationPointsCommand, canMergeWith, laneId_, mergeWith, points_ (+1 more)

### Community 111 - "EntityId"
Cohesion: 0.17
Nodes (6): EntityId, SetChannelMutedCommand, channelId_, execute, muted_, undo

### Community 112 - "AddPatternClipCommand"
Cohesion: 0.18
Nodes (10): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+2 more)

### Community 113 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (9): Command, size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_ (+1 more)

### Community 114 - "main.mm"
Cohesion: 0.23
Nodes (12): -editorChanged, -openAudioAssetInEditor, -selectChannel, -selectPattern, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll (+4 more)

### Community 115 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 116 - "humanizeNoteStarts"
Cohesion: 0.26
Nodes (11): Kind, RecordedEvent, appendRecordedEvents(), MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts() (+3 more)

### Community 117 - "MixerNode"
Cohesion: 0.17
Nodes (12): MixerNodeType, MixerNode, colour, id, inserts, muted, name, pan (+4 more)

### Community 118 - "WaveformOverview"
Cohesion: 0.18
Nodes (11): SampleRate, Bucket, FrameCount, size_t, vector, WaveformOverview, channelCount, channels (+3 more)

### Community 119 - "ResizeClipsCommand"
Cohesion: 0.18
Nodes (11): Command, FrameCount, canMergeWith, mergeWith, ResizeClipsCommand, canMergeWith, clips_, lengthDelta_ (+3 more)

### Community 120 - "Channel"
Cohesion: 0.17
Nodes (12): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+4 more)

### Community 121 - "Track"
Cohesion: 0.17
Nodes (12): findTrack, Track, colour, height, id, muted, name, outputMixerNode (+4 more)

### Community 122 - "renderNode"
Cohesion: 0.22
Nodes (11): AudioFileData, shared_ptr, add, FrameCount, Node, Sample, size_t, vector (+3 more)

### Community 123 - "Denormals.h"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 124 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 125 - "AddNoteCommand"
Cohesion: 0.20
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, Command, size_t

### Community 126 - "vector"
Cohesion: 0.18
Nodes (6): vector, EntityId, Step, Tick, note(), stepAt()

### Community 127 - "captureAudioBlock"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 128 - "MixerStripNode.cpp"
Cohesion: 0.31
Nodes (10): FrameCount, Sample, SampleRate, panGains, prepare, refreshTargets, setGain, setMuted (+2 more)

### Community 129 - "MixerFixture"
Cohesion: 0.18
Nodes (8): buildParallelPaths(), EntityId, TempoMap, unique_ptr, MixerFixture, pattern, project, tempo

### Community 130 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 131 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 132 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 133 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 134 - "build"
Cohesion: 0.27
Nodes (9): Result, bucketize(), AudioFileData, Bucket, FrameCount, Sample, vector, sizeBuckets() (+1 more)

### Community 135 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (9): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo (+1 more)

### Community 136 - "DiskStreamer"
Cohesion: 0.27
Nodes (8): DiskStreamer, mutex_, running_, serviceOnce, streams_, thread_, vector, weak_ptr

### Community 137 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 138 - "Fixture"
Cohesion: 0.20
Nodes (8): EntityId, Project, Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 139 - "AutomationProbe"
Cohesion: 0.24
Nodes (7): AutomationProbe, calls, registry, written, Project, vector, makeProject()

### Community 140 - "MidiTests.cpp"
Cohesion: 0.22
Nodes (9): FrameCount, MidiMessage, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped() (+1 more)

### Community 141 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 142 - "ProjectGraphCompiler.cpp"
Cohesion: 0.31
Nodes (8): AutomationNode, channelStripFor, instrumentFor, stripFor, EntityId, InstrumentNode, MixerStripNode, MixerStripNode

### Community 143 - "RemoveChannelCommand"
Cohesion: 0.22
Nodes (8): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, index_, undo

### Community 144 - "string"
Cohesion: 0.31
Nodes (3): vector, Command, string

### Community 145 - "DuplicateClipsCommand"
Cohesion: 0.22
Nodes (8): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_, Clip

### Community 146 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 147 - "MoveNotesCommand"
Cohesion: 0.22
Nodes (8): MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, channel_, indices_, keyDelta_, pattern_, tickDelta_

### Community 148 - "RemovePatternCommand"
Cohesion: 0.22
Nodes (6): RemovePatternCommand, execute, index_, pattern_, patternId_, undo

### Community 149 - "AutomationFixture"
Cohesion: 0.22
Nodes (7): AutomationFixture, channel, pattern, project, tempo, EntityId, TempoMap

### Community 150 - "SetChannelSoloedCommand"
Cohesion: 0.29
Nodes (6): Command, SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 151 - "ResizeNotesCommand"
Cohesion: 0.25
Nodes (7): ResizeNotesCommand, channel_, durationDelta_, indices_, pattern_, previousDurations_, undo

### Community 152 - "SetPatternSwingCommand"
Cohesion: 0.25
Nodes (6): SetPatternSwingCommand, execute, patternId_, previousSwing_, swing_, undo

### Community 153 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 154 - "TimestampedMidiMessage"
Cohesion: 0.25
Nodes (8): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status

### Community 155 - "MidiDevice.h"
Cohesion: 0.25
Nodes (6): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback

### Community 156 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 157 - "compileProjectGraph"
Cohesion: 0.29
Nodes (7): Channel, Project, compileProjectGraph(), InstrumentFactory, defaultInstrumentFactory(), isAudible(), TempoMap

### Community 158 - "SetChannelStepKeyCommand"
Cohesion: 0.22
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 159 - "RenamePatternCommand"
Cohesion: 0.29
Nodes (6): Command, RenamePatternCommand, execute, patternId_, previousName_, undo

### Community 160 - "AudioEditorView.mm"
Cohesion: 0.29
Nodes (6): -acceptsFirstResponder, -hasSelection, -initWithFrameprojectregistry, -isFlipped, -selectionFrom, -selectionTo

### Community 161 - "PatternTests.cpp"
Cohesion: 0.48
Nodes (6): SequencedNote, Tick, vector, note(), shapeOf(), startsOf()

### Community 162 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 163 - "INCDAWMixerView"
Cohesion: 0.33
Nodes (5): incdaw, NSView, INCDAWMixerView, -initWithFrameprojectregistry, stripLookup

### Community 164 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 165 - "InstrumentTests.cpp"
Cohesion: 0.40
Nodes (5): SimpleSynth, AudioBufferPool, MidiBuffer, Sample, renderSynth()

### Community 166 - "RenameChannelCommand"
Cohesion: 0.33
Nodes (4): RenameChannelCommand, channelId_, previousName_, undo

### Community 168 - "noteAtStep"
Cohesion: 0.40
Nodes (5): size_t, Tick, vector, noteAtStep(), execute

### Community 169 - "collectForRange"
Cohesion: 0.33
Nodes (4): FrameCount, FramePosition, MidiBuffer, collectForRange

### Community 170 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 171 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 172 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 173 - "readAt"
Cohesion: 0.40
Nodes (4): FrameCount, Sample, size_t, readAt

### Community 174 - "WavStreamReader.cpp"
Cohesion: 0.50
Nodes (4): path, Result, close, open

### Community 175 - "collectForBlock"
Cohesion: 0.40
Nodes (5): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock

### Community 176 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 177 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 178 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 179 - "start"
Cohesion: 0.50
Nodes (4): AudioDeviceConfig, deviceName, start, string

### Community 180 - "ProjectGraphCompiler.h"
Cohesion: 0.50
Nodes (3): Instrument, ParameterRegistry, string

### Community 182 - "INCDAWChannelRackView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWChannelRackView, -initWithFrameprojectregistry

### Community 183 - "INCDAWPatternListView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWPatternListView, -initWithFrameprojectregistry

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
- **831 isolated node(s):** `index_`, `minted_`, `track_`, `clips_`, `index_` (+826 more)
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
- **Why does `Project` connect `Project` to `RemoveTrackCommand`, `MixerFixture`, `AudioAsset`, `EntityId`, `DeleteNotesCommand`, `MixerCommands.cpp`, `AutomationProbe`, `CommandRegistry`, `RemoveChannelCommand`, `SetChannelOutputCommand`, `Clip`, `RemovePatternCommand`, `AutomationFixture`, `SetChannelSoloedCommand`, `ResizeNotesCommand`, `SetPatternSwingCommand`, `SetChannelStepKeyCommand`, `RenamePatternCommand`, `ProjectFile.cpp`, `RenameChannelCommand`, `AddMixerNodeCommand`, `noteAtStep`, `friend`, `NoteCommands.cpp`, `InsertRecordedTakeCommand`, `compileArrangement`, `RoutingConnection`, `CountingCommand`, `Model.h`, `Pattern`, `ChannelCommands.cpp`, `SetVelocityCommand`, `string`, `PatternCommands.cpp`, `QuantizeNotesCommand`, `AddPatternCommand`, `DuplicatePatternCommand`, `Fixture`, `RemoveMixerNodeCommand`, `EntityId`, `MixerNode`, `Channel`, `Track`, `AddNoteCommand`?**
  _High betweenness centrality (0.223) - this node is a cross-community bridge._