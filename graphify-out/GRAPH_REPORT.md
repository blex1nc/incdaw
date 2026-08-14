# Graph Report - project-continuation-670d11  (2026-08-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 3090 nodes · 5301 edges · 213 communities (178 shown, 35 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 250 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `0ed54641`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- RemoveTrackCommand
- PianoRollModel
- WavBytes.h
- MixerNode
- CoreAudioDevice
- PlaylistModel
- AudioBufferPool
- SimpleSynth
- MidiEvent
- Project
- Transport
- CommandRegistry
- ChannelRackModel
- EditAssetRegionCommand
- TempoMap
- InstrumentNode
- Clip
- WavStreamReader
- MetronomeNode
- AudioDeviceConfig
- AudioStream
- MusicalPosition
- AudioEngine
- CoreAudioDevice.cpp
- ProjectFile.cpp
- PatternCommands.cpp
- MixerView.mm
- string
- MidiMessage
- NoteSequence
- AudioRecorder
- PlaylistView.mm
- Json
- SetAutomationPointsCommand
- AddPatternCommand
- atomic
- CallbackProfiler
- AudioDevice
- Pattern
- WaveformOverview
- GraphBuilder
- ResizeClipsCommand
- NoteCommands.cpp
- InsertRecordedTakeCommand
- Options
- DelayLineNode
- CompiledGraph
- CountingCommand
- RealtimeGuard.cpp
- compileArrangement
- GraphCompileOptions
- Region
- AudioEngine.cpp
- AudioBufferView
- ProcessContext
- MidiInput
- SystemInfo
- EditFixture
- WavFile
- RecordingSession
- AudioClipNode
- MixerCommands.cpp
- MoveNotesCommand
- DuplicatePatternCommand
- LevelMeter
- compile
- BasicMidiBuffer
- ConstantNode
- MidiRecorder
- ClipCommands.cpp
- LockFreeQueue
- SampleRingBuffer
- Smoother
- MixerStripNode
- Parser
- LoopbackResult
- LatentProcessorNode
- ioProcTrampoline
- compileProjectGraph
- DuplicateClipsCommand
- CoreMidiDevice
- WavStreamWriter
- vector
- GainNode
- SineOscillatorNode
- EntityId
- RoutingConnection
- MixerTests.cpp
- TimingProbeInstrument
- main
- -applicationDidFinishLaunching
- string
- ChannelCommands.cpp
- open
- PluginIdentifier
- Json.cpp
- CompiledProjectGraph
- MoveClipsCommand
- vector
- string
- EntityId
- ToggleStepCommand
- ParameterRegistry
- ChannelRackView.mm
- renderArrangement
- Fixture
- DiskStreamer
- AutomationPoint
- AutomationNode
- EntityId
- AddPatternClipCommand
- ResizeNotesCommand
- main.mm
- CoreMidiDevice.cpp
- humanizeNoteStarts
- RemoveMixerNodeCommand
- Track
- WavStreamWriterTests.cpp
- Denormals.h
- AddMixerNodeCommand
- captureAudioBlock
- AudioFileData
- TimelineAnchor
- MixerStripNode.cpp
- TimeSignatureEvent
- BlockSegment
- MidiDevice
- AudioAsset
- PatternListView.mm
- DisconnectMixerCommand
- AddNoteCommand
- DeleteNotesCommand
- RecordedEvent
- Fixture
- MidiTests.cpp
- make-dmg.sh
- RemoveChannelCommand
- SetChannelStepKeyCommand
- SetChannelOutputCommand
- SetMixerPanCommand
- SetMixerVolumeCommand
- QuantizeNotesCommand
- ProjectGraphCompiler.h
- MixerCommands.h
- SetMixerPolarityCommand
- SetMixerSoloedCommand
- SetVelocityCommand
- Version
- TimestampedMidiMessage
- MidiDevice.h
- makeTestSignal
- RemoveClipsCommand
- InstrumentTests.cpp
- SetChannelSoloedCommand
- AudioEditorView.mm
- MixerFixture
- PatternTests.cpp
- ScratchDirectory
- INCDAWMixerView
- INCDAWPianoRollView
- RenameChannelCommand
- collectForRange
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- ChannelRackTests.cpp
- OrderRecordingNode
- collectForBlock
- INCDAWAudioEditorView
- INCDAWPlaylistView
- check
- AudioCaptureSink
- INCDAWChannelRackView
- INCDAWPatternListView
- CallbackProfiler
- FrameCount
- FramePosition
- INCDAWPlaylistView
- Node
- NSMenu
- NSView
- Sample
- AutomationLane
- Project
- AutomationLane
- RemovedClip
- Rect
- PlacedClip
- T
- Kind
- friend
- MidiEventType
- MixerNodeType
- PluginIdentifier
- uint32_t
- InstrumentFactory
- InstrumentNode
- MixerStripNode
- TempoMap
- CompiledGraph
- string
- unique_ptr
- Step
- Project
- Tick
- uint64_t
- uint8_t
- Viewport

