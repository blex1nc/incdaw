# Graph Report - project-continuation-670d11  (2026-08-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 3059 nodes · 5280 edges · 222 communities (189 shown, 33 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 248 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `0b078495`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- WavBytes.h
- PianoRollModel
- CoreAudioDevice
- SimpleSynth
- Project
- Transport
- CommandRegistry
- ChannelRackModel
- EditAssetRegionCommand
- TempoMap
- AudioStream
- Pattern
- Clip
- MetronomeNode
- Json
- Channel
- AudioDeviceConfig
- CompiledProjectGraph
- CoreMidiDevice
- MusicalPosition
- Json.cpp
- AudioEngine
- CoreAudioDevice.cpp
- NoteSequence
- MixerView.mm
- MidiMessage
- AudioRecorder
- MidiInput
- ProjectFile.cpp
- CallbackProfiler
- AudioDevice
- compileProjectGraph
- NoteCommands.cpp
- AudioBufferPool
- TimingProbeInstrument
- GraphBuilder
- InsertRecordedTakeCommand
- PlaylistView.mm
- atomic
- DelayLineNode
- CompiledGraph
- CountingCommand
- RealtimeGuard.cpp
- WavStreamReader
- MidiEvent
- GraphCompileOptions
- AddAutomationLaneCommand
- PlaylistModel
- Region
- AudioEngine.cpp
- TempoMap
- AudioBufferView
- ProcessContext
- BasicMidiBuffer
- SystemInfo
- EditFixture
- WavFile
- main
- RecordingSession
- AudioClipNode
- MixerCommands.cpp
- LevelMeter
- compile
- InstrumentNode
- PluginIdentifier
- ConstantNode
- MidiRecorder
- vector
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
- noteAtStep
- SineOscillatorNode
- EntityId
- RoutingConnection
- MixerTests.cpp
- AutomationPoint
- ChannelCommands.cpp
- AddPatternClipCommand
- PatternCommands.cpp
- TrackCommands.cpp
- open
- renderNode
- -applicationDidFinishLaunching
- WaveformOverview
- vector
- string
- EntityId
- SetVelocityCommand
- AddPatternCommand
- DuplicatePatternCommand
- ToggleStepCommand
- ChannelRackView.mm
- renderArrangement
- Fixture
- Options
- DuplicateClipsCommand
- MixerNode
- RemoveMixerNodeCommand
- EntityId
- ClipCommands.cpp
- AudioAsset
- ParameterRegistry
- main.mm
- humanizeNoteStarts
- AutomationCommands.cpp
- SetAutomationPointsCommand
- AddMixerNodeCommand
- RenameTrackCommand
- EntityId
- Track
- Instrument
- WavStreamWriterTests.cpp
- AutomationNode
- Denormals.h
- AddNoteCommand
- captureAudioBlock
- AudioFileData
- TimelineAnchor
- MixerStripNode.cpp
- TimeSignatureEvent
- BlockSegment
- MidiDevice
- PatternListView.mm
- RemoveClipsCommand
- string
- DisconnectMixerCommand
- DeleteNotesCommand
- readAt
- RecordedEvent
- make-dmg.sh
- DiskStreamer
- SetClipMutedCommand
- SetMixerPolarityCommand
- SetChannelOutputCommand
- SetMixerVolumeCommand
- SetSendGainCommand
- RemovePatternCommand
- MidiDevice.h
- AutomationFixture
- RemoveChannelCommand
- build
- RenameChannelCommand
- SetChannelMutedCommand
- MoveClipsCommand
- Command
- SetMixerSoloedCommand
- SetPatternSwingCommand
- AddTrackCommand
- RemoveTrackCommand
- SetTrackMutedCommand
- Version
- TimestampedMidiMessage
- MidiTests.cpp
- makeTestSignal
- ProjectGraphCompiler.h
- SetChannelSoloedCommand
- ResizeClipsCommand
- RenamePatternCommand
- AudioEditorView.mm
- MixerFixture
- ScratchDirectory
- INCDAWMixerView
- INCDAWPianoRollView
- string
- collectForRange
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- OrderRecordingNode
- INCDAWAudioEditorView
- check
- start
- AudioCaptureSink
- INCDAWChannelRackView
- INCDAWPatternListView
- INCDAWPlaylistView
- clipRect
- snapTick
- CallbackProfiler
- FrameCount
- FramePosition
- INCDAWPlaylistView
- NSMenu
- NSView
- Sample
- AutomationLane
- Project
- AutomationLane
- Rect
- Node
- T
- Kind
- friend
- MidiEventType
- MixerNodeType
- PluginIdentifier
- uint32_t
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
1. `Project` - 173 edges
2. `AudioEngine` - 60 edges
3. `CoreAudioDevice` - 59 edges
4. `Json` - 44 edges
5. `Clip` - 40 edges
6. `TempoMap` - 38 edges
7. `Transport` - 37 edges
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

