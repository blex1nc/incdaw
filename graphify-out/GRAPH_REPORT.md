# Graph Report - project-continuation-670d11  (2026-08-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 3232 nodes · 5497 edges · 232 communities (195 shown, 37 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 258 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `30d91310`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- Transport
- PianoRollModel
- WavBytes.h
- CoreAudioDevice
- PlaylistModel
- SimpleSynth
- Project
- CommandRegistry
- AudioEngine
- EditAssetRegionCommand
- ChannelRackModel
- main
- MixerNode
- WavStreamWriter
- NoteSequence
- RecordingSession
- InstrumentNode
- MidiMessage
- Clip
- WavStreamReader
- AudioStream
- PatternChannelContent
- Json
- AudioDeviceConfig
- MixerView.mm
- Json.cpp
- CoreAudioDevice.cpp
- InsertRecordedTakeCommand
- ProjectFile.cpp
- MidiEvent
- atomic
- SetAutomationPointsCommand
- PlaylistView.mm
- AudioRecorder
- GraphCompileOptions
- WriteAutomationCommand
- CallbackProfiler
- TimeSignature
- MetronomeNode
- AudioDevice
- WaveformOverview
- NoteCommands.cpp
- AudioFileData
- AudioBufferPool
- CompiledGraph
- compileArrangement
- LevelMeter
- AudioEngine.cpp
- AddAutomationLaneCommand
- DelayLineNode
- CompiledProjectGraph
- CountingCommand
- RealtimeGuard.cpp
- Region
- MidiInput
- SystemInfo
- EditFixture
- WavFile
- AudioClipNode
- AutomationWriteSession
- MixerCommands.cpp
- AudioLogger
- AudioBufferView
- BasicMidiBuffer
- ConstantNode
- RemoveClipsCommand
- ClipCommands.cpp
- MoveNotesCommand
- LockFreeQueue
- SampleRingBuffer
- Smoother
- MixerStripNode
- compile
- LoopbackResult
- LatentProcessorNode
- ioProcTrampoline
- -applicationDidFinishLaunching
- CoreMidiDevice
- InputMonitorNode
- ResizeNotesCommand
- Pattern
- GainNode
- SineOscillatorNode
- EntityId
- RoutingConnection
- MixerTests.cpp
- TimingProbeInstrument
- GraphBuilder
- vector
- PatternCommands.cpp
- TrackCommands.cpp
- ProcessContext
- PluginIdentifier
- ResizeClipsCommand
- MoveClipsCommand
- string
- ChannelCommands.cpp
- EntityId
- SetVelocityCommand
- AddPatternCommand
- DuplicatePatternCommand
- ToggleStepCommand
- process
- ParameterRegistry
- ChannelRackView.mm
- renderArrangement
- Fixture
- AutomationPoint
- AutomationNode
- RemoveMixerNodeCommand
- TempoMap
- EntityId
- AddPatternClipCommand
- RenameTrackCommand
- Channel
- Track
- main.mm
- compileProjectGraph
- MidiRecorder
- CoreMidiDevice.cpp
- AddMixerNodeCommand
- renderNode
- TimelineAnchor
- MidiTests.cpp
- AutomationProbe
- AddNoteCommand
- EntityId
- DiskStreamer
- captureAudioBlock
- Node
- MixerStripNode.cpp
- MixerFixture
- TimeSignatureEvent
- MidiDevice
- AudioAsset
- PatternListView.mm
- string
- DisconnectMixerCommand
- DeleteNotesCommand
- RecordedEvent
- Fixture
- make-dmg.sh
- RemoveChannelCommand
- DuplicateClipsCommand
- SetMixerMutedCommand
- SetChannelOutputCommand
- SetMixerVolumeCommand
- SetSendGainCommand
- RemovePatternCommand
- availableDevices
- AutomationFixture
- grab
- Denormals.h
- Command
- SetChannelMutedCommand
- SetChannelStepKeyCommand
- SetMixerPolarityCommand
- Command
- SetPatternSwingCommand
- AddTrackCommand
- RemoveTrackCommand
- SetTrackMutedCommand
- Version
- TimestampedMidiMessage
- MidiDevice.h
- makeTestSignal
- InstrumentTests.cpp
- RenameChannelCommand
- RenamePatternCommand
- AudioEditorView.mm
- PatternTests.cpp
- ScratchDirectory
- INCDAWMixerView
- INCDAWPianoRollView
- string
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- renderBlock
- OrderRecordingNode
- start
- vector
- collectForBlock
- INCDAWAudioEditorView
- INCDAWPlaylistView
- AutomationTests.cpp
- check
- AudioCaptureSink
- framesToSeconds
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
- Command
- EntityId
- Rect
- PlacedClip
- int64_t
- string
- CallbackProfiler
- FramePosition
- ProcessContext
- atomic
- Tick
- T
- Kind
- friend
- MidiEventType
- MixerNodeType
- PluginIdentifier
- uint32_t
- Project
- TempoMap
- uint64_t
- NSView
- NSMenu
- Step
- Project
- uint8_t
- Viewport

