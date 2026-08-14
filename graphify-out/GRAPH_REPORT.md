# Graph Report - project-continuation-670d11  (2026-08-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 3331 nodes · 5643 edges · 230 communities (193 shown, 37 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 263 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `df520a17`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- MixerCommands.cpp
- INCDAW
- PatternCommands.cpp
- RemoveTrackCommand
- Transport
- WavBytes.h
- MixerNode
- CoreAudioDevice
- Project
- PlaylistModel
- SimpleSynth
- Pattern
- PianoRollModel
- CommandRegistry
- CompiledProjectGraph
- AudioEngine
- EditAssetRegionCommand
- ChannelRackModel
- InstrumentNode
- NoteSequence
- RecordingSession
- TempoMap
- Clip
- WavStreamReader
- AudioStream
- AudioDeviceConfig
- MixerView.mm
- CoreAudioDevice.cpp
- ProjectFile.cpp
- string
- InsertRecordedTakeCommand
- MidiEvent
- MidiMessage
- PlaylistView.mm
- AudioRecorder
- atomic
- TestGainPlugin.cpp
- GraphCompileOptions
- Json
- AudioLogger
- CallbackProfiler
- TimeSignature
- MetronomeNode
- AudioDevice
- InputMonitorNode
- WriteAutomationCommand
- NoteCommands.cpp
- AudioBufferPool
- CompiledGraph
- TimingProbeInstrument
- LevelMeter
- AudioEngine.cpp
- EntityId
- DelayLineNode
- compile
- CountingCommand
- RealtimeGuard.cpp
- compileArrangement
- WaveformOverview
- Region
- MidiInput
- EditFixture
- WavFile
- main
- AutomationNode
- AudioClipNode
- AddAutomationLaneCommand
- AudioBufferView
- BasicMidiBuffer
- ConstantNode
- ioProcTrampoline
- ResizeClipsCommand
- AutomationWriteSession
- ClipCommands.cpp
- LockFreeQueue
- SampleRingBuffer
- Smoother
- MixerStripNode
- SystemInfo
- Parser
- LoopbackResult
- LatentProcessorNode
- -applicationDidFinishLaunching
- WavStreamWriter
- AutomationCommands.cpp
- GainNode
- Node
- SineOscillatorNode
- EntityId
- MixerTests.cpp
- ClapLibrary
- GraphBuilder
- string
- DeleteNotesCommand
- open
- ProcessContext
- PluginIdentifier
- Json.cpp
- CoreMidiDevice.cpp
- Channel
- CoreMidiDevice
- MoveClipsCommand
- ResizeNotesCommand
- ParameterRegistry
- ChannelRackView.mm
- renderArrangement
- Fixture
- Options
- DiskStreamer
- renderNode
- AutomationPoint
- AddPatternClipCommand
- EntityId
- main.mm
- MidiRecorder
- ClapLibrary.cpp
- string
- ChannelCommands.cpp
- Track
- WavStreamWriterTests.cpp
- TimelineAnchor
- MidiTests.cpp
- AddNoteCommand
- vector
- captureAudioBlock
- AudioFileData
- MixerStripNode.cpp
- MixerFixture
- TimeSignatureEvent
- MidiDevice
- ScanOutcome
- AudioAsset
- PatternListView.mm
- RemoveChannelCommand
- build
- RecordedEvent
- SharedLibrary
- ClapDescriptor
- RoutingConnection
- Fixture
- AutomationProbe
- make-dmg.sh
- Command
- SetChannelStepKeyCommand
- DuplicateClipsCommand
- SetChannelOutputCommand
- QuantizeNotesCommand
- MoveNotesCommand
- SetVelocityCommand
- ClapInstance
- PianoRollView.mm
- AutomationFixture
- Denormals.h
- SetAutomationPointsCommand
- humanizeNoteStarts
- Version
- TimestampedMidiMessage
- MidiDevice.h
- makeTestSignal
- RemoveMixerNodeCommand
- PatternTests.cpp
- ScratchDirectory
- INCDAWMixerView
- INCDAWPianoRollView
- AddMixerNodeCommand
- DisconnectMixerCommand
- process
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- collectForBlock
- MidiRecorder.cpp
- scanOutOfProcess
- INCDAWAudioEditorView
- INCDAWPlaylistView
- check
- start
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
- SetMixerPanCommand
- SetMixerVolumeCommand
- SetChannelSoloedCommand
- Command
- MixerCommands.h
- SetMixerSoloedCommand
- RemoveClipsCommand
- MixerStripNode.h