## Communities (222 total, 33 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "WavBytes.h"
Cohesion: 0.09
Nodes (45): appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample, channels (+37 more)

### Community 2 - "PianoRollModel"
Cohesion: 0.09
Nodes (32): NoteList, size_t, Tick, vector, size_t, Tick, Viewport, PianoRollModel (+24 more)

### Community 3 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 4 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 5 - "Project"
Cohesion: 0.12
Nodes (37): IdGenerator, undo, EntityId, size_t, vector, operator==(), events, totalEventCount (+29 more)

### Community 6 - "Transport"
Cohesion: 0.09
Nodes (24): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, Transport (+16 more)

### Community 7 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 8 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 9 - "EditAssetRegionCommand"
Cohesion: 0.09
Nodes (24): AudioEditOp, Command, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_ (+16 more)

### Community 10 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 11 - "AudioStream"
Cohesion: 0.10
Nodes (24): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+16 more)

### Community 12 - "Pattern"
Cohesion: 0.06
Nodes (28): AutomationCurve, friend, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint (+20 more)

### Community 13 - "Clip"
Cohesion: 0.07
Nodes (29): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+21 more)

### Community 14 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 15 - "Json"
Cohesion: 0.08
Nodes (18): nullptr_t, pair, int64_t, int64_t, string, vector, Json, append (+10 more)

### Community 16 - "Channel"
Cohesion: 0.07
Nodes (29): PluginIdentifier, Channel, colour, id, instrument, instrumentStateFile, muted, name (+21 more)

### Community 17 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 18 - "CompiledProjectGraph"
Cohesion: 0.09
Nodes (26): AutomationNode, Channel, CompiledGraph, CompiledProjectGraph, automation, channels, channelStripFor, channelStrips (+18 more)

### Community 19 - "CoreMidiDevice"
Cohesion: 0.14
Nodes (24): CFStringRef, MIDIClientRef, MIDIEndpointRef, MIDIObjectRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_ (+16 more)

### Community 20 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 21 - "Json.cpp"
Cohesion: 0.19
Nodes (22): size_t, string, escapeInto(), formatDouble(), asString, contains, dump, dumpTo (+14 more)

### Community 22 - "AudioEngine"
Cohesion: 0.10
Nodes (23): AudioDevice, AudioIOCallback, MidiBuffer, MidiInput, mutex, RetiredGraph, AudioCaptureSink, AudioEngine (+15 more)

### Community 23 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 24 - "NoteSequence"
Cohesion: 0.11
Nodes (21): Tick, vector, size_t, Tick, uint32_t, vector, NoteSequence, byEnd_ (+13 more)

### Community 25 - "MixerView.mm"
Cohesion: 0.18
Nodes (25): -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect, -drawStripnode, -faderRectAt (+17 more)

### Community 26 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 27 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

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

### Community 32 - "compileProjectGraph"
Cohesion: 0.20
Nodes (21): Emit, NoteSequence, content, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto() (+13 more)

### Community 33 - "NoteCommands.cpp"
Cohesion: 0.16
Nodes (21): Command, EntityId, NoteIndices, size_t, vector, execute, findEvents(), canMergeWith (+13 more)

### Community 34 - "AudioBufferPool"
Cohesion: 0.13
Nodes (13): AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t, FrameCount (+5 more)

### Community 35 - "TimingProbeInstrument"
Cohesion: 0.11
Nodes (14): Applied, SimpleSynth, AudioBufferPool, AudioBufferView, FrameCount, MidiBuffer, MidiMessage, Sample (+6 more)