## God Nodes (most connected - your core abstractions)
1. `Project` - 147 edges
2. `AudioEngine` - 67 edges
3. `CoreAudioDevice` - 59 edges
4. `Json` - 44 edges
5. `Transport` - 38 edges
6. `TempoMap` - 36 edges
7. `Clip` - 36 edges
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

## Communities (232 total, 37 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "Transport"
Cohesion: 0.06
Nodes (41): atomic, MusicalPosition, BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount (+33 more)

### Community 2 - "PianoRollModel"
Cohesion: 0.07
Nodes (39): NoteList, size_t, Tick, vector, size_t, Tick, Viewport, PianoRollModel (+31 more)

### Community 3 - "WavBytes.h"
Cohesion: 0.09
Nodes (45): appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample, channels (+37 more)

### Community 4 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 5 - "PlaylistModel"
Cohesion: 0.09
Nodes (35): Rect, Clip, EntityId, Project, size_t, Tick, Track, vector (+27 more)

### Community 6 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 7 - "Project"
Cohesion: 0.13
Nodes (35): IdGenerator, undo, EntityId, size_t, vector, operator==(), events, totalEventCount (+27 more)

### Community 8 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 9 - "AudioEngine"
Cohesion: 0.08
Nodes (26): AudioDevice, AudioIOCallback, CallbackProfiler, MidiBuffer, mutex, RetiredGraph, SampleRingBuffer, AudioCaptureSink (+18 more)

### Community 10 - "EditAssetRegionCommand"
Cohesion: 0.08
Nodes (24): AudioEditOp, Command, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_ (+16 more)

### Community 11 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 12 - "main"
Cohesion: 0.09
Nodes (26): midiInput_, profiler_, sampleRate, setGraph, transport_, CompiledGraph, unique_ptr, CallbackProfiler (+18 more)

### Community 13 - "MixerNode"
Cohesion: 0.08
Nodes (29): MixerNodeType, PluginIdentifier, string, TempoMap, MixerNode, colour, id, inserts (+21 more)

### Community 14 - "WavStreamWriter"
Cohesion: 0.11
Nodes (26): ofstream, Format, FrameCount, path, Result, Sample, SampleRate, size_t (+18 more)

### Community 15 - "NoteSequence"
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, MidiBuffer, Tick, vector, size_t, Tick, uint32_t (+17 more)

### Community 16 - "RecordingSession"
Cohesion: 0.09
Nodes (23): AudioEngine, AudioRecorder, path, Slice, Placement, string, vector, FrameCount (+15 more)

### Community 17 - "InstrumentNode"
Cohesion: 0.08
Nodes (23): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+15 more)

### Community 18 - "MidiMessage"
Cohesion: 0.09
Nodes (14): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+6 more)