## God Nodes (most connected - your core abstractions)
1. `Project` - 147 edges
2. `AudioEngine` - 67 edges
3. `CoreAudioDevice` - 59 edges
4. `Json` - 44 edges
5. `Transport` - 38 edges
6. `SimpleSynth` - 36 edges
7. `TempoMap` - 36 edges
8. `Clip` - 36 edges
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

## Communities (230 total, 37 thin omitted)

### Community 0 - "MixerCommands.cpp"
Cohesion: 0.16
Nodes (14): undo, Command, canMergeWith, mergeWith, canMergeWith, mergeWith, SetSendGainCommand, canMergeWith (+6 more)

### Community 1 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 2 - "PatternCommands.cpp"
Cohesion: 0.05
Nodes (46): AddPatternCommand, execute, index_, minted_, pattern_, undo, Command, DuplicatePatternCommand (+38 more)

### Community 3 - "RemoveTrackCommand"
Cohesion: 0.05
Nodes (43): AddTrackCommand, execute, index_, minted_, track_, undo, Command, Command (+35 more)

### Community 4 - "Transport"
Cohesion: 0.06
Nodes (37): MusicalPosition, BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FramePosition (+29 more)

### Community 5 - "WavBytes.h"
Cohesion: 0.09
Nodes (45): appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample, channels (+37 more)

### Community 6 - "MixerNode"
Cohesion: 0.08
Nodes (28): MixerNodeType, PluginIdentifier, string, TempoMap, MixerNode, colour, id, inserts (+20 more)

### Community 7 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 8 - "Project"
Cohesion: 0.13
Nodes (35): IdGenerator, EntityId, size_t, vector, operator==(), totalEventCount, Project, audioAssets_ (+27 more)

### Community 9 - "PlaylistModel"
Cohesion: 0.09
Nodes (35): Rect, Clip, EntityId, Project, size_t, Tick, Track, vector (+27 more)

### Community 10 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 11 - "Pattern"
Cohesion: 0.06
Nodes (27): AutomationCurve, friend, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint (+19 more)

### Community 12 - "PianoRollModel"
Cohesion: 0.11
Nodes (26): NoteList, size_t, Tick, vector, size_t, Tick, Viewport, PianoRollModel (+18 more)

### Community 13 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 14 - "CompiledProjectGraph"
Cohesion: 0.09
Nodes (31): AutomationNode, Channel, Instrument, ParameterRegistry, CompiledProjectGraph, automation, channels, channelStripFor (+23 more)

### Community 15 - "AudioEngine"
Cohesion: 0.08
Nodes (26): AudioDevice, AudioIOCallback, CallbackProfiler, MidiBuffer, mutex, RetiredGraph, SampleRingBuffer, AudioCaptureSink (+18 more)

### Community 16 - "EditAssetRegionCommand"
Cohesion: 0.08
Nodes (24): AudioEditOp, Command, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_ (+16 more)

### Community 17 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 18 - "InstrumentNode"
Cohesion: 0.08
Nodes (23): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+15 more)

### Community 19 - "NoteSequence"
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, MidiBuffer, Tick, vector, size_t, Tick, uint32_t (+17 more)

### Community 20 - "RecordingSession"
Cohesion: 0.09
Nodes (23): AudioEngine, AudioRecorder, path, Slice, Placement, string, vector, FrameCount (+15 more)

### Community 21 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 22 - "Clip"
Cohesion: 0.07
Nodes (29): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+21 more)

### Community 23 - "WavStreamReader"
Cohesion: 0.09
Nodes (24): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+16 more)

### Community 24 - "AudioStream"
Cohesion: 0.10
Nodes (24): shared_ptr, AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_ (+16 more)

### Community 25 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 26 - "MixerView.mm"
Cohesion: 0.17
Nodes (26): MixerStripNode, NSMenu, -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect (+18 more)

