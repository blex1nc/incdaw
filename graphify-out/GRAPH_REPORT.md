# Graph Report - project-continuation-670d11  (2026-08-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 3201 nodes · 5452 edges · 232 communities (196 shown, 36 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 258 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `6f0558de`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- PianoRollModel
- Transport
- WavBytes.h
- CoreAudioDevice
- WavStreamWriter
- PlaylistModel
- SimpleSynth
- Project
- CommandRegistry
- ChannelRackModel
- AudioEngine
- Pattern
- RecordingSession
- Clip
- WavStreamReader
- AudioStream
- AudioDeviceConfig
- MixerView.mm
- ProjectFile.cpp
- CoreAudioDevice.cpp
- MidiEvent
- MidiMessage
- NoteSequence
- PlaylistView.mm
- AudioRecorder
- GraphCompileOptions
- Json
- AudioClipNode
- Model.h
- WriteAutomationCommand
- CallbackProfiler
- TimeSignature
- MetronomeNode
- AudioDevice
- main
- WaveformOverview
- AudioBufferPool
- CompiledGraph
- TimingProbeInstrument
- NoteCommands.cpp
- LevelMeter
- InsertRecordedTakeCommand
- compileArrangement
- -applicationDidFinishLaunching
- AudioEngine.cpp
- atomic
- AddAutomationLaneCommand
- DelayLineNode
- compile
- CompiledProjectGraph
- CountingCommand
- RealtimeGuard.cpp
- RoutingConnection
- Region
- ProcessContext
- BasicMidiBuffer
- MidiInput
- SystemInfo
- EditFixture
- WavFile
- TrimAssetCommand
- AutomationWriteSession
- MixerCommands.cpp
- MoveNotesCommand
- AudioFileData
- AudioBufferView
- InstrumentNode
- ConstantNode
- MidiRecorder
- ClipCommands.cpp
- LockFreeQueue
- SampleRingBuffer
- Smoother
- GainNode
- MixerStripNode
- Json.cpp
- LoopbackResult
- LatentProcessorNode
- ioProcTrampoline
- DuplicateClipsCommand
- CoreMidiDevice
- InputMonitorNode
- AutomationCommands.cpp
- SineOscillatorNode
- EntityId
- MixerTests.cpp
- GraphBuilder
- vector
- ChannelCommands.cpp
- PatternCommands.cpp
- TrackCommands.cpp
- Parser
- EditAssetRegionCommand
- string
- EntityId
- SetVelocityCommand
- AddPatternCommand
- DuplicatePatternCommand
- main.mm
- ToggleStepCommand
- process
- PluginIdentifier
- ParameterRegistry
- ChannelRackView.mm
- renderArrangement
- Fixture
- Options
- renderNode
- compileProjectGraph
- AutomationPoint
- AutomationNode
- MixerNode
- RemoveMixerNodeCommand
- TempoMap
- EntityId
- AddPatternClipCommand
- RenameTrackCommand
- EntityId
- Channel
- Track
- CoreMidiDevice.cpp
- AddMixerNodeCommand
- Node
- Instrument
- Denormals.h
- TimelineAnchor
- MidiTests.cpp
- captureAudioBlock
- MixerStripNode.cpp
- MixerFixture
- TimeSignatureEvent
- MidiDevice
- AudioAsset
- PatternListView.mm
- SetClipMutedCommand
- string
- DisconnectMixerCommand
- AddNoteCommand
- DeleteNotesCommand
- DiskStreamer
- RecordedEvent
- Fixture
- AutomationProbe
- make-dmg.sh
- MoveClipsCommand
- RemoveClipsCommand
- Command
- SetAutomationPointsCommand
- SetChannelStepKeyCommand
- SetMixerPolarityCommand
- SetChannelOutputCommand
- SetMixerVolumeCommand
- SetSendGainCommand
- QuantizeNotesCommand
- RemovePatternCommand
- INCDAWChannelRackView
- AutomationFixture
- RemoveChannelCommand
- AddChannelCommand
- ResizeClipsCommand
- Command
- SetMixerSoloedCommand
- ResizeNotesCommand
- SetPatternSwingCommand
- AddTrackCommand
- RemoveTrackCommand
- Version
- TimestampedMidiMessage
- MidiDevice.h
- SetChannelSoloedCommand
- RenamePatternCommand
- AudioEditorView.mm
- PatternTests.cpp
- ScratchDirectory
- INCDAWMixerView
- INCDAWPianoRollView
- string
- SetTrackMutedCommand
- collectForRange
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- renderBlock
- collectForBlock
- INCDAWAudioEditorView
- INCDAWPlaylistView
- AutomationTests.cpp
- check
- Project
- AudioCaptureSink
- framesToSeconds
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
2. `AudioEngine` - 65 edges
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