### Community 19 - "Clip"
Cohesion: 0.07
Nodes (29): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+21 more)

### Community 20 - "WavStreamReader"
Cohesion: 0.09
Nodes (24): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+16 more)

### Community 21 - "AudioStream"
Cohesion: 0.10
Nodes (24): shared_ptr, AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_ (+16 more)

### Community 22 - "PatternChannelContent"
Cohesion: 0.08
Nodes (18): AutomationCurve, friend, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint (+10 more)

### Community 23 - "Json"
Cohesion: 0.08
Nodes (18): nullptr_t, pair, int64_t, int64_t, string, vector, Json, append (+10 more)

### Community 24 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 25 - "MixerView.mm"
Cohesion: 0.17
Nodes (26): MixerStripNode, NSMenu, -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect (+18 more)

### Community 26 - "Json.cpp"
Cohesion: 0.19
Nodes (22): size_t, string, escapeInto(), formatDouble(), asString, contains, dump, dumpTo (+14 more)

### Community 27 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 28 - "InsertRecordedTakeCommand"
Cohesion: 0.09
Nodes (20): Clip, EntityId, Project, Placement, size_t, string, vector, InsertRecordedTakeCommand (+12 more)

### Community 29 - "ProjectFile.cpp"
Cohesion: 0.21
Nodes (23): Json, automationPointFrom(), bindUnassignedContent(), AutomationPoint, EntityId, path, PluginIdentifier, Result (+15 more)

### Community 30 - "MidiEvent"
Cohesion: 0.11
Nodes (24): Kind, MidiEventType, RecordedEvent, appendRecordedEvents(), MidiEventType, Tick, uint64_t, vector (+16 more)

### Community 31 - "atomic"
Cohesion: 0.11
Nodes (6): vector, atomic, MidiBuffer, array, allocationSize(), size_t

### Community 32 - "SetAutomationPointsCommand"
Cohesion: 0.13
Nodes (22): execute, undo, AutomationLane, AutomationPoint, Command, EntityId, Project, vector (+14 more)

### Community 33 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (24): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+16 more)

### Community 34 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

### Community 35 - "GraphCompileOptions"
Cohesion: 0.09
Nodes (23): DiskStreamer, PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, instrumentFactory, masterGain, maxBlockSize (+15 more)

### Community 36 - "WriteAutomationCommand"
Cohesion: 0.10
Nodes (19): AutomationPoint, Clip, Track, vector, WriteAutomationCommand, clip_, clipIndex_, key_ (+11 more)

### Community 37 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 38 - "TimeSignature"
Cohesion: 0.13
Nodes (16): Tick, friend, int64_t, Tick, MusicalPosition, bar, beat, tick (+8 more)

### Community 39 - "MetronomeNode"
Cohesion: 0.09
Nodes (17): atomic, FrameCount, Sample, SampleRate, size_t, vector, MetronomeNode, amplitude_ (+9 more)

### Community 40 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 41 - "WaveformOverview"
Cohesion: 0.12
Nodes (20): Result, SampleRate, bucketize(), AudioFileData, Bucket, FrameCount, Sample, vector (+12 more)

### Community 42 - "NoteCommands.cpp"
Cohesion: 0.16
Nodes (21): Command, EntityId, NoteIndices, size_t, vector, execute, findEvents(), canMergeWith (+13 more)

### Community 43 - "AudioFileData"
Cohesion: 0.10
Nodes (20): AudioFileData, channelCount, channels, frameCount, sampleRate, FrameCount, Sample, SampleRate (+12 more)

### Community 44 - "AudioBufferPool"
Cohesion: 0.13
Nodes (13): AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t, FrameCount (+5 more)

### Community 45 - "CompiledGraph"
Cohesion: 0.11
Nodes (15): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+7 more)

### Community 46 - "compileArrangement"
Cohesion: 0.21
Nodes (19): Emit, NoteSequence, content, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto() (+11 more)