## God Nodes (most connected - your core abstractions)
1. `Project` - 154 edges
2. `AudioEngine` - 60 edges
3. `CoreAudioDevice` - 59 edges
4. `Json` - 44 edges
5. `TempoMap` - 40 edges
6. `Transport` - 37 edges
7. `Clip` - 37 edges
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

## Communities (213 total, 35 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "RemoveTrackCommand"
Cohesion: 0.05
Nodes (43): AddTrackCommand, execute, index_, minted_, track_, undo, Command, Command (+35 more)

### Community 2 - "PianoRollModel"
Cohesion: 0.08
Nodes (34): NoteList, size_t, Tick, vector, size_t, Tick, Viewport, PianoRollModel (+26 more)

### Community 3 - "WavBytes.h"
Cohesion: 0.09
Nodes (45): appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample, channels (+37 more)

### Community 4 - "MixerNode"
Cohesion: 0.05
Nodes (43): MixerNodeType, PluginIdentifier, vector, Channel, colour, id, instrument, instrumentStateFile (+35 more)

### Community 5 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 6 - "PlaylistModel"
Cohesion: 0.09
Nodes (35): Rect, Clip, EntityId, Project, size_t, Tick, vector, EntityId (+27 more)

### Community 7 - "AudioBufferPool"
Cohesion: 0.06
Nodes (30): AudioFileData, AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t (+22 more)

### Community 8 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 9 - "MidiEvent"
Cohesion: 0.05
Nodes (31): AutomationCurve, friend, MidiEventType, AutomationLane, id, parameterKey, points, targetEntity (+23 more)

### Community 10 - "Project"
Cohesion: 0.13
Nodes (34): IdGenerator, EntityId, size_t, vector, operator==(), totalEventCount, Project, audioAssets_ (+26 more)

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

### Community 15 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 16 - "InstrumentNode"
Cohesion: 0.08
Nodes (23): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+15 more)

### Community 17 - "Clip"
Cohesion: 0.07
Nodes (29): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+21 more)

### Community 18 - "WavStreamReader"
Cohesion: 0.09
Nodes (24): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+16 more)

### Community 19 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 20 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 21 - "AudioStream"
Cohesion: 0.11
Nodes (22): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+14 more)

### Community 22 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 23 - "AudioEngine"
Cohesion: 0.10
Nodes (23): AudioDevice, AudioIOCallback, MidiBuffer, MidiInput, mutex, RetiredGraph, AudioCaptureSink, AudioEngine (+15 more)

### Community 24 - "CoreAudioDevice.cpp"
Cohesion: 0.29
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 25 - "ProjectFile.cpp"
Cohesion: 0.20
Nodes (23): Json, automationPointFrom(), bindUnassignedContent(), AutomationPoint, EntityId, path, PluginIdentifier, Result (+15 more)