### Community 27 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 28 - "ProjectFile.cpp"
Cohesion: 0.20
Nodes (23): Json, automationPointFrom(), bindUnassignedContent(), AutomationPoint, EntityId, path, PluginIdentifier, Result (+15 more)

### Community 29 - "string"
Cohesion: 0.16
Nodes (6): string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 30 - "InsertRecordedTakeCommand"
Cohesion: 0.09
Nodes (20): Clip, EntityId, Project, Placement, size_t, string, vector, InsertRecordedTakeCommand (+12 more)

### Community 31 - "MidiEvent"
Cohesion: 0.07
Nodes (29): MidiEventType, size_t, Tick, vector, Command, size_t, Step, string (+21 more)

### Community 32 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 33 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (24): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+16 more)

### Community 34 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

### Community 35 - "atomic"
Cohesion: 0.12
Nodes (6): vector, atomic, MidiBuffer, array, allocationSize(), size_t

### Community 36 - "TestGainPlugin.cpp"
Cohesion: 0.13
Nodes (19): clap_plugin_descriptor_t, clap_process_status, clap_process_t, clap_host_t, clap_plugin_factory_t, clap_plugin_t, factoryCreatePlugin(), factoryGetPluginCount() (+11 more)

### Community 37 - "GraphCompileOptions"
Cohesion: 0.09
Nodes (23): DiskStreamer, PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, instrumentFactory, masterGain, maxBlockSize (+15 more)

### Community 38 - "Json"
Cohesion: 0.10
Nodes (15): nullptr_t, pair, int64_t, int64_t, string, vector, Json, asInt (+7 more)

### Community 39 - "AudioLogger"
Cohesion: 0.08
Nodes (24): AudioFileData, AudioLogger, capacityFrames_, circle_, enabled_, grab, log, prepare (+16 more)

### Community 40 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 41 - "TimeSignature"
Cohesion: 0.13
Nodes (16): Tick, friend, int64_t, Tick, MusicalPosition, bar, beat, tick (+8 more)

### Community 42 - "MetronomeNode"
Cohesion: 0.09
Nodes (17): atomic, FrameCount, Sample, SampleRate, size_t, vector, MetronomeNode, amplitude_ (+9 more)

### Community 43 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 44 - "InputMonitorNode"
Cohesion: 0.12
Nodes (17): Node, FrameCount, Sample, SampleRate, SampleRingBuffer, size_t, vector, InputMonitorNode (+9 more)

### Community 45 - "WriteAutomationCommand"
Cohesion: 0.10
Nodes (19): AutomationPoint, Clip, Track, vector, WriteAutomationCommand, clip_, clipIndex_, key_ (+11 more)

### Community 46 - "NoteCommands.cpp"
Cohesion: 0.17
Nodes (20): Command, EntityId, NoteIndices, size_t, vector, execute, findEvents(), canMergeWith (+12 more)

### Community 47 - "AudioBufferPool"
Cohesion: 0.13
Nodes (13): AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t, FrameCount (+5 more)

### Community 48 - "CompiledGraph"
Cohesion: 0.11
Nodes (15): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+7 more)

### Community 49 - "TimingProbeInstrument"
Cohesion: 0.11
Nodes (14): Applied, SimpleSynth, AudioBufferPool, AudioBufferView, FrameCount, MidiBuffer, MidiMessage, Sample (+6 more)

### Community 50 - "LevelMeter"
Cohesion: 0.14
Nodes (13): atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond (+5 more)

### Community 51 - "AudioEngine.cpp"
Cohesion: 0.17
Nodes (17): int64_t, audioDeviceAboutToStart, audioDeviceStopped, bufferSize, captureAudioBlock, collectRetiredGraphs, inputChannels, isRunning (+9 more)

### Community 52 - "EntityId"
Cohesion: 0.12
Nodes (12): Command, EntityId, RenameChannelCommand, channelId_, execute, previousName_, undo, SetChannelMutedCommand (+4 more)

### Community 53 - "DelayLineNode"
Cohesion: 0.12
Nodes (16): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+8 more)

### Community 54 - "compile"
Cohesion: 0.14
Nodes (19): process, AudioBufferView, FrameCount, FramePosition, MidiBuffer, Node, NodeIndex, SampleRate (+11 more)