### Community 36 - "GraphBuilder"
Cohesion: 0.11
Nodes (16): Connection, NodeIndex, GraphBuilder, compensate_, connect, connections_, error_, master_ (+8 more)

### Community 37 - "InsertRecordedTakeCommand"
Cohesion: 0.11
Nodes (16): Command, EntityId, Placement, size_t, InsertRecordedTakeCommand, asset_, assetIndex_, clip_ (+8 more)

### Community 38 - "PlaylistView.mm"
Cohesion: 0.17
Nodes (20): -acceptsFirstResponder, -addTrackRect, -drawBarLinesInLaneAtheight, -drawClips, -drawRect, -drawRuler, -drawTracks, -gridPointFor (+12 more)

### Community 39 - "atomic"
Cohesion: 0.14
Nodes (5): atomic, MidiBuffer, array, Node, process

### Community 40 - "DelayLineNode"
Cohesion: 0.12
Nodes (16): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+8 more)

### Community 41 - "CompiledGraph"
Cohesion: 0.12
Nodes (14): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+6 more)

### Community 42 - "CountingCommand"
Cohesion: 0.12
Nodes (10): CountingCommand, counter_, delta_, Command, EntityId, string, Tick, makeProjectWithNotes() (+2 more)

### Community 43 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 44 - "WavStreamReader"
Cohesion: 0.12
Nodes (16): ifstream, FrameCount, path, SampleRate, size_t, uint16_t, uint64_t, uint8_t (+8 more)

### Community 45 - "MidiEvent"
Cohesion: 0.11
Nodes (19): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+11 more)

### Community 46 - "GraphCompileOptions"
Cohesion: 0.11
Nodes (19): PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, instrumentFactory, masterGain, maxBlockSize, parameters (+11 more)

### Community 47 - "AddAutomationLaneCommand"
Cohesion: 0.13
Nodes (12): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, EntityId, size_t (+4 more)

### Community 48 - "PlaylistModel"
Cohesion: 0.15
Nodes (11): EntityId, size_t, Tick, vector, Viewport, PlaylistModel, noClip, noTrack (+3 more)

### Community 49 - "Region"
Cohesion: 0.30
Nodes (16): applyGain(), applyRamp(), clampedRegion(), AudioFileData, Sample, fadeIn(), fadeOut(), FrameCount (+8 more)

### Community 50 - "AudioEngine.cpp"
Cohesion: 0.18
Nodes (17): audioDeviceAboutToStart, audioDeviceStopped, bufferSize, captureAudioBlock, collectRetiredGraphs, inputChannels, isRunning, maxServiceableBlockSize (+9 more)

### Community 51 - "TempoMap"
Cohesion: 0.15
Nodes (15): TempoMap, AudioBufferPool, AutomationPoint, Tick, enginePoint(), modelPoint(), EntityId, Step (+7 more)

### Community 52 - "AudioBufferView"
Cohesion: 0.20
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 53 - "ProcessContext"
Cohesion: 0.12
Nodes (15): FrameCount, FramePosition, MidiBuffer, SampleRate, size_t, ProcessContext, frameCount, inputCount (+7 more)

### Community 54 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 55 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 56 - "EditFixture"
Cohesion: 0.14
Nodes (15): AudioFileData, EntityId, FrameCount, Project, Sample, size_t, EditFixture, assetId (+7 more)

### Community 57 - "WavFile"
Cohesion: 0.25
Nodes (17): AudioAsset, assetFilePath(), AudioFileData, EntityId, Project, Sample, string, vector (+9 more)

### Community 58 - "main"
Cohesion: 0.14
Nodes (17): AudioDeviceInfo, availableDevices, midiInput_, profiler_, sampleRate, setGraph, transport_, CompiledGraph (+9 more)

### Community 59 - "RecordingSession"
Cohesion: 0.14
Nodes (12): AudioRecorder, string, Placement, string, FrameCount, uint64_t, RecordingSession, arm (+4 more)

### Community 60 - "AudioClipNode"
Cohesion: 0.13
Nodes (14): Node, ProcessContext, AudioClipNode, addClip, clips_, fetchScratch_, prepare, process (+6 more)