### Community 47 - "LevelMeter"
Cohesion: 0.12
Nodes (14): Node, atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_ (+6 more)

### Community 48 - "AudioEngine.cpp"
Cohesion: 0.17
Nodes (17): int64_t, audioDeviceAboutToStart, audioDeviceStopped, bufferSize, captureAudioBlock, collectRetiredGraphs, inputChannels, isRunning (+9 more)

### Community 49 - "AddAutomationLaneCommand"
Cohesion: 0.13
Nodes (13): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, AutomationLane, EntityId (+5 more)

### Community 50 - "DelayLineNode"
Cohesion: 0.12
Nodes (16): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+8 more)

### Community 51 - "CompiledProjectGraph"
Cohesion: 0.12
Nodes (20): CompiledProjectGraph, automation, channels, channelStripFor, channelStrips, error, graph, instrumentFor (+12 more)

### Community 52 - "CountingCommand"
Cohesion: 0.12
Nodes (10): CountingCommand, counter_, delta_, Command, EntityId, string, Tick, makeProjectWithNotes() (+2 more)

### Community 53 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 54 - "Region"
Cohesion: 0.30
Nodes (16): applyGain(), applyRamp(), clampedRegion(), AudioFileData, Sample, fadeIn(), fadeOut(), FrameCount (+8 more)

### Community 55 - "MidiInput"
Cohesion: 0.14
Nodes (14): atomic, queueCapacity, size_t, uint64_t, MidiInput, dropped_, hasPending_, late_ (+6 more)

### Community 56 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 57 - "EditFixture"
Cohesion: 0.14
Nodes (15): AudioFileData, EntityId, FrameCount, Project, Sample, size_t, EditFixture, assetId (+7 more)

### Community 58 - "WavFile"
Cohesion: 0.25
Nodes (17): AudioAsset, assetFilePath(), AudioFileData, EntityId, Project, Sample, string, vector (+9 more)

### Community 59 - "AudioClipNode"
Cohesion: 0.12
Nodes (13): PlacedClip, ProcessContext, AudioClipNode, addClip, clips_, fetchScratch_, prepare, process (+5 more)

### Community 60 - "AutomationWriteSession"
Cohesion: 0.13
Nodes (14): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, Command, EntityId (+6 more)

### Community 61 - "MixerCommands.cpp"
Cohesion: 0.16
Nodes (14): execute, Command, SetMixerPanCommand, canMergeWith, execute, mergeWith, nodeId_, pan_ (+6 more)

### Community 62 - "AudioLogger"
Cohesion: 0.12
Nodes (14): AudioLogger, capacityFrames_, circle_, enabled_, ready_, scratch_, written_, atomic (+6 more)

### Community 63 - "AudioBufferView"
Cohesion: 0.22
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 64 - "BasicMidiBuffer"
Cohesion: 0.13
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 65 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 66 - "RemoveClipsCommand"
Cohesion: 0.15
Nodes (13): RemovedClip, string, Command, vector, RemoveClipsCommand, clips_, name, removed_ (+5 more)

### Community 67 - "ClipCommands.cpp"
Cohesion: 0.24
Nodes (15): execute, undo, EntityId, Project, execute, undo, execute, undo (+7 more)

### Community 68 - "MoveNotesCommand"
Cohesion: 0.18
Nodes (11): EntityId, NoteIndices, Tick, MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, channel_, indices_ (+3 more)

### Community 69 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 70 - "SampleRingBuffer"
Cohesion: 0.21
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 71 - "Smoother"
Cohesion: 0.18
Nodes (10): atomic, AudioBufferView, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds (+2 more)

### Community 72 - "MixerStripNode"
Cohesion: 0.12
Nodes (11): ProcessContext, atomic, Node, Sample, MixerStripNode, left_, meter_, muted_ (+3 more)