### Community 55 - "CountingCommand"
Cohesion: 0.12
Nodes (10): CountingCommand, counter_, delta_, Command, EntityId, string, Tick, makeProjectWithNotes() (+2 more)

### Community 56 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 57 - "compileArrangement"
Cohesion: 0.25
Nodes (18): Emit, NoteSequence, content, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto() (+10 more)

### Community 58 - "WaveformOverview"
Cohesion: 0.11
Nodes (17): SampleRate, Bucket, FrameCount, size_t, vector, WaveformOverview, channelCount, channels (+9 more)

### Community 59 - "Region"
Cohesion: 0.30
Nodes (16): applyGain(), applyRamp(), clampedRegion(), AudioFileData, Sample, fadeIn(), fadeOut(), FrameCount (+8 more)

### Community 60 - "MidiInput"
Cohesion: 0.14
Nodes (14): atomic, queueCapacity, size_t, uint64_t, MidiInput, dropped_, hasPending_, late_ (+6 more)

### Community 61 - "EditFixture"
Cohesion: 0.14
Nodes (15): AudioFileData, EntityId, FrameCount, Project, Sample, size_t, EditFixture, assetId (+7 more)

### Community 62 - "WavFile"
Cohesion: 0.25
Nodes (17): AudioAsset, assetFilePath(), AudioFileData, EntityId, Project, Sample, string, vector (+9 more)

### Community 63 - "main"
Cohesion: 0.14
Nodes (17): AudioDeviceInfo, availableDevices, midiInput_, profiler_, sampleRate, setGraph, transport_, CompiledGraph (+9 more)

### Community 64 - "AutomationNode"
Cohesion: 0.15
Nodes (11): Binding, AutomationNode, bindings_, tempoMap_, size_t, TempoMap, vector, AutomationPoint (+3 more)

### Community 65 - "AudioClipNode"
Cohesion: 0.11
Nodes (15): PlacedClip, ProcessContext, vector, AudioClipNode, addClip, clips_, fetchScratch_, prepare (+7 more)

### Community 66 - "AddAutomationLaneCommand"
Cohesion: 0.14
Nodes (13): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, AutomationLane, EntityId (+5 more)

### Community 67 - "AudioBufferView"
Cohesion: 0.22
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 68 - "BasicMidiBuffer"
Cohesion: 0.13
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 69 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 70 - "ioProcTrampoline"
Cohesion: 0.21
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 71 - "ResizeClipsCommand"
Cohesion: 0.12
Nodes (12): Command, FrameCount, string, ResizeClipsCommand, clips_, lengthDelta_, previousFrameLengths_, previousLengths_ (+4 more)

### Community 72 - "AutomationWriteSession"
Cohesion: 0.14
Nodes (13): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, Command, EntityId (+5 more)

### Community 73 - "ClipCommands.cpp"
Cohesion: 0.24
Nodes (15): execute, undo, EntityId, Project, execute, undo, execute, undo (+7 more)

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

### Community 78 - "SystemInfo"
Cohesion: 0.15
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 79 - "Parser"
Cohesion: 0.25
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 80 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 81 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 82 - "-applicationDidFinishLaunching"
Cohesion: 0.18
Nodes (16): INCDAWAudioEditorView, INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, INCDAWPlaylistView, NSApplicationDelegate, NSObject (+8 more)

### Community 83 - "WavStreamWriter"
Cohesion: 0.13
Nodes (14): ofstream, Format, FrameCount, path, size_t, uint8_t, vector, WavStreamWriter (+6 more)

### Community 84 - "AutomationCommands.cpp"
Cohesion: 0.22
Nodes (15): execute, undo, AutomationLane, AutomationPoint, EntityId, Project, vector, findLane() (+7 more)

### Community 85 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 86 - "Node"
Cohesion: 0.14
Nodes (8): FrameCount, SampleRate, Node, process, vector, OrderRecordingNode, identifier_, log_

### Community 87 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 88 - "EntityId"
Cohesion: 0.21
Nodes (7): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, Value

### Community 89 - "MixerTests.cpp"
Cohesion: 0.17
Nodes (11): AudioBufferPool, AudioBufferView, ProcessContext, Sample, size_t, vector, channel, onsets() (+3 more)