## Communities (232 total, 36 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "PianoRollModel"
Cohesion: 0.07
Nodes (39): NoteList, size_t, Tick, vector, size_t, Tick, Viewport, PianoRollModel (+31 more)

### Community 2 - "Transport"
Cohesion: 0.06
Nodes (38): atomic, MusicalPosition, BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount (+30 more)

### Community 3 - "WavBytes.h"
Cohesion: 0.09
Nodes (45): appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample, channels (+37 more)

### Community 4 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 5 - "WavStreamWriter"
Cohesion: 0.07
Nodes (36): ofstream, Format, FrameCount, path, Result, Sample, SampleRate, size_t (+28 more)

### Community 6 - "PlaylistModel"
Cohesion: 0.09
Nodes (35): Rect, Clip, EntityId, Project, size_t, Tick, Track, vector (+27 more)

### Community 7 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 8 - "Project"
Cohesion: 0.11
Nodes (39): IdGenerator, EntityId, size_t, vector, EntityId, operator==(), events, totalEventCount (+31 more)

### Community 9 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 10 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 11 - "AudioEngine"
Cohesion: 0.08
Nodes (25): AudioDevice, AudioIOCallback, CallbackProfiler, MidiBuffer, mutex, RetiredGraph, AudioCaptureSink, AudioEngine (+17 more)

### Community 12 - "Pattern"
Cohesion: 0.07
Nodes (28): AutomationCurve, size_t, Tick, vector, noteAtStep(), execute, undo, AutomationPoint (+20 more)

### Community 13 - "RecordingSession"
Cohesion: 0.09
Nodes (23): AudioEngine, AudioRecorder, path, Slice, Placement, string, vector, FrameCount (+15 more)

### Community 14 - "Clip"
Cohesion: 0.07
Nodes (29): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+21 more)

### Community 15 - "WavStreamReader"
Cohesion: 0.09
Nodes (24): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+16 more)

### Community 16 - "AudioStream"
Cohesion: 0.10
Nodes (24): shared_ptr, AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_ (+16 more)

### Community 17 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 18 - "MixerView.mm"
Cohesion: 0.17
Nodes (26): MixerStripNode, NSMenu, -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect (+18 more)

### Community 19 - "ProjectFile.cpp"
Cohesion: 0.20
Nodes (23): Json, automationPointFrom(), bindUnassignedContent(), AutomationPoint, EntityId, path, PluginIdentifier, Result (+15 more)

### Community 20 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 21 - "MidiEvent"
Cohesion: 0.11
Nodes (24): Kind, MidiEventType, RecordedEvent, appendRecordedEvents(), MidiEventType, Tick, uint64_t, vector (+16 more)

### Community 22 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 23 - "NoteSequence"
Cohesion: 0.11
Nodes (21): Tick, vector, size_t, Tick, uint32_t, vector, NoteSequence, byEnd_ (+13 more)

### Community 24 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (24): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+16 more)

### Community 25 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

### Community 26 - "GraphCompileOptions"
Cohesion: 0.09
Nodes (23): DiskStreamer, PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, instrumentFactory, masterGain, maxBlockSize (+15 more)

### Community 27 - "Json"
Cohesion: 0.10
Nodes (13): nullptr_t, pair, int64_t, string, vector, Json, boolean_, elements_ (+5 more)

### Community 28 - "AudioClipNode"
Cohesion: 0.10
Nodes (16): PlacedClip, ProcessContext, AudioClipNode, addClip, clips_, fetchScratch_, prepare, process (+8 more)

### Community 29 - "Model.h"
Cohesion: 0.10
Nodes (21): PluginIdentifier, AutomationLane, id, parameterKey, points, targetEntity, string, TempoMap (+13 more)

### Community 30 - "WriteAutomationCommand"
Cohesion: 0.11
Nodes (17): Clip, Track, WriteAutomationCommand, clip_, clipIndex_, key_, laneAfter_, laneCreated_ (+9 more)