### Community 73 - "compile"
Cohesion: 0.17
Nodes (16): process, AudioBufferView, FrameCount, FramePosition, MidiBuffer, Node, NodeIndex, SampleRate (+8 more)

### Community 74 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 75 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 76 - "ioProcTrampoline"
Cohesion: 0.23
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 77 - "-applicationDidFinishLaunching"
Cohesion: 0.18
Nodes (16): INCDAWAudioEditorView, INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, INCDAWPlaylistView, NSApplicationDelegate, NSObject (+8 more)

### Community 78 - "CoreMidiDevice"
Cohesion: 0.15
Nodes (14): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+6 more)

### Community 79 - "InputMonitorNode"
Cohesion: 0.15
Nodes (12): Node, FrameCount, Sample, SampleRate, SampleRingBuffer, size_t, vector, InputMonitorNode (+4 more)

### Community 80 - "ResizeNotesCommand"
Cohesion: 0.13
Nodes (13): Command, QuantizeNotesCommand, channel_, grid_, pattern_, previousEvents_, strength_, ResizeNotesCommand (+5 more)

### Community 81 - "Pattern"
Cohesion: 0.13
Nodes (15): size_t, Tick, vector, noteAtStep(), execute, Pattern, automationLanes, channels (+7 more)

### Community 82 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 83 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 84 - "EntityId"
Cohesion: 0.21
Nodes (7): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, Value

### Community 85 - "RoutingConnection"
Cohesion: 0.12
Nodes (14): EntityId, findRouting, ids_, metadata_, tempoMap_, RoutingConnection, destination, gain (+6 more)

### Community 86 - "MixerTests.cpp"
Cohesion: 0.17
Nodes (11): AudioBufferPool, AudioBufferView, ProcessContext, Sample, size_t, vector, channel, onsets() (+3 more)

### Community 87 - "TimingProbeInstrument"
Cohesion: 0.15
Nodes (9): Applied, AudioBufferView, FrameCount, MidiMessage, SampleRate, vector, TimingProbeInstrument, applied (+1 more)

### Community 88 - "GraphBuilder"
Cohesion: 0.14
Nodes (12): Connection, GraphBuilder, compensate_, connections_, error_, master_, nodes_, Node (+4 more)

### Community 89 - "vector"
Cohesion: 0.23
Nodes (4): vector, Command, string, vector

### Community 90 - "PatternCommands.cpp"
Cohesion: 0.20
Nodes (12): Command, Tick, SetPatternLengthCommand, canMergeWith, execute, length_, mergeWith, patternId_ (+4 more)

### Community 91 - "TrackCommands.cpp"
Cohesion: 0.17
Nodes (12): Command, execute, undo, undo, SetTrackHeightCommand, canMergeWith, execute, height_ (+4 more)

### Community 92 - "ProcessContext"
Cohesion: 0.14
Nodes (13): FramePosition, MidiBuffer, size_t, ProcessContext, frameCount, inputCount, inputs, liveMidi (+5 more)

### Community 93 - "PluginIdentifier"
Cohesion: 0.17
Nodes (11): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+3 more)

### Community 94 - "ResizeClipsCommand"
Cohesion: 0.19
Nodes (8): ClipIds, FrameCount, Tick, ResizeClipsCommand, clips_, lengthDelta_, previousFrameLengths_, previousLengths_

### Community 95 - "MoveClipsCommand"
Cohesion: 0.15
Nodes (13): MovedAudioClip, Command, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, canMergeWith, clips_, mergeWith (+5 more)

### Community 96 - "string"
Cohesion: 0.16
Nodes (8): AddChannelCommand, channel_, execute, index_, minted_, undo, size_t, string

### Community 97 - "ChannelCommands.cpp"
Cohesion: 0.19
Nodes (11): Command, undo, undo, SetChannelVolumeCommand, canMergeWith, channelId_, execute, mergeWith (+3 more)

### Community 98 - "EntityId"
Cohesion: 0.16
Nodes (11): ConnectMixerCommand, connection_, destination_, gain_, index_, isSend_, minted_, preFader_ (+3 more)