### Community 61 - "MixerCommands.cpp"
Cohesion: 0.16
Nodes (14): execute, Command, SetMixerPanCommand, canMergeWith, execute, mergeWith, nodeId_, pan_ (+6 more)

### Community 62 - "LevelMeter"
Cohesion: 0.14
Nodes (13): atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond (+5 more)

### Community 63 - "compile"
Cohesion: 0.16
Nodes (17): process, AudioBufferView, FrameCount, FramePosition, MidiBuffer, Node, SampleRate, size_t (+9 more)

### Community 64 - "InstrumentNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, unique_ptr, InstrumentNode, blockMidi_ (+6 more)

### Community 65 - "PluginIdentifier"
Cohesion: 0.14
Nodes (13): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+5 more)

### Community 66 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 67 - "MidiRecorder"
Cohesion: 0.14
Nodes (14): CapturedMessage, FramePosition, MidiBuffer, atomic, queueCapacity, size_t, uint64_t, MidiRecorder (+6 more)

### Community 68 - "vector"
Cohesion: 0.21
Nodes (4): Command, string, vector, Node

### Community 69 - "MoveNotesCommand"
Cohesion: 0.18
Nodes (11): EntityId, NoteIndices, Tick, MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, channel_, indices_ (+3 more)

### Community 70 - "PlaylistModel.cpp"
Cohesion: 0.21
Nodes (16): EntityId, size_t, vector, addToSelection, clipAtPoint, clipsInRectangle, collectVisibleClips, isOverResizeHandle (+8 more)

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

### Community 76 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 77 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 78 - "ioProcTrampoline"
Cohesion: 0.23
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 79 - "WavStreamWriter"
Cohesion: 0.13
Nodes (14): ofstream, Format, FrameCount, path, size_t, uint8_t, vector, WavStreamWriter (+6 more)

### Community 80 - "ResizeNotesCommand"
Cohesion: 0.13
Nodes (14): Command, vector, QuantizeNotesCommand, channel_, grid_, pattern_, previousEvents_, strength_ (+6 more)

### Community 81 - "noteAtStep"
Cohesion: 0.40
Nodes (5): size_t, Tick, vector, noteAtStep(), execute

### Community 82 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 83 - "EntityId"
Cohesion: 0.21
Nodes (7): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, Value

### Community 84 - "RoutingConnection"
Cohesion: 0.12
Nodes (14): EntityId, findRouting, ids_, metadata_, tempoMap_, RoutingConnection, destination, gain (+6 more)

### Community 85 - "MixerTests.cpp"
Cohesion: 0.17
Nodes (11): AudioBufferPool, AudioBufferView, ProcessContext, Sample, size_t, vector, channel, onsets() (+3 more)

### Community 86 - "AutomationPoint"
Cohesion: 0.17
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 87 - "ChannelCommands.cpp"
Cohesion: 0.17
Nodes (12): Command, execute, undo, undo, SetChannelVolumeCommand, canMergeWith, channelId_, execute (+4 more)

### Community 88 - "AddPatternClipCommand"
Cohesion: 0.15
Nodes (12): AddPatternClipCommand, clip_, execute, index_, length_, minted_, pattern_, start_ (+4 more)

### Community 89 - "PatternCommands.cpp"
Cohesion: 0.20
Nodes (12): Command, Tick, SetPatternLengthCommand, canMergeWith, execute, length_, mergeWith, patternId_ (+4 more)

### Community 90 - "TrackCommands.cpp"
Cohesion: 0.17
Nodes (12): Command, execute, undo, undo, SetTrackHeightCommand, canMergeWith, execute, height_ (+4 more)

### Community 91 - "open"
Cohesion: 0.28
Nodes (12): Format, FrameCount, path, Result, Sample, SampleRate, size_t, append (+4 more)

### Community 92 - "renderNode"
Cohesion: 0.19
Nodes (12): AudioFileData, shared_ptr, vector, FrameCount, Node, Sample, size_t, vector (+4 more)

### Community 93 - "-applicationDidFinishLaunching"
Cohesion: 0.20
Nodes (14): INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, NSApplicationDelegate, NSObject, NSScrollView, NSSegmentedControl (+6 more)