### Community 90 - "ClapLibrary"
Cohesion: 0.17
Nodes (13): clap_plugin_entry_t, ClapLibrary, close, create, entry_, factory_, library_, open (+5 more)

### Community 91 - "GraphBuilder"
Cohesion: 0.14
Nodes (12): Connection, GraphBuilder, compensate_, connections_, error_, master_, nodes_, Node (+4 more)

### Community 92 - "string"
Cohesion: 0.15
Nodes (8): AddChannelCommand, channel_, execute, index_, minted_, undo, size_t, string

### Community 93 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (9): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo (+1 more)

### Community 94 - "open"
Cohesion: 0.28
Nodes (12): Format, FrameCount, path, Result, Sample, SampleRate, size_t, append (+4 more)

### Community 95 - "ProcessContext"
Cohesion: 0.14
Nodes (13): FramePosition, MidiBuffer, size_t, ProcessContext, frameCount, inputCount, inputs, liveMidi (+5 more)

### Community 96 - "PluginIdentifier"
Cohesion: 0.17
Nodes (11): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+3 more)

### Community 97 - "Json.cpp"
Cohesion: 0.22
Nodes (13): size_t, string, escapeInto(), formatDouble(), append, asBool, asDouble, asString (+5 more)

### Community 98 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 99 - "Channel"
Cohesion: 0.13
Nodes (15): process, Channel, colour, id, instrument, instrumentStateFile, muted, name (+7 more)

### Community 100 - "CoreMidiDevice"
Cohesion: 0.15
Nodes (14): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+6 more)

### Community 101 - "MoveClipsCommand"
Cohesion: 0.15
Nodes (13): MovedAudioClip, Command, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, canMergeWith, clips_, mergeWith (+5 more)

### Community 102 - "ResizeNotesCommand"
Cohesion: 0.15
Nodes (7): string, ResizeNotesCommand, channel_, durationDelta_, indices_, pattern_, previousDurations_

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

### Community 108 - "DiskStreamer"
Cohesion: 0.21
Nodes (10): atomic, vector, DiskStreamer, mutex_, running_, serviceOnce, streams_, thread_ (+2 more)

### Community 109 - "renderNode"
Cohesion: 0.23
Nodes (10): FrameCount, Node, Sample, size_t, vector, makeAudio(), renderNode(), ScratchDir (+2 more)

### Community 110 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 111 - "AddPatternClipCommand"
Cohesion: 0.18
Nodes (10): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+2 more)

### Community 112 - "EntityId"
Cohesion: 0.16
Nodes (11): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+3 more)

### Community 113 - "main.mm"
Cohesion: 0.23
Nodes (12): -editorChanged, -openAudioAssetInEditor, -selectChannel, -selectPattern, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll (+4 more)

### Community 114 - "MidiRecorder"
Cohesion: 0.20
Nodes (10): CapturedMessage, atomic, queueCapacity, size_t, uint64_t, MidiRecorder, captured_, dropped_ (+2 more)

### Community 115 - "ClapLibrary.cpp"
Cohesion: 0.24
Nodes (11): clap_event_header_t, clap_input_events_t, clap_output_events_t, clap_host_t, emptyInGet(), emptyInSize(), emptyOutTryPush(), hostGetExtension() (+3 more)

### Community 116 - "string"
Cohesion: 0.24
Nodes (3): vector, Command, string

### Community 117 - "ChannelCommands.cpp"
Cohesion: 0.23
Nodes (9): Command, SetChannelVolumeCommand, canMergeWith, channelId_, execute, mergeWith, previousVolume_, undo (+1 more)

### Community 118 - "Track"
Cohesion: 0.15
Nodes (13): findTrack, Track, colour, height, id, muted, name, outputMixerNode (+5 more)

### Community 119 - "WavStreamWriterTests.cpp"
Cohesion: 0.18
Nodes (10): FrameCount, path, size_t, uint8_t, vector, fileBytes(), makeTestSignal(), ScratchFile (+2 more)

### Community 120 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 121 - "MidiTests.cpp"
Cohesion: 0.20
Nodes (10): MidiInput, FrameCount, MidiMessage, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote() (+2 more)