### Community 31 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 32 - "TimeSignature"
Cohesion: 0.13
Nodes (16): Tick, friend, int64_t, Tick, MusicalPosition, bar, beat, tick (+8 more)

### Community 33 - "MetronomeNode"
Cohesion: 0.09
Nodes (17): atomic, FrameCount, Sample, SampleRate, size_t, vector, MetronomeNode, amplitude_ (+9 more)

### Community 34 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 35 - "main"
Cohesion: 0.11
Nodes (21): AudioDeviceConfig, AudioDeviceInfo, availableDevices, deviceName, midiInput_, profiler_, sampleRate, setGraph (+13 more)

### Community 36 - "WaveformOverview"
Cohesion: 0.12
Nodes (20): Result, SampleRate, bucketize(), AudioFileData, Bucket, FrameCount, Sample, vector (+12 more)

### Community 37 - "AudioBufferPool"
Cohesion: 0.13
Nodes (13): AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t, FrameCount (+5 more)

### Community 38 - "CompiledGraph"
Cohesion: 0.11
Nodes (15): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+7 more)

### Community 39 - "TimingProbeInstrument"
Cohesion: 0.11
Nodes (14): Applied, SimpleSynth, AudioBufferPool, AudioBufferView, FrameCount, MidiBuffer, MidiMessage, Sample (+6 more)

### Community 40 - "NoteCommands.cpp"
Cohesion: 0.17
Nodes (20): undo, Command, EntityId, NoteIndices, size_t, vector, execute, findEvents() (+12 more)

### Community 41 - "LevelMeter"
Cohesion: 0.12
Nodes (14): Node, atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_ (+6 more)

### Community 42 - "InsertRecordedTakeCommand"
Cohesion: 0.11
Nodes (16): Clip, EntityId, Placement, size_t, string, InsertRecordedTakeCommand, asset_, assetIndex_ (+8 more)

### Community 43 - "compileArrangement"
Cohesion: 0.23
Nodes (19): Emit, NoteSequence, content, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto() (+11 more)

### Community 44 - "-applicationDidFinishLaunching"
Cohesion: 0.18
Nodes (16): INCDAWAudioEditorView, INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, INCDAWPlaylistView, NSApplicationDelegate, NSObject (+8 more)

### Community 45 - "AudioEngine.cpp"
Cohesion: 0.17
Nodes (17): int64_t, audioDeviceAboutToStart, audioDeviceStopped, bufferSize, captureAudioBlock, collectRetiredGraphs, inputChannels, isRunning (+9 more)

### Community 46 - "atomic"
Cohesion: 0.15
Nodes (4): vector, atomic, MidiBuffer, array

### Community 47 - "AddAutomationLaneCommand"
Cohesion: 0.13
Nodes (13): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, AutomationLane, EntityId (+5 more)

### Community 48 - "DelayLineNode"
Cohesion: 0.12
Nodes (16): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+8 more)

### Community 49 - "compile"
Cohesion: 0.14
Nodes (19): process, AudioBufferView, FrameCount, FramePosition, MidiBuffer, Node, NodeIndex, SampleRate (+11 more)

### Community 50 - "CompiledProjectGraph"
Cohesion: 0.12
Nodes (20): CompiledProjectGraph, automation, channels, channelStripFor, channelStrips, error, graph, instrumentFor (+12 more)

### Community 51 - "CountingCommand"
Cohesion: 0.12
Nodes (10): CountingCommand, counter_, delta_, Command, EntityId, string, Tick, makeProjectWithNotes() (+2 more)

### Community 52 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 53 - "RoutingConnection"
Cohesion: 0.11
Nodes (10): friend, findRouting, RoutingConnection, destination, gain, id, isSend, preFader (+2 more)

### Community 54 - "Region"
Cohesion: 0.30
Nodes (16): applyGain(), applyRamp(), clampedRegion(), AudioFileData, Sample, fadeIn(), fadeOut(), FrameCount (+8 more)

### Community 55 - "ProcessContext"
Cohesion: 0.12
Nodes (15): FrameCount, FramePosition, MidiBuffer, SampleRate, size_t, ProcessContext, frameCount, inputCount (+7 more)

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