### Community 94 - "WaveformOverview"
Cohesion: 0.16
Nodes (12): SampleRate, Bucket, FrameCount, size_t, vector, sizeBuckets(), WaveformOverview, channelCount (+4 more)

### Community 95 - "vector"
Cohesion: 0.15
Nodes (6): Command, execute, id, name, undo, vector

### Community 96 - "string"
Cohesion: 0.16
Nodes (8): AddChannelCommand, channel_, execute, index_, minted_, undo, size_t, string

### Community 97 - "EntityId"
Cohesion: 0.16
Nodes (11): ConnectMixerCommand, connection_, destination_, gain_, index_, isSend_, minted_, preFader_ (+3 more)

### Community 98 - "SetVelocityCommand"
Cohesion: 0.14
Nodes (8): string, SetVelocityCommand, channel_, indices_, pattern_, previousVelocities_, undo, velocity_

### Community 99 - "AddPatternCommand"
Cohesion: 0.16
Nodes (8): AddPatternCommand, execute, index_, minted_, pattern_, undo, size_t, string

### Community 100 - "DuplicatePatternCommand"
Cohesion: 0.16
Nodes (8): DuplicatePatternCommand, execute, index_, minted_, pattern_, source_, undo, EntityId

### Community 101 - "ToggleStepCommand"
Cohesion: 0.15
Nodes (9): Command, size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_ (+1 more)

### Community 102 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 103 - "renderArrangement"
Cohesion: 0.19
Nodes (12): AudioFileData, FrameCount, path, Project, Sample, size_t, vector, makeAudio() (+4 more)

### Community 104 - "Fixture"
Cohesion: 0.16
Nodes (12): EntityId, SequencedNote, Tick, vector, Fixture, channel, pattern, project (+4 more)

### Community 105 - "Options"
Cohesion: 0.14
Nodes (14): int64_t, Options, amplitude, buffer, device, frequency, input, listOnly (+6 more)

### Community 106 - "DuplicateClipsCommand"
Cohesion: 0.21
Nodes (9): ClipIds, DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_ (+1 more)

### Community 107 - "MixerNode"
Cohesion: 0.15
Nodes (13): MixerNodeType, MixerNode, colour, id, inserts, muted, name, pan (+5 more)

### Community 108 - "RemoveMixerNodeCommand"
Cohesion: 0.15
Nodes (10): RemovedRouting, vector, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_ (+2 more)

### Community 109 - "EntityId"
Cohesion: 0.17
Nodes (7): EntityId, SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 110 - "ClipCommands.cpp"
Cohesion: 0.26
Nodes (11): Command, EntityId, execute, undo, canMergeWith, execute, mergeWith, undo (+3 more)

### Community 111 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 112 - "ParameterRegistry"
Cohesion: 0.22
Nodes (11): Entry, string, Entry, size_t, vector, ParameterRegistry, entries_, find (+3 more)

### Community 113 - "main.mm"
Cohesion: 0.23
Nodes (12): -editorChanged, -openAudioAssetInEditor, -selectChannel, -selectPattern, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll (+4 more)

### Community 114 - "humanizeNoteStarts"
Cohesion: 0.26
Nodes (11): Kind, RecordedEvent, appendRecordedEvents(), MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts() (+3 more)

### Community 115 - "AutomationCommands.cpp"
Cohesion: 0.21
Nodes (11): execute, undo, AutomationPoint, EntityId, vector, findLane(), execute, undo (+3 more)

### Community 116 - "SetAutomationPointsCommand"
Cohesion: 0.21
Nodes (9): Command, AutomationPoint, vector, SetAutomationPointsCommand, canMergeWith, laneId_, mergeWith, points_ (+1 more)

### Community 117 - "AddMixerNodeCommand"
Cohesion: 0.18
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 118 - "RenameTrackCommand"
Cohesion: 0.20
Nodes (6): Command, string, RenameTrackCommand, execute, previousName_, trackId_

### Community 119 - "EntityId"
Cohesion: 0.18
Nodes (6): EntityId, SetTrackSoloedCommand, execute, soloed_, trackId_, undo

### Community 120 - "Track"
Cohesion: 0.17
Nodes (12): trackHeight, Track, colour, height, id, muted, name, outputMixerNode (+4 more)