### Community 122 - "AddNoteCommand"
Cohesion: 0.22
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, size_t

### Community 123 - "vector"
Cohesion: 0.18
Nodes (6): vector, EntityId, Step, Tick, note(), stepAt()

### Community 124 - "captureAudioBlock"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 125 - "AudioFileData"
Cohesion: 0.18
Nodes (10): AudioFileData, channelCount, channels, frameCount, sampleRate, FrameCount, Sample, SampleRate (+2 more)

### Community 126 - "MixerStripNode.cpp"
Cohesion: 0.31
Nodes (10): FrameCount, Sample, SampleRate, panGains, prepare, refreshTargets, setGain, setMuted (+2 more)

### Community 127 - "MixerFixture"
Cohesion: 0.18
Nodes (8): buildParallelPaths(), EntityId, TempoMap, unique_ptr, MixerFixture, pattern, project, tempo

### Community 128 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 129 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 130 - "ScanOutcome"
Cohesion: 0.24
Nodes (7): string, vector, ScanOutcome, detail, plugins, status, Status

### Community 131 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 132 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 133 - "RemoveChannelCommand"
Cohesion: 0.20
Nodes (9): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, execute, index_ (+1 more)

### Community 134 - "build"
Cohesion: 0.27
Nodes (9): Result, bucketize(), AudioFileData, Bucket, FrameCount, Sample, vector, sizeBuckets() (+1 more)

### Community 135 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 136 - "SharedLibrary"
Cohesion: 0.33
Nodes (7): path, string, SharedLibrary, close, handle_, open, symbol

### Community 137 - "ClapDescriptor"
Cohesion: 0.20
Nodes (9): ClapDescriptor, id, name, vendor, version, descriptors, vector, string (+1 more)

### Community 138 - "RoutingConnection"
Cohesion: 0.12
Nodes (14): EntityId, findRouting, ids_, metadata_, tempoMap_, RoutingConnection, destination, gain (+6 more)

### Community 139 - "Fixture"
Cohesion: 0.20
Nodes (8): EntityId, Project, Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 140 - "AutomationProbe"
Cohesion: 0.24
Nodes (7): AutomationProbe, calls, registry, written, Project, vector, makeProject()

### Community 141 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 142 - "Command"
Cohesion: 0.22
Nodes (5): Command, execute, id, name, undo

### Community 143 - "SetChannelStepKeyCommand"
Cohesion: 0.22
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 144 - "DuplicateClipsCommand"
Cohesion: 0.16
Nodes (10): ClipIds, DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_ (+2 more)

### Community 145 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 146 - "QuantizeNotesCommand"
Cohesion: 0.22
Nodes (8): QuantizeNotesCommand, channel_, execute, grid_, pattern_, previousEvents_, strength_, undo

### Community 147 - "MoveNotesCommand"
Cohesion: 0.16
Nodes (11): EntityId, NoteIndices, Tick, MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, channel_, indices_ (+3 more)

### Community 148 - "SetVelocityCommand"
Cohesion: 0.25
Nodes (7): vector, SetVelocityCommand, channel_, indices_, pattern_, previousVelocities_, velocity_

### Community 149 - "ClapInstance"
Cohesion: 0.25
Nodes (7): ClapInstance, host_, plugin_, processing_, steadyTime_, clap_host_t, clap_plugin_t

### Community 150 - "PianoRollView.mm"
Cohesion: 0.25
Nodes (8): -acceptsFirstResponder, -currentNotes, -displayLinkFired, -initWithFrameprojectregistry, -isFlipped, -setNeedsDisplay, -updateDrawableSize, -viewDidMoveToWindow

### Community 151 - "AutomationFixture"
Cohesion: 0.22
Nodes (7): AutomationFixture, channel, pattern, project, tempo, EntityId, TempoMap

### Community 152 - "Denormals.h"
Cohesion: 0.39
Nodes (5): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister()

### Community 153 - "SetAutomationPointsCommand"
Cohesion: 0.29
Nodes (7): Command, SetAutomationPointsCommand, canMergeWith, laneId_, mergeWith, points_, previous_