### Community 26 - "PatternCommands.cpp"
Cohesion: 0.11
Nodes (21): Command, execute, undo, Command, Tick, SetPatternLengthCommand, canMergeWith, execute (+13 more)

### Community 27 - "MixerView.mm"
Cohesion: 0.18
Nodes (25): -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect, -drawStripnode, -faderRectAt (+17 more)

### Community 28 - "string"
Cohesion: 0.12
Nodes (14): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, Command, EntityId (+6 more)

### Community 29 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 30 - "NoteSequence"
Cohesion: 0.11
Nodes (21): Tick, vector, size_t, Tick, uint32_t, vector, NoteSequence, byEnd_ (+13 more)

### Community 31 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

### Community 32 - "PlaylistView.mm"
Cohesion: 0.14
Nodes (23): -acceptsFirstResponder, -addTrackRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler, -drawTracks (+15 more)

### Community 33 - "Json"
Cohesion: 0.10
Nodes (15): nullptr_t, pair, int64_t, int64_t, string, vector, Json, asInt (+7 more)

### Community 34 - "SetAutomationPointsCommand"
Cohesion: 0.12
Nodes (20): execute, undo, AutomationPoint, Command, EntityId, vector, findLane(), AutomationPoint (+12 more)

### Community 35 - "AddPatternCommand"
Cohesion: 0.10
Nodes (12): AddPatternCommand, execute, index_, minted_, pattern_, undo, string, RenamePatternCommand (+4 more)

### Community 36 - "atomic"
Cohesion: 0.13
Nodes (5): atomic, MidiBuffer, array, Node, process

### Community 37 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 38 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 39 - "Pattern"
Cohesion: 0.13
Nodes (18): size_t, Tick, vector, noteAtStep(), execute, undo, Pattern, automationLanes (+10 more)

### Community 40 - "WaveformOverview"
Cohesion: 0.12
Nodes (20): Result, SampleRate, bucketize(), AudioFileData, Bucket, FrameCount, Sample, vector (+12 more)

### Community 41 - "GraphBuilder"
Cohesion: 0.11
Nodes (16): Connection, NodeIndex, GraphBuilder, compensate_, connect, connections_, error_, master_ (+8 more)

### Community 42 - "ResizeClipsCommand"
Cohesion: 0.12
Nodes (13): Command, FrameCount, string, vector, ResizeClipsCommand, clips_, lengthDelta_, previousFrameLengths_ (+5 more)

### Community 43 - "NoteCommands.cpp"
Cohesion: 0.17
Nodes (20): Command, EntityId, NoteIndices, size_t, vector, execute, findEvents(), canMergeWith (+12 more)

### Community 44 - "InsertRecordedTakeCommand"
Cohesion: 0.11
Nodes (16): Command, EntityId, Placement, size_t, InsertRecordedTakeCommand, asset_, assetIndex_, clip_ (+8 more)

### Community 45 - "Options"
Cohesion: 0.11
Nodes (19): AudioDeviceInfo, availableDevices, vector, int64_t, Options, amplitude, buffer, device (+11 more)

### Community 46 - "DelayLineNode"
Cohesion: 0.12
Nodes (16): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+8 more)

### Community 47 - "CompiledGraph"
Cohesion: 0.12
Nodes (14): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+6 more)

### Community 48 - "CountingCommand"
Cohesion: 0.12
Nodes (10): CountingCommand, counter_, delta_, Command, EntityId, string, Tick, makeProjectWithNotes() (+2 more)

### Community 49 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 50 - "compileArrangement"
Cohesion: 0.25
Nodes (18): Emit, NoteSequence, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto(), EntityId (+10 more)

### Community 51 - "GraphCompileOptions"
Cohesion: 0.11
Nodes (19): PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, instrumentFactory, masterGain, maxBlockSize, parameters (+11 more)