### Community 121 - "Instrument"
Cohesion: 0.18
Nodes (9): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+1 more)

### Community 122 - "WavStreamWriterTests.cpp"
Cohesion: 0.18
Nodes (10): FrameCount, path, size_t, uint8_t, vector, fileBytes(), makeTestSignal(), ScratchFile (+2 more)

### Community 123 - "AutomationNode"
Cohesion: 0.20
Nodes (7): Binding, AutomationNode, bindings_, tempoMap_, ProcessContext, size_t, vector

### Community 124 - "Denormals.h"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 125 - "AddNoteCommand"
Cohesion: 0.20
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, size_t

### Community 126 - "captureAudioBlock"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 127 - "AudioFileData"
Cohesion: 0.18
Nodes (10): AudioFileData, channelCount, channels, frameCount, sampleRate, FrameCount, Sample, SampleRate (+2 more)

### Community 128 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 129 - "MixerStripNode.cpp"
Cohesion: 0.31
Nodes (10): FrameCount, Sample, SampleRate, panGains, prepare, refreshTargets, setGain, setMuted (+2 more)

### Community 130 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 131 - "BlockSegment"
Cohesion: 0.18
Nodes (9): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, size_t (+1 more)

### Community 132 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 133 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 134 - "RemoveClipsCommand"
Cohesion: 0.20
Nodes (8): string, RemovedClip, RemoveClipsCommand, clips_, execute, name, removed_, undo

### Community 135 - "string"
Cohesion: 0.24
Nodes (6): string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 136 - "DisconnectMixerCommand"
Cohesion: 0.20
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 137 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (8): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo

### Community 138 - "readAt"
Cohesion: 0.22
Nodes (8): FrameCount, path, Result, Sample, size_t, close, open, readAt

### Community 139 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 140 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 141 - "DiskStreamer"
Cohesion: 0.31
Nodes (8): atomic, DiskStreamer, mutex_, running_, streams_, thread_, vector, weak_ptr

### Community 142 - "SetClipMutedCommand"
Cohesion: 0.22
Nodes (7): vector, SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 143 - "SetMixerPolarityCommand"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 144 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 145 - "SetMixerVolumeCommand"
Cohesion: 0.22
Nodes (6): SetMixerVolumeCommand, execute, nodeId_, previous_, undo, volume_

### Community 146 - "SetSendGainCommand"
Cohesion: 0.22
Nodes (6): SetSendGainCommand, connectionId_, execute, gain_, previous_, undo

### Community 147 - "RemovePatternCommand"
Cohesion: 0.22
Nodes (6): RemovePatternCommand, execute, index_, pattern_, patternId_, undo

### Community 148 - "MidiDevice.h"
Cohesion: 0.22
Nodes (7): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback, midiMessageReceived

### Community 149 - "AutomationFixture"
Cohesion: 0.22
Nodes (7): AutomationFixture, channel, pattern, project, tempo, EntityId, TempoMap

### Community 150 - "RemoveChannelCommand"
Cohesion: 0.25
Nodes (7): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, index_

### Community 151 - "build"
Cohesion: 0.29
Nodes (8): Result, bucketize(), AudioFileData, Bucket, FrameCount, Sample, vector, build

### Community 152 - "RenameChannelCommand"
Cohesion: 0.25
Nodes (5): RenameChannelCommand, channelId_, execute, previousName_, undo

### Community 153 - "SetChannelMutedCommand"
Cohesion: 0.25
Nodes (5): SetChannelMutedCommand, channelId_, execute, muted_, undo

### Community 154 - "MoveClipsCommand"
Cohesion: 0.25
Nodes (7): Command, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, tickDelta_, trackDelta_

### Community 155 - "Command"
Cohesion: 0.22
Nodes (6): Command, SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 156 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 157 - "SetPatternSwingCommand"
Cohesion: 0.25
Nodes (6): SetPatternSwingCommand, execute, patternId_, previousSwing_, swing_, undo

### Community 158 - "AddTrackCommand"
Cohesion: 0.25
Nodes (7): AddTrackCommand, execute, index_, minted_, track_, undo, size_t

### Community 159 - "RemoveTrackCommand"
Cohesion: 0.25
Nodes (7): RemovedClip, vector, RemoveTrackCommand, clips_, index_, track_, trackId_