### Community 99 - "SetVelocityCommand"
Cohesion: 0.14
Nodes (8): string, SetVelocityCommand, channel_, indices_, pattern_, previousVelocities_, undo, velocity_

### Community 100 - "AddPatternCommand"
Cohesion: 0.16
Nodes (8): AddPatternCommand, execute, index_, minted_, pattern_, undo, size_t, string

### Community 101 - "DuplicatePatternCommand"
Cohesion: 0.16
Nodes (8): DuplicatePatternCommand, execute, index_, minted_, pattern_, source_, undo, EntityId

### Community 102 - "ToggleStepCommand"
Cohesion: 0.15
Nodes (9): Command, size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_ (+1 more)

### Community 103 - "process"
Cohesion: 0.40
Nodes (5): FrameCount, SampleRate, prepare, process, triggerClick

### Community 104 - "ParameterRegistry"
Cohesion: 0.20
Nodes (11): Entry, string, Entry, size_t, vector, ParameterRegistry, entries_, find (+3 more)

### Community 105 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 106 - "renderArrangement"
Cohesion: 0.19
Nodes (12): AudioFileData, FrameCount, path, Project, Sample, size_t, vector, makeAudio() (+4 more)

### Community 107 - "Fixture"
Cohesion: 0.16
Nodes (12): EntityId, SequencedNote, Tick, vector, Fixture, channel, pattern, project (+4 more)

### Community 108 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 109 - "AutomationNode"
Cohesion: 0.19
Nodes (7): Binding, AutomationNode, bindings_, tempoMap_, size_t, TempoMap, vector

### Community 110 - "RemoveMixerNodeCommand"
Cohesion: 0.15
Nodes (10): RemovedRouting, vector, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_ (+2 more)

### Community 111 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 112 - "EntityId"
Cohesion: 0.17
Nodes (6): EntityId, SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 113 - "AddPatternClipCommand"
Cohesion: 0.18
Nodes (10): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+2 more)

### Community 114 - "RenameTrackCommand"
Cohesion: 0.20
Nodes (6): Command, string, RenameTrackCommand, execute, previousName_, trackId_

### Community 115 - "Channel"
Cohesion: 0.15
Nodes (13): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+5 more)

### Community 116 - "Track"
Cohesion: 0.15
Nodes (13): findTrack, Track, colour, height, id, muted, name, outputMixerNode (+5 more)

### Community 117 - "main.mm"
Cohesion: 0.23
Nodes (12): -editorChanged, -openAudioAssetInEditor, -selectChannel, -selectPattern, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll (+4 more)

### Community 118 - "compileProjectGraph"
Cohesion: 0.23
Nodes (10): AutomationNode, Channel, Instrument, compileProjectGraph(), InstrumentFactory, defaultInstrumentFactory(), MixerStripNode, string (+2 more)

### Community 119 - "MidiRecorder"
Cohesion: 0.20
Nodes (10): CapturedMessage, atomic, queueCapacity, size_t, uint64_t, MidiRecorder, captured_, dropped_ (+2 more)

### Community 120 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 121 - "AddMixerNodeCommand"
Cohesion: 0.18
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 122 - "renderNode"
Cohesion: 0.23
Nodes (10): FrameCount, Node, Sample, size_t, vector, makeAudio(), renderNode(), ScratchDir (+2 more)

### Community 123 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 124 - "MidiTests.cpp"
Cohesion: 0.20
Nodes (10): MidiInput, FrameCount, MidiMessage, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote() (+2 more)

### Community 125 - "AutomationProbe"
Cohesion: 0.22
Nodes (8): ParameterRegistry, AutomationProbe, calls, registry, written, Project, vector, makeProject()

### Community 126 - "AddNoteCommand"
Cohesion: 0.20
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, size_t