### Community 52 - "Region"
Cohesion: 0.30
Nodes (16): applyGain(), applyRamp(), clampedRegion(), AudioFileData, Sample, fadeIn(), fadeOut(), FrameCount (+8 more)

### Community 53 - "AudioEngine.cpp"
Cohesion: 0.18
Nodes (17): audioDeviceAboutToStart, audioDeviceStopped, bufferSize, captureAudioBlock, collectRetiredGraphs, inputChannels, isRunning, maxServiceableBlockSize (+9 more)

### Community 54 - "AudioBufferView"
Cohesion: 0.20
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 55 - "ProcessContext"
Cohesion: 0.12
Nodes (15): FrameCount, FramePosition, MidiBuffer, SampleRate, size_t, ProcessContext, frameCount, inputCount (+7 more)

### Community 56 - "MidiInput"
Cohesion: 0.14
Nodes (14): atomic, queueCapacity, size_t, uint64_t, MidiInput, dropped_, hasPending_, late_ (+6 more)

### Community 57 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 58 - "EditFixture"
Cohesion: 0.14
Nodes (15): AudioFileData, EntityId, FrameCount, Project, Sample, size_t, EditFixture, assetId (+7 more)

### Community 59 - "WavFile"
Cohesion: 0.25
Nodes (17): AudioAsset, assetFilePath(), AudioFileData, EntityId, Project, Sample, string, vector (+9 more)

### Community 60 - "RecordingSession"
Cohesion: 0.14
Nodes (12): AudioRecorder, string, Placement, string, FrameCount, uint64_t, RecordingSession, arm (+4 more)

### Community 61 - "AudioClipNode"
Cohesion: 0.13
Nodes (14): PlacedClip, ProcessContext, AudioClipNode, addClip, clips_, fetchScratch_, prepare, process (+6 more)

### Community 62 - "MixerCommands.cpp"
Cohesion: 0.16
Nodes (14): undo, Command, canMergeWith, mergeWith, canMergeWith, mergeWith, SetSendGainCommand, canMergeWith (+6 more)

### Community 63 - "MoveNotesCommand"
Cohesion: 0.16
Nodes (11): EntityId, NoteIndices, Tick, MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, channel_, indices_ (+3 more)

### Community 64 - "DuplicatePatternCommand"
Cohesion: 0.14
Nodes (13): DuplicatePatternCommand, index_, minted_, pattern_, source_, EntityId, size_t, RemovePatternCommand (+5 more)

### Community 65 - "LevelMeter"
Cohesion: 0.14
Nodes (13): atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond (+5 more)

### Community 66 - "compile"
Cohesion: 0.16
Nodes (17): process, AudioBufferView, FrameCount, FramePosition, MidiBuffer, Node, SampleRate, size_t (+9 more)

### Community 67 - "BasicMidiBuffer"
Cohesion: 0.13
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 68 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 69 - "MidiRecorder"
Cohesion: 0.14
Nodes (14): CapturedMessage, FramePosition, MidiBuffer, atomic, queueCapacity, size_t, uint64_t, MidiRecorder (+6 more)

### Community 70 - "ClipCommands.cpp"
Cohesion: 0.24
Nodes (15): execute, undo, EntityId, Project, execute, undo, execute, undo (+7 more)

### Community 71 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 72 - "SampleRingBuffer"
Cohesion: 0.21
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 73 - "Smoother"
Cohesion: 0.18
Nodes (10): atomic, AudioBufferView, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds (+2 more)

### Community 74 - "MixerStripNode"
Cohesion: 0.12
Nodes (11): ProcessContext, atomic, Node, Sample, MixerStripNode, left_, meter_, muted_ (+3 more)

### Community 75 - "Parser"
Cohesion: 0.25
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 76 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 77 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 78 - "ioProcTrampoline"
Cohesion: 0.23
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 79 - "compileProjectGraph"
Cohesion: 0.17
Nodes (15): Channel, CompiledProjectGraph, GraphCompileOptions, InstrumentFactory, InstrumentNode, MixerStripNode, channelStripFor, instrumentFor (+7 more)