### Community 160 - "SetTrackMutedCommand"
Cohesion: 0.25
Nodes (5): SetTrackMutedCommand, execute, muted_, trackId_, undo

### Community 161 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 162 - "TimestampedMidiMessage"
Cohesion: 0.25
Nodes (8): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status

### Community 163 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, MidiMessage, SampleRate, uint64_t, nanosForFrame(), timestamped(), TimestampedMidiMessage

### Community 164 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 165 - "ProjectGraphCompiler.h"
Cohesion: 0.33
Nodes (5): Instrument, ParameterRegistry, vector, string, TempoMap

### Community 166 - "SetChannelSoloedCommand"
Cohesion: 0.29
Nodes (5): Command, SetChannelSoloedCommand, channelId_, execute, soloed_

### Community 167 - "ResizeClipsCommand"
Cohesion: 0.29
Nodes (6): ResizeClipsCommand, clips_, execute, lengthDelta_, previousLengths_, undo

### Community 168 - "RenamePatternCommand"
Cohesion: 0.29
Nodes (6): Command, RenamePatternCommand, execute, patternId_, previousName_, undo

### Community 169 - "AudioEditorView.mm"
Cohesion: 0.29
Nodes (6): -acceptsFirstResponder, -hasSelection, -initWithFrameprojectregistry, -isFlipped, -selectionFrom, -selectionTo

### Community 170 - "MixerFixture"
Cohesion: 0.29
Nodes (6): EntityId, TempoMap, MixerFixture, pattern, project, tempo

### Community 171 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 172 - "INCDAWMixerView"
Cohesion: 0.33
Nodes (5): incdaw, NSView, INCDAWMixerView, -initWithFrameprojectregistry, stripLookup

### Community 173 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 175 - "collectForRange"
Cohesion: 0.33
Nodes (4): FrameCount, FramePosition, MidiBuffer, collectForRange

### Community 176 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 177 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 178 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 179 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 180 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 181 - "start"
Cohesion: 0.50
Nodes (4): AudioDeviceConfig, deviceName, start, string

### Community 183 - "INCDAWChannelRackView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWChannelRackView, -initWithFrameprojectregistry

### Community 184 - "INCDAWPatternListView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWPatternListView, -initWithFrameprojectregistry

### Community 185 - "INCDAWPlaylistView"
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
- **794 isolated node(s):** `bitsPerSample`, `channels`, `format`, `sampleRate`, `bitsPerSample` (+789 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **33 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `RemoveClipsCommand`, `CommandRegistry`, `DisconnectMixerCommand`, `string`, `DeleteNotesCommand`, `Pattern`, `Clip`, `SetClipMutedCommand`, `SetMixerPolarityCommand`, `SetChannelOutputCommand`, `SetMixerVolumeCommand`, `SetSendGainCommand`, `RemovePatternCommand`, `Channel`, `AutomationFixture`, `RenameChannelCommand`, `SetChannelMutedCommand`, `Command`, `SetMixerSoloedCommand`, `SetPatternSwingCommand`, `AddTrackCommand`, `ProjectFile.cpp`, `SetTrackMutedCommand`, `NoteCommands.cpp`, `compileProjectGraph`, `InsertRecordedTakeCommand`, `SetChannelSoloedCommand`, `ResizeClipsCommand`, `RenamePatternCommand`, `CountingCommand`, `MixerFixture`, `clipRect`, `MixerCommands.cpp`, `PlaylistModel.cpp`, `noteAtStep`, `RoutingConnection`, `ChannelCommands.cpp`, `AddPatternClipCommand`, `PatternCommands.cpp`, `TrackCommands.cpp`, `string`, `EntityId`, `SetVelocityCommand`, `AddPatternCommand`, `DuplicatePatternCommand`, `Fixture`, `MixerNode`, `RemoveMixerNodeCommand`, `EntityId`, `ClipCommands.cpp`, `AudioAsset`, `AutomationCommands.cpp`, `AddMixerNodeCommand`, `RenameTrackCommand`, `EntityId`, `Track`, `AddNoteCommand`?**
  _High betweenness centrality (0.265) - this node is a cross-community bridge._