### Community 154 - "humanizeNoteStarts"
Cohesion: 0.26
Nodes (11): Kind, RecordedEvent, appendRecordedEvents(), MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts() (+3 more)

### Community 155 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 156 - "TimestampedMidiMessage"
Cohesion: 0.25
Nodes (8): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status

### Community 157 - "MidiDevice.h"
Cohesion: 0.25
Nodes (6): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback

### Community 158 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 159 - "RemoveMixerNodeCommand"
Cohesion: 0.17
Nodes (10): RemovedRouting, vector, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_ (+2 more)

### Community 160 - "PatternTests.cpp"
Cohesion: 0.48
Nodes (6): SequencedNote, Tick, vector, note(), shapeOf(), startsOf()

### Community 161 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 162 - "INCDAWMixerView"
Cohesion: 0.33
Nodes (5): incdaw, NSView, INCDAWMixerView, -initWithFrameprojectregistry, stripLookup

### Community 163 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 164 - "AddMixerNodeCommand"
Cohesion: 0.20
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 165 - "DisconnectMixerCommand"
Cohesion: 0.20
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 166 - "process"
Cohesion: 0.40
Nodes (5): FrameCount, SampleRate, prepare, process, triggerClick

### Community 167 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 168 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 169 - "collectForBlock"
Cohesion: 0.40
Nodes (5): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock

### Community 170 - "MidiRecorder.cpp"
Cohesion: 0.40
Nodes (4): FramePosition, MidiBuffer, capture, reset

### Community 171 - "scanOutOfProcess"
Cohesion: 0.60
Nodes (4): path, string, parseLine(), scanOutOfProcess()

### Community 172 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 173 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 174 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 175 - "start"
Cohesion: 0.50
Nodes (4): AudioDeviceConfig, deviceName, start, string

### Community 177 - "framesToSeconds"
Cohesion: 0.67
Nodes (4): framesToSeconds(), FrameCount, SampleRate, secondsToFrames()

### Community 178 - "INCDAWChannelRackView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWChannelRackView, -initWithFrameprojectregistry

### Community 179 - "INCDAWPatternListView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWPatternListView, -initWithFrameprojectregistry

### Community 222 - "SetMixerPanCommand"
Cohesion: 0.22
Nodes (6): SetMixerPanCommand, execute, nodeId_, pan_, previous_, undo

### Community 223 - "SetMixerVolumeCommand"
Cohesion: 0.22
Nodes (6): SetMixerVolumeCommand, execute, nodeId_, previous_, undo, volume_

### Community 224 - "SetChannelSoloedCommand"
Cohesion: 0.25
Nodes (5): SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 225 - "Command"
Cohesion: 0.25
Nodes (6): Command, SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 226 - "MixerCommands.h"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 227 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 228 - "RemoveClipsCommand"
Cohesion: 0.29
Nodes (6): RemovedClip, string, RemoveClipsCommand, clips_, name, removed_

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
- **859 isolated node(s):** `nodeId_`, `soloed_`, `connection_`, `connectionId_`, `index_` (+854 more)
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
- **Why does `Project` connect `Project` to `MixerCommands.cpp`, `PatternCommands.cpp`, `RemoveTrackCommand`, `AudioAsset`, `RemoveChannelCommand`, `MixerNode`, `RoutingConnection`, `Pattern`, `AutomationProbe`, `CommandRegistry`, `SetChannelStepKeyCommand`, `SetChannelOutputCommand`, `QuantizeNotesCommand`, `Clip`, `AutomationFixture`, `ProjectFile.cpp`, `string`, `RemoveMixerNodeCommand`, `MidiEvent`, `AddMixerNodeCommand`, `DisconnectMixerCommand`, `NoteCommands.cpp`, `EntityId`, `CountingCommand`, `compileArrangement`, `string`, `DeleteNotesCommand`, `SetMixerPanCommand`, `SetMixerVolumeCommand`, `SetChannelSoloedCommand`, `Command`, `MixerCommands.h`, `SetMixerSoloedCommand`, `Channel`, `Fixture`, `EntityId`, `ChannelCommands.cpp`, `Track`, `AddNoteCommand`, `MixerFixture`?**
  _High betweenness centrality (0.233) - this node is a cross-community bridge._