### Community 80 - "DuplicateClipsCommand"
Cohesion: 0.16
Nodes (10): ClipIds, DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_ (+2 more)

### Community 81 - "CoreMidiDevice"
Cohesion: 0.15
Nodes (14): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+6 more)

### Community 82 - "WavStreamWriter"
Cohesion: 0.13
Nodes (14): ofstream, Format, FrameCount, path, size_t, uint8_t, vector, WavStreamWriter (+6 more)

### Community 83 - "vector"
Cohesion: 0.18
Nodes (7): vector, Node, AudioBufferPool, AutomationPoint, Tick, enginePoint(), modelPoint()

### Community 84 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 85 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 86 - "EntityId"
Cohesion: 0.21
Nodes (7): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, Value

### Community 87 - "RoutingConnection"
Cohesion: 0.12
Nodes (14): EntityId, findRouting, ids_, metadata_, tempoMap_, RoutingConnection, destination, gain (+6 more)

### Community 88 - "MixerTests.cpp"
Cohesion: 0.17
Nodes (11): AudioBufferPool, AudioBufferView, ProcessContext, Sample, size_t, vector, channel, onsets() (+3 more)

### Community 89 - "TimingProbeInstrument"
Cohesion: 0.15
Nodes (9): Applied, AudioBufferView, FrameCount, MidiMessage, SampleRate, vector, TimingProbeInstrument, applied (+1 more)

### Community 90 - "main"
Cohesion: 0.15
Nodes (15): AudioDeviceConfig, deviceName, midiInput_, profiler_, sampleRate, setGraph, start, transport_ (+7 more)

### Community 91 - "-applicationDidFinishLaunching"
Cohesion: 0.19
Nodes (15): INCDAWAudioEditorView, INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, NSApplicationDelegate, NSObject, NSScrollView (+7 more)

### Community 92 - "string"
Cohesion: 0.15
Nodes (8): AddChannelCommand, channel_, execute, index_, minted_, undo, size_t, string

### Community 93 - "ChannelCommands.cpp"
Cohesion: 0.17
Nodes (11): Command, execute, execute, SetChannelVolumeCommand, canMergeWith, channelId_, execute, mergeWith (+3 more)

### Community 94 - "open"
Cohesion: 0.28
Nodes (12): Format, FrameCount, path, Result, Sample, SampleRate, size_t, append (+4 more)

### Community 95 - "PluginIdentifier"
Cohesion: 0.17
Nodes (11): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+3 more)

### Community 96 - "Json.cpp"
Cohesion: 0.22
Nodes (13): size_t, string, escapeInto(), formatDouble(), append, asBool, asDouble, asString (+5 more)

### Community 97 - "CompiledProjectGraph"
Cohesion: 0.14
Nodes (14): CompiledGraph, CompiledProjectGraph, automation, channels, channelStrips, error, graph, instruments (+6 more)

### Community 98 - "MoveClipsCommand"
Cohesion: 0.15
Nodes (13): MovedAudioClip, Command, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, canMergeWith, clips_, mergeWith (+5 more)

### Community 99 - "vector"
Cohesion: 0.15
Nodes (6): Command, execute, id, name, undo, vector

### Community 100 - "string"
Cohesion: 0.18
Nodes (7): Command, string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 101 - "EntityId"
Cohesion: 0.16
Nodes (11): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+3 more)

### Community 102 - "ToggleStepCommand"
Cohesion: 0.15
Nodes (9): Command, size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_ (+1 more)

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

### Community 107 - "DiskStreamer"
Cohesion: 0.21
Nodes (11): atomic, shared_ptr, DiskStreamer, add, mutex_, running_, serviceOnce, streams_ (+3 more)