### Community 61 - "TrimAssetCommand"
Cohesion: 0.13
Nodes (13): Command, FrameCount, Sample, string, vector, TrimAssetCommand, applied_, asset_ (+5 more)

### Community 62 - "AutomationWriteSession"
Cohesion: 0.13
Nodes (14): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, Command, EntityId (+6 more)

### Community 63 - "MixerCommands.cpp"
Cohesion: 0.16
Nodes (14): execute, Command, SetMixerPanCommand, canMergeWith, execute, mergeWith, nodeId_, pan_ (+6 more)

### Community 64 - "MoveNotesCommand"
Cohesion: 0.16
Nodes (11): EntityId, NoteIndices, Tick, MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, channel_, indices_ (+3 more)

### Community 65 - "AudioFileData"
Cohesion: 0.11
Nodes (16): AudioFileData, channelCount, channels, frameCount, sampleRate, FrameCount, Sample, SampleRate (+8 more)

### Community 66 - "AudioBufferView"
Cohesion: 0.22
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 67 - "InstrumentNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, unique_ptr, InstrumentNode, blockMidi_ (+6 more)

### Community 68 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 69 - "MidiRecorder"
Cohesion: 0.14
Nodes (14): CapturedMessage, FramePosition, MidiBuffer, atomic, queueCapacity, size_t, uint64_t, MidiRecorder (+6 more)

### Community 70 - "ClipCommands.cpp"
Cohesion: 0.23
Nodes (15): execute, undo, Command, EntityId, Project, execute, undo, canMergeWith (+7 more)

### Community 71 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 72 - "SampleRingBuffer"
Cohesion: 0.21
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 73 - "Smoother"
Cohesion: 0.18
Nodes (10): atomic, AudioBufferView, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds (+2 more)

### Community 74 - "GainNode"
Cohesion: 0.15
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 75 - "MixerStripNode"
Cohesion: 0.12
Nodes (11): ProcessContext, atomic, Node, Sample, MixerStripNode, left_, meter_, muted_ (+3 more)

### Community 76 - "Json.cpp"
Cohesion: 0.18
Nodes (15): int64_t, size_t, string, escapeInto(), formatDouble(), append, asBool, asDouble (+7 more)

### Community 77 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 78 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 79 - "ioProcTrampoline"
Cohesion: 0.23
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 80 - "DuplicateClipsCommand"
Cohesion: 0.16
Nodes (10): ClipIds, DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_ (+2 more)

### Community 81 - "CoreMidiDevice"
Cohesion: 0.15
Nodes (14): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+6 more)

### Community 82 - "InputMonitorNode"
Cohesion: 0.15
Nodes (12): Node, FrameCount, Sample, SampleRate, SampleRingBuffer, size_t, vector, InputMonitorNode (+4 more)

### Community 83 - "AutomationCommands.cpp"
Cohesion: 0.22
Nodes (15): execute, undo, AutomationLane, AutomationPoint, EntityId, Project, vector, findLane() (+7 more)

### Community 84 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 85 - "EntityId"
Cohesion: 0.21
Nodes (7): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, Value

### Community 86 - "MixerTests.cpp"
Cohesion: 0.17
Nodes (11): AudioBufferPool, AudioBufferView, ProcessContext, Sample, size_t, vector, channel, onsets() (+3 more)

### Community 87 - "GraphBuilder"
Cohesion: 0.14
Nodes (12): Connection, GraphBuilder, compensate_, connections_, error_, master_, nodes_, Node (+4 more)

### Community 88 - "vector"
Cohesion: 0.22
Nodes (4): vector, Command, string, vector

### Community 89 - "ChannelCommands.cpp"
Cohesion: 0.17
Nodes (12): execute, Command, execute, undo, SetChannelVolumeCommand, canMergeWith, channelId_, execute (+4 more)

### Community 90 - "PatternCommands.cpp"
Cohesion: 0.20
Nodes (12): Command, Tick, SetPatternLengthCommand, canMergeWith, execute, length_, mergeWith, patternId_ (+4 more)

### Community 91 - "TrackCommands.cpp"
Cohesion: 0.17
Nodes (12): Command, execute, undo, undo, SetTrackHeightCommand, canMergeWith, execute, height_ (+4 more)

### Community 92 - "Parser"
Cohesion: 0.30
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 93 - "EditAssetRegionCommand"
Cohesion: 0.16
Nodes (11): AudioEditOp, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_, minted_ (+3 more)