### Community 127 - "EntityId"
Cohesion: 0.18
Nodes (6): EntityId, SetTrackSoloedCommand, execute, soloed_, trackId_, undo

### Community 128 - "DiskStreamer"
Cohesion: 0.24
Nodes (9): vector, DiskStreamer, mutex_, running_, serviceOnce, streams_, thread_, vector (+1 more)

### Community 129 - "captureAudioBlock"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 130 - "Node"
Cohesion: 0.20
Nodes (4): FrameCount, SampleRate, Node, process

### Community 131 - "MixerStripNode.cpp"
Cohesion: 0.31
Nodes (10): FrameCount, Sample, SampleRate, panGains, prepare, refreshTargets, setGain, setMuted (+2 more)

### Community 132 - "MixerFixture"
Cohesion: 0.18
Nodes (8): buildParallelPaths(), EntityId, TempoMap, unique_ptr, MixerFixture, pattern, project, tempo

### Community 133 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 134 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 135 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 136 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 137 - "string"
Cohesion: 0.24
Nodes (6): string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 138 - "DisconnectMixerCommand"
Cohesion: 0.20
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 139 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (9): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo (+1 more)

### Community 140 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 141 - "Fixture"
Cohesion: 0.20
Nodes (8): EntityId, Project, Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 142 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 143 - "RemoveChannelCommand"
Cohesion: 0.22
Nodes (8): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, execute, index_

### Community 144 - "DuplicateClipsCommand"
Cohesion: 0.22
Nodes (8): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_, Clip

### Community 145 - "SetMixerMutedCommand"
Cohesion: 0.25
Nodes (5): SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 146 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 147 - "SetMixerVolumeCommand"
Cohesion: 0.22
Nodes (6): SetMixerVolumeCommand, execute, nodeId_, previous_, undo, volume_

### Community 148 - "SetSendGainCommand"
Cohesion: 0.22
Nodes (6): SetSendGainCommand, connectionId_, execute, gain_, previous_, undo

### Community 149 - "RemovePatternCommand"
Cohesion: 0.22
Nodes (6): RemovePatternCommand, execute, index_, pattern_, patternId_, undo

### Community 150 - "availableDevices"
Cohesion: 0.50
Nodes (4): AudioDeviceInfo, availableDevices, vector, printDevices()

### Community 151 - "AutomationFixture"
Cohesion: 0.22
Nodes (7): AutomationFixture, channel, pattern, project, tempo, EntityId, TempoMap

### Community 152 - "grab"
Cohesion: 0.32
Nodes (7): AudioFileData, grab, log, prepare, FrameCount, SampleRate, size_t

### Community 153 - "Denormals.h"
Cohesion: 0.39
Nodes (5): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister()

### Community 154 - "Command"
Cohesion: 0.25
Nodes (5): Command, execute, id, name, undo

### Community 155 - "SetChannelMutedCommand"
Cohesion: 0.25
Nodes (5): SetChannelMutedCommand, channelId_, execute, muted_, undo

### Community 156 - "SetChannelStepKeyCommand"
Cohesion: 0.25
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 157 - "SetMixerPolarityCommand"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 158 - "Command"
Cohesion: 0.22
Nodes (6): Command, SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 159 - "SetPatternSwingCommand"
Cohesion: 0.25
Nodes (6): SetPatternSwingCommand, execute, patternId_, previousSwing_, swing_, undo

### Community 160 - "AddTrackCommand"
Cohesion: 0.25
Nodes (7): AddTrackCommand, execute, index_, minted_, track_, undo, size_t

### Community 161 - "RemoveTrackCommand"
Cohesion: 0.25
Nodes (7): RemovedClip, vector, RemoveTrackCommand, clips_, index_, track_, trackId_

### Community 162 - "SetTrackMutedCommand"
Cohesion: 0.25
Nodes (5): SetTrackMutedCommand, execute, muted_, trackId_, undo

### Community 163 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 164 - "TimestampedMidiMessage"
Cohesion: 0.25
Nodes (8): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status