### Community 108 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 109 - "AutomationNode"
Cohesion: 0.18
Nodes (8): Binding, AutomationNode, bindings_, tempoMap_, ProcessContext, size_t, TempoMap, vector

### Community 110 - "EntityId"
Cohesion: 0.17
Nodes (6): EntityId, SetChannelMutedCommand, channelId_, execute, muted_, undo

### Community 111 - "AddPatternClipCommand"
Cohesion: 0.18
Nodes (10): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+2 more)

### Community 112 - "ResizeNotesCommand"
Cohesion: 0.15
Nodes (7): string, ResizeNotesCommand, channel_, durationDelta_, indices_, pattern_, previousDurations_

### Community 113 - "main.mm"
Cohesion: 0.23
Nodes (12): -editorChanged, -openAudioAssetInEditor, -selectChannel, -selectPattern, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll (+4 more)

### Community 114 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 115 - "humanizeNoteStarts"
Cohesion: 0.26
Nodes (11): Kind, RecordedEvent, appendRecordedEvents(), MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts() (+3 more)

### Community 116 - "RemoveMixerNodeCommand"
Cohesion: 0.17
Nodes (10): RemovedRouting, vector, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_ (+2 more)

### Community 117 - "Track"
Cohesion: 0.17
Nodes (12): findTrack, Track, colour, height, id, muted, name, outputMixerNode (+4 more)

### Community 118 - "WavStreamWriterTests.cpp"
Cohesion: 0.18
Nodes (10): FrameCount, path, size_t, uint8_t, vector, fileBytes(), makeTestSignal(), ScratchFile (+2 more)

### Community 119 - "Denormals.h"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 120 - "AddMixerNodeCommand"
Cohesion: 0.18
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 121 - "captureAudioBlock"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 122 - "AudioFileData"
Cohesion: 0.18
Nodes (10): AudioFileData, channelCount, channels, frameCount, sampleRate, FrameCount, Sample, SampleRate (+2 more)

### Community 123 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 124 - "MixerStripNode.cpp"
Cohesion: 0.31
Nodes (10): FrameCount, Sample, SampleRate, panGains, prepare, refreshTargets, setGain, setMuted (+2 more)

### Community 125 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 126 - "BlockSegment"
Cohesion: 0.18
Nodes (9): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, size_t (+1 more)

### Community 127 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 128 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 129 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 130 - "DisconnectMixerCommand"
Cohesion: 0.20
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 131 - "AddNoteCommand"
Cohesion: 0.22
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, size_t

### Community 132 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (9): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo (+1 more)

### Community 133 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 134 - "Fixture"
Cohesion: 0.20
Nodes (8): EntityId, Project, Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 135 - "MidiTests.cpp"
Cohesion: 0.22
Nodes (9): FrameCount, MidiMessage, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped() (+1 more)

### Community 136 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 137 - "RemoveChannelCommand"
Cohesion: 0.22
Nodes (8): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, index_, undo

### Community 138 - "SetChannelStepKeyCommand"
Cohesion: 0.22
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 139 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 140 - "SetMixerPanCommand"
Cohesion: 0.22
Nodes (6): SetMixerPanCommand, execute, nodeId_, pan_, previous_, undo

### Community 141 - "SetMixerVolumeCommand"
Cohesion: 0.22
Nodes (6): SetMixerVolumeCommand, execute, nodeId_, previous_, undo, volume_

### Community 142 - "QuantizeNotesCommand"
Cohesion: 0.22
Nodes (8): QuantizeNotesCommand, channel_, execute, grid_, pattern_, previousEvents_, strength_, undo

### Community 143 - "ProjectGraphCompiler.h"
Cohesion: 0.29
Nodes (6): AutomationNode, Instrument, ParameterRegistry, vector, MixerStripNode, string