### Community 94 - "string"
Cohesion: 0.16
Nodes (6): string, RenameChannelCommand, channelId_, execute, previousName_, undo

### Community 95 - "EntityId"
Cohesion: 0.16
Nodes (11): ConnectMixerCommand, connection_, destination_, gain_, index_, isSend_, minted_, preFader_ (+3 more)

### Community 96 - "SetVelocityCommand"
Cohesion: 0.14
Nodes (8): string, SetVelocityCommand, channel_, indices_, pattern_, previousVelocities_, undo, velocity_

### Community 97 - "AddPatternCommand"
Cohesion: 0.16
Nodes (8): AddPatternCommand, execute, index_, minted_, pattern_, undo, size_t, string

### Community 98 - "DuplicatePatternCommand"
Cohesion: 0.16
Nodes (8): DuplicatePatternCommand, execute, index_, minted_, pattern_, source_, undo, EntityId

### Community 99 - "main.mm"
Cohesion: 0.21
Nodes (13): vector, -editorChanged, -openAudioAssetInEditor, -selectChannel, -selectPattern, -showAudioEditor, -showEditorAtSegment, -showMixer (+5 more)

### Community 100 - "ToggleStepCommand"
Cohesion: 0.15
Nodes (9): Command, size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_ (+1 more)

### Community 101 - "process"
Cohesion: 0.40
Nodes (5): FrameCount, SampleRate, prepare, process, triggerClick

### Community 102 - "PluginIdentifier"
Cohesion: 0.19
Nodes (11): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+3 more)

### Community 103 - "ParameterRegistry"
Cohesion: 0.20
Nodes (11): Entry, string, Entry, size_t, vector, ParameterRegistry, entries_, find (+3 more)

### Community 104 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 105 - "renderArrangement"
Cohesion: 0.19
Nodes (12): AudioFileData, FrameCount, path, Project, Sample, size_t, vector, makeAudio() (+4 more)

### Community 106 - "Fixture"
Cohesion: 0.16
Nodes (12): EntityId, SequencedNote, Tick, vector, Fixture, channel, pattern, project (+4 more)

### Community 107 - "Options"
Cohesion: 0.14
Nodes (14): int64_t, Options, amplitude, buffer, device, frequency, input, listOnly (+6 more)

### Community 108 - "renderNode"
Cohesion: 0.21
Nodes (11): AudioFileData, FrameCount, Node, Sample, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 109 - "compileProjectGraph"
Cohesion: 0.21
Nodes (11): AutomationNode, Channel, Instrument, ParameterRegistry, compileProjectGraph(), InstrumentFactory, defaultInstrumentFactory(), MixerStripNode (+3 more)

### Community 110 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 111 - "AutomationNode"
Cohesion: 0.19
Nodes (7): Binding, AutomationNode, bindings_, tempoMap_, size_t, TempoMap, vector

### Community 112 - "MixerNode"
Cohesion: 0.15
Nodes (13): MixerNodeType, MixerNode, colour, id, inserts, muted, name, pan (+5 more)

### Community 113 - "RemoveMixerNodeCommand"
Cohesion: 0.15
Nodes (10): RemovedRouting, vector, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_ (+2 more)

### Community 114 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 115 - "EntityId"
Cohesion: 0.17
Nodes (6): EntityId, SetChannelMutedCommand, channelId_, execute, muted_, undo

### Community 116 - "AddPatternClipCommand"
Cohesion: 0.18
Nodes (10): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+2 more)

### Community 117 - "RenameTrackCommand"
Cohesion: 0.20
Nodes (6): Command, string, RenameTrackCommand, execute, previousName_, trackId_

### Community 118 - "EntityId"
Cohesion: 0.18
Nodes (6): EntityId, SetTrackSoloedCommand, execute, soloed_, trackId_, undo

### Community 119 - "Channel"
Cohesion: 0.15
Nodes (13): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+5 more)

### Community 120 - "Track"
Cohesion: 0.15
Nodes (13): findTrack, Track, colour, height, id, muted, name, outputMixerNode (+5 more)

### Community 121 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 122 - "AddMixerNodeCommand"
Cohesion: 0.18
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 123 - "Node"
Cohesion: 0.18
Nodes (6): Node, process, vector, OrderRecordingNode, identifier_, log_