### Community 165 - "MidiDevice.h"
Cohesion: 0.25
Nodes (6): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback

### Community 166 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 167 - "InstrumentTests.cpp"
Cohesion: 0.33
Nodes (6): SimpleSynth, InstrumentNode, AudioBufferPool, MidiBuffer, Sample, renderSynth()

### Community 168 - "RenameChannelCommand"
Cohesion: 0.29
Nodes (5): Command, RenameChannelCommand, channelId_, execute, previousName_

### Community 169 - "RenamePatternCommand"
Cohesion: 0.29
Nodes (6): Command, RenamePatternCommand, execute, patternId_, previousName_, undo

### Community 170 - "AudioEditorView.mm"
Cohesion: 0.29
Nodes (6): -acceptsFirstResponder, -hasSelection, -initWithFrameprojectregistry, -isFlipped, -selectionFrom, -selectionTo

### Community 171 - "PatternTests.cpp"
Cohesion: 0.48
Nodes (6): SequencedNote, Tick, vector, note(), shapeOf(), startsOf()

### Community 172 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 173 - "INCDAWMixerView"
Cohesion: 0.33
Nodes (5): incdaw, NSView, INCDAWMixerView, -initWithFrameprojectregistry, stripLookup

### Community 174 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 176 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 177 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 178 - "renderBlock"
Cohesion: 0.47
Nodes (5): FrameCount, Sample, vector, renderBlock(), tone()

### Community 179 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 180 - "start"
Cohesion: 0.40
Nodes (4): AudioDeviceConfig, deviceName, start, string

### Community 181 - "vector"
Cohesion: 0.40
Nodes (3): vector, FrameCount, logCounting()

### Community 182 - "collectForBlock"
Cohesion: 0.40
Nodes (5): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock

### Community 183 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 184 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 185 - "AutomationTests.cpp"
Cohesion: 0.60
Nodes (4): AutomationPoint, Tick, enginePoint(), modelPoint()

### Community 186 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 188 - "framesToSeconds"
Cohesion: 0.67
Nodes (4): framesToSeconds(), FrameCount, SampleRate, secondsToFrames()

### Community 189 - "INCDAWChannelRackView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWChannelRackView, -initWithFrameprojectregistry

### Community 190 - "INCDAWPatternListView"
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
- **844 isolated node(s):** `length`, `offset`, `startFrame`, `startsAfterLoopWrap`, `loopEnabled_` (+839 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **37 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `MixerFixture`, `AudioAsset`, `CommandRegistry`, `string`, `DisconnectMixerCommand`, `DeleteNotesCommand`, `MixerNode`, `RemoveChannelCommand`, `SetMixerMutedCommand`, `SetChannelOutputCommand`, `SetMixerVolumeCommand`, `SetSendGainCommand`, `RemovePatternCommand`, `PatternChannelContent`, `Clip`, `AutomationFixture`, `SetChannelMutedCommand`, `SetChannelStepKeyCommand`, `SetMixerPolarityCommand`, `Command`, `SetPatternSwingCommand`, `AddTrackCommand`, `ProjectFile.cpp`, `SetTrackMutedCommand`, `RenameChannelCommand`, `RenamePatternCommand`, `NoteCommands.cpp`, `compileArrangement`, `CountingCommand`, `MixerCommands.cpp`, `Pattern`, `RoutingConnection`, `PatternCommands.cpp`, `TrackCommands.cpp`, `string`, `ChannelCommands.cpp`, `EntityId`, `SetVelocityCommand`, `AddPatternCommand`, `DuplicatePatternCommand`, `Fixture`, `RemoveMixerNodeCommand`, `EntityId`, `RenameTrackCommand`, `Channel`, `Track`, `AddMixerNodeCommand`, `AutomationProbe`, `AddNoteCommand`, `EntityId`?**
  _High betweenness centrality (0.226) - this node is a cross-community bridge._