### Community 144 - "MixerCommands.h"
Cohesion: 0.25
Nodes (5): SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 145 - "SetMixerPolarityCommand"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 146 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 147 - "SetVelocityCommand"
Cohesion: 0.25
Nodes (7): vector, SetVelocityCommand, channel_, indices_, pattern_, previousVelocities_, velocity_

### Community 148 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 149 - "TimestampedMidiMessage"
Cohesion: 0.25
Nodes (8): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status

### Community 150 - "MidiDevice.h"
Cohesion: 0.25
Nodes (6): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback

### Community 151 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 152 - "RemoveClipsCommand"
Cohesion: 0.29
Nodes (6): RemovedClip, string, RemoveClipsCommand, clips_, name, removed_

### Community 153 - "InstrumentTests.cpp"
Cohesion: 0.33
Nodes (6): SimpleSynth, InstrumentNode, AudioBufferPool, MidiBuffer, Sample, renderSynth()

### Community 154 - "SetChannelSoloedCommand"
Cohesion: 0.29
Nodes (6): Command, SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 155 - "AudioEditorView.mm"
Cohesion: 0.29
Nodes (6): -acceptsFirstResponder, -hasSelection, -initWithFrameprojectregistry, -isFlipped, -selectionFrom, -selectionTo

### Community 156 - "MixerFixture"
Cohesion: 0.29
Nodes (6): EntityId, TempoMap, MixerFixture, pattern, project, tempo

### Community 157 - "PatternTests.cpp"
Cohesion: 0.48
Nodes (6): SequencedNote, Tick, vector, note(), shapeOf(), startsOf()

### Community 158 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 159 - "INCDAWMixerView"
Cohesion: 0.33
Nodes (5): incdaw, NSView, INCDAWMixerView, -initWithFrameprojectregistry, stripLookup

### Community 160 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 161 - "RenameChannelCommand"
Cohesion: 0.33
Nodes (4): RenameChannelCommand, channelId_, previousName_, undo

### Community 162 - "collectForRange"
Cohesion: 0.33
Nodes (4): FrameCount, FramePosition, MidiBuffer, collectForRange

### Community 163 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 164 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 165 - "ChannelRackTests.cpp"
Cohesion: 0.33
Nodes (5): EntityId, Step, Tick, note(), stepAt()

### Community 166 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 167 - "collectForBlock"
Cohesion: 0.40
Nodes (5): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock

### Community 168 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 169 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 170 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 172 - "INCDAWChannelRackView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWChannelRackView, -initWithFrameprojectregistry

### Community 173 - "INCDAWPatternListView"
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
- **804 isolated node(s):** `index_`, `minted_`, `track_`, `soloed_`, `trackId_` (+799 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **35 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `AudioAsset`, `RemoveTrackCommand`, `DisconnectMixerCommand`, `AddNoteCommand`, `DeleteNotesCommand`, `MixerNode`, `AudioBufferPool`, `RemoveChannelCommand`, `SetChannelStepKeyCommand`, `SetChannelOutputCommand`, `CommandRegistry`, `SetMixerPanCommand`, `SetMixerVolumeCommand`, `QuantizeNotesCommand`, `MixerCommands.h`, `SetMixerPolarityCommand`, `SetMixerSoloedCommand`, `Clip`, `ProjectFile.cpp`, `SetChannelSoloedCommand`, `PatternCommands.cpp`, `MixerFixture`, `RenameChannelCommand`, `SetAutomationPointsCommand`, `AddPatternCommand`, `Pattern`, `NoteCommands.cpp`, `InsertRecordedTakeCommand`, `MidiEvent`, `CountingCommand`, `compileArrangement`, `MixerCommands.cpp`, `DuplicatePatternCommand`, `RoutingConnection`, `string`, `ChannelCommands.cpp`, `string`, `EntityId`, `Fixture`, `EntityId`, `RemoveMixerNodeCommand`, `Track`, `AddMixerNodeCommand`?**
  _High betweenness centrality (0.247) - this node is a cross-community bridge._