### Community 124 - "Instrument"
Cohesion: 0.18
Nodes (9): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+1 more)

### Community 125 - "Denormals.h"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 126 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 127 - "MidiTests.cpp"
Cohesion: 0.20
Nodes (10): MidiInput, FrameCount, MidiMessage, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote() (+2 more)

### Community 128 - "captureAudioBlock"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 129 - "MixerStripNode.cpp"
Cohesion: 0.31
Nodes (10): FrameCount, Sample, SampleRate, panGains, prepare, refreshTargets, setGain, setMuted (+2 more)

### Community 130 - "MixerFixture"
Cohesion: 0.18
Nodes (8): buildParallelPaths(), EntityId, TempoMap, unique_ptr, MixerFixture, pattern, project, tempo

### Community 131 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 132 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 133 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 134 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 135 - "SetClipMutedCommand"
Cohesion: 0.24
Nodes (8): Command, vector, SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 136 - "string"
Cohesion: 0.24
Nodes (6): string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 137 - "DisconnectMixerCommand"
Cohesion: 0.20
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 138 - "AddNoteCommand"
Cohesion: 0.22
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, Command, size_t

### Community 139 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (8): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo

### Community 140 - "DiskStreamer"
Cohesion: 0.27
Nodes (8): DiskStreamer, mutex_, running_, serviceOnce, streams_, thread_, vector, weak_ptr

### Community 141 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 142 - "Fixture"
Cohesion: 0.20
Nodes (8): EntityId, Project, Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 143 - "AutomationProbe"
Cohesion: 0.24
Nodes (7): AutomationProbe, calls, registry, written, Project, vector, makeProject()

### Community 144 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 145 - "MoveClipsCommand"
Cohesion: 0.22
Nodes (8): MovedAudioClip, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, movedAudio_, tickDelta_, trackDelta_

### Community 146 - "RemoveClipsCommand"
Cohesion: 0.22
Nodes (8): RemovedClip, string, RemoveClipsCommand, clips_, execute, name, removed_, undo

### Community 147 - "Command"
Cohesion: 0.22
Nodes (5): Command, execute, id, name, undo

### Community 148 - "SetAutomationPointsCommand"
Cohesion: 0.21
Nodes (9): Command, AutomationPoint, vector, SetAutomationPointsCommand, canMergeWith, laneId_, mergeWith, points_ (+1 more)

### Community 149 - "SetChannelStepKeyCommand"
Cohesion: 0.22
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 150 - "SetMixerPolarityCommand"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 151 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 152 - "SetMixerVolumeCommand"
Cohesion: 0.22
Nodes (6): SetMixerVolumeCommand, execute, nodeId_, previous_, undo, volume_

### Community 153 - "SetSendGainCommand"
Cohesion: 0.22
Nodes (6): SetSendGainCommand, connectionId_, execute, gain_, previous_, undo

### Community 154 - "QuantizeNotesCommand"
Cohesion: 0.22
Nodes (8): QuantizeNotesCommand, channel_, execute, grid_, pattern_, previousEvents_, strength_, undo

### Community 155 - "RemovePatternCommand"
Cohesion: 0.22
Nodes (6): RemovePatternCommand, execute, index_, pattern_, patternId_, undo

### Community 156 - "INCDAWChannelRackView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWChannelRackView, -initWithFrameprojectregistry

### Community 157 - "AutomationFixture"
Cohesion: 0.22
Nodes (7): AutomationFixture, channel, pattern, project, tempo, EntityId, TempoMap

### Community 158 - "RemoveChannelCommand"
Cohesion: 0.25
Nodes (7): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, index_

### Community 159 - "AddChannelCommand"
Cohesion: 0.25
Nodes (6): AddChannelCommand, channel_, index_, minted_, undo, size_t

### Community 160 - "ResizeClipsCommand"
Cohesion: 0.25
Nodes (7): FrameCount, ResizeClipsCommand, clips_, execute, lengthDelta_, previousFrameLengths_, previousLengths_

### Community 161 - "Command"
Cohesion: 0.22
Nodes (6): Command, SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 162 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 163 - "ResizeNotesCommand"
Cohesion: 0.25
Nodes (7): vector, ResizeNotesCommand, channel_, durationDelta_, indices_, pattern_, previousDurations_

### Community 164 - "SetPatternSwingCommand"
Cohesion: 0.25
Nodes (6): SetPatternSwingCommand, execute, patternId_, previousSwing_, swing_, undo

### Community 165 - "AddTrackCommand"
Cohesion: 0.25
Nodes (7): AddTrackCommand, execute, index_, minted_, track_, undo, size_t

### Community 166 - "RemoveTrackCommand"
Cohesion: 0.25
Nodes (7): RemovedClip, vector, RemoveTrackCommand, clips_, index_, track_, trackId_

### Community 167 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 168 - "TimestampedMidiMessage"
Cohesion: 0.25
Nodes (8): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status

### Community 169 - "MidiDevice.h"
Cohesion: 0.25
Nodes (6): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback

### Community 170 - "SetChannelSoloedCommand"
Cohesion: 0.29
Nodes (6): Command, SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 171 - "RenamePatternCommand"
Cohesion: 0.29
Nodes (6): Command, RenamePatternCommand, execute, patternId_, previousName_, undo

### Community 172 - "AudioEditorView.mm"
Cohesion: 0.29
Nodes (6): -acceptsFirstResponder, -hasSelection, -initWithFrameprojectregistry, -isFlipped, -selectionFrom, -selectionTo

### Community 173 - "PatternTests.cpp"
Cohesion: 0.48
Nodes (6): SequencedNote, Tick, vector, note(), shapeOf(), startsOf()

### Community 174 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 175 - "INCDAWMixerView"
Cohesion: 0.33
Nodes (5): incdaw, NSView, INCDAWMixerView, -initWithFrameprojectregistry, stripLookup

### Community 176 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 178 - "SetTrackMutedCommand"
Cohesion: 0.25
Nodes (5): SetTrackMutedCommand, execute, muted_, trackId_, undo

### Community 179 - "collectForRange"
Cohesion: 0.33
Nodes (4): FrameCount, FramePosition, MidiBuffer, collectForRange

### Community 180 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 181 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 182 - "renderBlock"
Cohesion: 0.47
Nodes (5): FrameCount, Sample, vector, renderBlock(), tone()

### Community 183 - "collectForBlock"
Cohesion: 0.40
Nodes (5): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock

### Community 184 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 185 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 186 - "AutomationTests.cpp"
Cohesion: 0.60
Nodes (4): AutomationPoint, Tick, enginePoint(), modelPoint()

### Community 187 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 188 - "Project"
Cohesion: 0.67
Nodes (3): Project, execute, undo

### Community 190 - "framesToSeconds"
Cohesion: 0.67
Nodes (4): framesToSeconds(), FrameCount, SampleRate, secondsToFrames()

### Community 191 - "INCDAWPatternListView"
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
- **837 isolated node(s):** `noNote`, `resizeHandleWidth`, `selection_`, `viewport_`, `-acceptsFirstResponder` (+832 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **36 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `MixerFixture`, `AudioAsset`, `string`, `CommandRegistry`, `DisconnectMixerCommand`, `AddNoteCommand`, `DeleteNotesCommand`, `Pattern`, `Clip`, `AutomationProbe`, `ProjectFile.cpp`, `SetChannelStepKeyCommand`, `SetMixerPolarityCommand`, `SetChannelOutputCommand`, `SetMixerVolumeCommand`, `SetSendGainCommand`, `QuantizeNotesCommand`, `RemovePatternCommand`, `Model.h`, `AutomationFixture`, `AddChannelCommand`, `Command`, `SetMixerSoloedCommand`, `SetPatternSwingCommand`, `AddTrackCommand`, `NoteCommands.cpp`, `SetChannelSoloedCommand`, `RenamePatternCommand`, `compileArrangement`, `SetTrackMutedCommand`, `CountingCommand`, `RoutingConnection`, `MixerCommands.cpp`, `ChannelCommands.cpp`, `PatternCommands.cpp`, `TrackCommands.cpp`, `string`, `EntityId`, `SetVelocityCommand`, `AddPatternCommand`, `DuplicatePatternCommand`, `Fixture`, `MixerNode`, `RemoveMixerNodeCommand`, `EntityId`, `RenameTrackCommand`, `EntityId`, `Channel`, `Track`, `AddMixerNodeCommand`?**
  _High betweenness centrality (0.255) - this node is a cross-community bridge._