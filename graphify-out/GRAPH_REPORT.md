# Graph Report - project-continuation-670d11  (2026-08-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 3380 nodes · 5722 edges · 232 communities (194 shown, 38 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 264 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `41de42e5`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- PatternCommands.cpp
- RemoveTrackCommand
- Transport
- WavBytes.h
- PluginRegistry.cpp
- AudioBufferPool
- CoreAudioDevice
- PlaylistModel
- SimpleSynth
- Project
- CommandRegistry
- CompiledProjectGraph
- MixerNode
- AudioEngine
- main
- EditAssetRegionCommand
- ChannelRackModel
- AudioLogger
- RecordingSession
- TempoMap
- InstrumentNode
- Clip
- WavStreamReader
- Json
- MixerStripNode
- AudioDeviceConfig
- CoreMidiDevice
- MixerView.mm
- AudioStream
- Json.cpp
- CoreAudioDevice.cpp
- NoteSequence
- InsertRecordedTakeCommand
- atomic
- MidiMessage
- PlaylistView.mm
- AudioRecorder
- MidiInput
- Options
- TestGainPlugin.cpp
- GraphCompileOptions
- ProjectFile.cpp
- CallbackProfiler
- TimeSignature
- MetronomeNode
- AudioDevice
- WaveformOverview
- WriteAutomationCommand
- GraphBuilder
- ResizeClipsCommand
- NoteCommands.cpp
- compileArrangement
- AddAutomationLaneCommand
- DelayLineNode
- CompiledGraph
- CountingCommand
- RealtimeGuard.cpp
- AutomationWriteSession
- Region
- BasicMidiBuffer
- SystemInfo
- EditFixture
- WavFile
- friend
- AudioClipNode
- MixerCommands.cpp
- AudioBufferView
- LevelMeter
- compile
- PluginIdentifier
- ConstantNode
- MidiRecorder
- PianoRollModel.cpp
- ClipCommands.cpp
- Pattern
- PianoRollModel
- LockFreeQueue
- SampleRingBuffer
- Smoother
- LoopbackResult
- LatentProcessorNode
- ioProcTrampoline
- -applicationDidFinishLaunching
- WavStreamWriter
- AutomationCommands.cpp
- SetVelocityCommand
- GainNode
- SineOscillatorNode
- EntityId
- MixerTests.cpp
- TimingProbeInstrument
- PluginNode
- MidiEvent
- ChannelCommands.cpp
- open
- ProcessContext
- DuplicateClipsCommand
- MoveClipsCommand
- DiskStreamer
- vector
- string
- EntityId
- string
- EntityId
- QuantizeNotesCommand
- Track
- ParameterRegistry
- ChannelRackView.mm
- renderArrangement
- Fixture
- AutomationPoint
- AutomationNode
- AddPatternClipCommand
- ToggleStepCommand
- InputMonitorNode
- ClapLibrary.cpp
- Channel
- main.mm
- humanizeNoteStarts
- RemoveMixerNodeCommand
- string
- AudioBufferPool
- WavStreamWriterTests.cpp
- Denormals.h
- TimelineAnchor
- MidiTests.cpp
- AddMixerNodeCommand
- AddNoteCommand
- vector
- captureAudioBlock
- AudioFileData
- TimeSignatureEvent
- MidiDevice
- AudioAsset
- PatternListView.mm
- DisconnectMixerCommand
- DeleteNotesCommand
- RecordedEvent
- SharedLibrary
- ScanOutcome
- ClapDescriptor
- RoutingConnection
- Fixture
- make-dmg.sh
- SetChannelOutputCommand
- SetMixerPanCommand
- SetMixerVolumeCommand
- MoveNotesCommand
- MidiDevice.h
- ClapInstance
- PianoRollView.mm
- AutomationFixture
- RemoveClipsCommand
- RemoveChannelCommand
- RenameChannelCommand
- SetChannelSoloedCommand
- Command
- SetMixerSoloedCommand
- MixerCommands.h
- ResizeNotesCommand
- Version
- TimestampedMidiMessage
- ProjectMetadata
- makeTestSignal
- ClapLibrary
- AudioEditorView.mm
- MixerFixture
- ScratchDirectory
- emptyInGet
- INCDAWMixerView
- INCDAWPianoRollView
- InstrumentTests.cpp
- SetChannelStepKeyCommand
- process
- collectForRange
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- renderBlock
- OrderRecordingNode
- INCDAWAudioEditorView
- INCDAWPlaylistView
- AutomationTests.cpp
- check
- Command
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
- path
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

## Communities (232 total, 38 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "PatternCommands.cpp"
Cohesion: 0.05
Nodes (46): AddPatternCommand, execute, index_, minted_, pattern_, undo, Command, DuplicatePatternCommand (+38 more)

### Community 2 - "RemoveTrackCommand"
Cohesion: 0.05
Nodes (43): AddTrackCommand, execute, index_, minted_, track_, undo, Command, Command (+35 more)

### Community 3 - "Transport"
Cohesion: 0.06
Nodes (38): atomic, MusicalPosition, BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount (+30 more)

### Community 4 - "WavBytes.h"
Cohesion: 0.09
Nodes (45): appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample, channels (+37 more)

### Community 5 - "PluginRegistry.cpp"
Cohesion: 0.07
Nodes (38): ClapDescriptor, End, Library, Located, ScanOutcome, size_t, ChildResult, code (+30 more)

### Community 6 - "AudioBufferPool"
Cohesion: 0.06
Nodes (31): ParameterRegistry, AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t (+23 more)

### Community 7 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 8 - "PlaylistModel"
Cohesion: 0.09
Nodes (35): Rect, Clip, EntityId, Project, size_t, Tick, Track, vector (+27 more)

### Community 9 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 10 - "Project"
Cohesion: 0.11
Nodes (39): IdGenerator, EntityId, size_t, vector, operator==(), events, totalEventCount, Project (+31 more)

### Community 11 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 12 - "CompiledProjectGraph"
Cohesion: 0.09
Nodes (31): AutomationNode, Channel, Instrument, CompiledProjectGraph, automation, channels, channelStripFor, channelStrips (+23 more)

### Community 13 - "MixerNode"
Cohesion: 0.08
Nodes (31): MixerNodeType, PluginIdentifier, AutomationLane, id, parameterKey, points, targetEntity, EntityId (+23 more)

### Community 14 - "AudioEngine"
Cohesion: 0.08
Nodes (26): AudioDevice, AudioIOCallback, CallbackProfiler, MidiBuffer, mutex, RetiredGraph, SampleRingBuffer, AudioCaptureSink (+18 more)

### Community 15 - "main"
Cohesion: 0.11
Nodes (29): AudioDeviceConfig, int64_t, audioDeviceAboutToStart, audioDeviceStopped, bufferSize, captureAudioBlock, collectRetiredGraphs, deviceName (+21 more)

### Community 16 - "EditAssetRegionCommand"
Cohesion: 0.08
Nodes (24): AudioEditOp, Command, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_ (+16 more)

### Community 17 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 18 - "AudioLogger"
Cohesion: 0.08
Nodes (24): AudioFileData, AudioLogger, capacityFrames_, circle_, enabled_, grab, log, prepare (+16 more)

### Community 19 - "RecordingSession"
Cohesion: 0.09
Nodes (23): AudioEngine, AudioRecorder, path, Slice, Placement, string, vector, FrameCount (+15 more)

### Community 20 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 21 - "InstrumentNode"
Cohesion: 0.08
Nodes (23): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+15 more)

### Community 22 - "Clip"
Cohesion: 0.07
Nodes (29): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+21 more)

### Community 23 - "WavStreamReader"
Cohesion: 0.09
Nodes (24): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+16 more)

### Community 24 - "Json"
Cohesion: 0.08
Nodes (18): nullptr_t, pair, int64_t, int64_t, string, vector, Json, append (+10 more)

### Community 25 - "MixerStripNode"
Cohesion: 0.11
Nodes (21): FrameCount, ProcessContext, Sample, SampleRate, atomic, Node, Sample, MixerStripNode (+13 more)

### Community 26 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 27 - "CoreMidiDevice"
Cohesion: 0.14
Nodes (24): CFStringRef, MIDIClientRef, MIDIEndpointRef, MIDIObjectRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_ (+16 more)

### Community 28 - "MixerView.mm"
Cohesion: 0.17
Nodes (26): MixerStripNode, NSMenu, -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect (+18 more)

### Community 29 - "AudioStream"
Cohesion: 0.11
Nodes (22): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+14 more)

### Community 30 - "Json.cpp"
Cohesion: 0.19
Nodes (22): size_t, string, escapeInto(), formatDouble(), asString, contains, dump, dumpTo (+14 more)

### Community 31 - "CoreAudioDevice.cpp"
Cohesion: 0.29
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 32 - "NoteSequence"
Cohesion: 0.11
Nodes (21): Tick, vector, size_t, Tick, uint32_t, vector, NoteSequence, byEnd_ (+13 more)

### Community 33 - "InsertRecordedTakeCommand"
Cohesion: 0.09
Nodes (20): Clip, EntityId, Project, Placement, size_t, string, vector, InsertRecordedTakeCommand (+12 more)

### Community 34 - "atomic"
Cohesion: 0.11
Nodes (7): atomic, MidiBuffer, array, FrameCount, SampleRate, Node, process

### Community 35 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 36 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (24): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+16 more)

### Community 37 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

### Community 38 - "MidiInput"
Cohesion: 0.11
Nodes (19): FrameCount, MidiBuffer, SampleRate, uint64_t, atomic, queueCapacity, size_t, uint64_t (+11 more)

### Community 39 - "Options"
Cohesion: 0.09
Nodes (22): AudioDeviceInfo, availableDevices, vector, CallbackProfiler, int64_t, string, Options, amplitude (+14 more)

### Community 40 - "TestGainPlugin.cpp"
Cohesion: 0.13
Nodes (19): clap_plugin_descriptor_t, clap_process_status, clap_process_t, clap_host_t, clap_plugin_factory_t, clap_plugin_t, factoryCreatePlugin(), factoryGetPluginCount() (+11 more)

### Community 41 - "GraphCompileOptions"
Cohesion: 0.09
Nodes (23): DiskStreamer, PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, instrumentFactory, masterGain, maxBlockSize (+15 more)

### Community 42 - "ProjectFile.cpp"
Cohesion: 0.23
Nodes (21): Json, automationPointFrom(), bindUnassignedContent(), EntityId, path, PluginIdentifier, Result, string (+13 more)

### Community 43 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 44 - "TimeSignature"
Cohesion: 0.13
Nodes (16): Tick, friend, int64_t, Tick, MusicalPosition, bar, beat, tick (+8 more)

### Community 45 - "MetronomeNode"
Cohesion: 0.09
Nodes (17): atomic, FrameCount, Sample, SampleRate, size_t, vector, MetronomeNode, amplitude_ (+9 more)

### Community 46 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 47 - "WaveformOverview"
Cohesion: 0.12
Nodes (20): Result, SampleRate, bucketize(), AudioFileData, Bucket, FrameCount, Sample, vector (+12 more)

### Community 48 - "WriteAutomationCommand"
Cohesion: 0.10
Nodes (19): AutomationPoint, Clip, Track, vector, WriteAutomationCommand, clip_, clipIndex_, key_ (+11 more)

### Community 49 - "GraphBuilder"
Cohesion: 0.11
Nodes (16): Connection, NodeIndex, GraphBuilder, compensate_, connect, connections_, error_, master_ (+8 more)

### Community 50 - "ResizeClipsCommand"
Cohesion: 0.11
Nodes (12): Command, FrameCount, string, ResizeClipsCommand, clips_, lengthDelta_, previousFrameLengths_, previousLengths_ (+4 more)

### Community 51 - "NoteCommands.cpp"
Cohesion: 0.17
Nodes (20): undo, Command, EntityId, NoteIndices, size_t, vector, execute, findEvents() (+12 more)

### Community 52 - "compileArrangement"
Cohesion: 0.23
Nodes (19): Emit, NoteSequence, content, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto() (+11 more)

### Community 53 - "AddAutomationLaneCommand"
Cohesion: 0.13
Nodes (13): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, AutomationLane, EntityId (+5 more)

### Community 54 - "DelayLineNode"
Cohesion: 0.12
Nodes (16): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+8 more)

### Community 55 - "CompiledGraph"
Cohesion: 0.12
Nodes (14): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+6 more)

### Community 56 - "CountingCommand"
Cohesion: 0.12
Nodes (10): CountingCommand, counter_, delta_, Command, EntityId, string, Tick, makeProjectWithNotes() (+2 more)

### Community 57 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 58 - "AutomationWriteSession"
Cohesion: 0.13
Nodes (14): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, Command, EntityId (+6 more)

### Community 59 - "Region"
Cohesion: 0.30
Nodes (16): applyGain(), applyRamp(), clampedRegion(), AudioFileData, Sample, fadeIn(), fadeOut(), FrameCount (+8 more)

### Community 60 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 61 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 62 - "EditFixture"
Cohesion: 0.14
Nodes (15): AudioFileData, EntityId, FrameCount, Project, Sample, size_t, EditFixture, assetId (+7 more)

### Community 63 - "WavFile"
Cohesion: 0.25
Nodes (17): AudioAsset, assetFilePath(), AudioFileData, EntityId, Project, Sample, string, vector (+9 more)

### Community 64 - "friend"
Cohesion: 0.11
Nodes (8): AutomationCurve, friend, AutomationPoint, curve, tension, tick, value, Tick

### Community 65 - "AudioClipNode"
Cohesion: 0.13
Nodes (14): PlacedClip, vector, AudioClipNode, addClip, clips_, fetchScratch_, prepare, process (+6 more)

### Community 66 - "MixerCommands.cpp"
Cohesion: 0.16
Nodes (14): undo, Command, canMergeWith, mergeWith, canMergeWith, mergeWith, SetSendGainCommand, canMergeWith (+6 more)

### Community 67 - "AudioBufferView"
Cohesion: 0.22
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 68 - "LevelMeter"
Cohesion: 0.14
Nodes (13): atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond (+5 more)

### Community 69 - "compile"
Cohesion: 0.16
Nodes (17): process, AudioBufferView, FrameCount, FramePosition, MidiBuffer, Node, SampleRate, size_t (+9 more)

### Community 70 - "PluginIdentifier"
Cohesion: 0.14
Nodes (13): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+5 more)

### Community 71 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 72 - "MidiRecorder"
Cohesion: 0.14
Nodes (14): CapturedMessage, FramePosition, MidiBuffer, atomic, queueCapacity, size_t, uint64_t, MidiRecorder (+6 more)

### Community 73 - "PianoRollModel.cpp"
Cohesion: 0.25
Nodes (13): NoteList, size_t, vector, addToSelection, collectVisibleNotes, isOverResizeHandle, isSelected, noteAtPoint (+5 more)

### Community 74 - "ClipCommands.cpp"
Cohesion: 0.24
Nodes (15): execute, undo, EntityId, Project, execute, undo, execute, undo (+7 more)

### Community 75 - "Pattern"
Cohesion: 0.12
Nodes (16): size_t, Tick, vector, noteAtStep(), execute, undo, Pattern, automationLanes (+8 more)

### Community 76 - "PianoRollModel"
Cohesion: 0.16
Nodes (11): Tick, size_t, Tick, Viewport, PianoRollModel, noNote, resizeHandleWidth, selection_ (+3 more)

### Community 77 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 78 - "SampleRingBuffer"
Cohesion: 0.21
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 79 - "Smoother"
Cohesion: 0.18
Nodes (10): atomic, AudioBufferView, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds (+2 more)

### Community 80 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 81 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 82 - "ioProcTrampoline"
Cohesion: 0.23
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 83 - "-applicationDidFinishLaunching"
Cohesion: 0.18
Nodes (16): INCDAWAudioEditorView, INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, INCDAWPlaylistView, NSApplicationDelegate, NSObject (+8 more)

### Community 84 - "WavStreamWriter"
Cohesion: 0.13
Nodes (14): ofstream, Format, FrameCount, path, size_t, uint8_t, vector, WavStreamWriter (+6 more)

### Community 85 - "AutomationCommands.cpp"
Cohesion: 0.22
Nodes (15): execute, undo, AutomationLane, AutomationPoint, EntityId, Project, vector, findLane() (+7 more)

### Community 86 - "SetVelocityCommand"
Cohesion: 0.18
Nodes (10): EntityId, NoteIndices, Tick, SetVelocityCommand, channel_, indices_, pattern_, previousVelocities_ (+2 more)

### Community 87 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 88 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 89 - "EntityId"
Cohesion: 0.21
Nodes (7): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, Value

### Community 90 - "MixerTests.cpp"
Cohesion: 0.17
Nodes (11): AudioBufferPool, AudioBufferView, ProcessContext, Sample, size_t, vector, channel, onsets() (+3 more)

### Community 91 - "TimingProbeInstrument"
Cohesion: 0.15
Nodes (9): Applied, AudioBufferView, FrameCount, MidiMessage, SampleRate, vector, TimingProbeInstrument, applied (+1 more)

### Community 92 - "PluginNode"
Cohesion: 0.16
Nodes (7): ClapInstance, Node, ProcessContext, SampleRingBuffer, PluginNode, instance_, unique_ptr

### Community 93 - "MidiEvent"
Cohesion: 0.13
Nodes (15): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+7 more)

### Community 94 - "ChannelCommands.cpp"
Cohesion: 0.17
Nodes (12): Command, execute, undo, execute, SetChannelVolumeCommand, canMergeWith, channelId_, execute (+4 more)

### Community 95 - "open"
Cohesion: 0.28
Nodes (12): Format, FrameCount, path, Result, Sample, SampleRate, size_t, append (+4 more)

### Community 96 - "ProcessContext"
Cohesion: 0.14
Nodes (13): FramePosition, MidiBuffer, size_t, ProcessContext, frameCount, inputCount, inputs, liveMidi (+5 more)

### Community 97 - "DuplicateClipsCommand"
Cohesion: 0.19
Nodes (10): ClipIds, DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_ (+2 more)

### Community 98 - "MoveClipsCommand"
Cohesion: 0.15
Nodes (13): MovedAudioClip, Command, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, canMergeWith, clips_, mergeWith (+5 more)

### Community 99 - "DiskStreamer"
Cohesion: 0.18
Nodes (11): shared_ptr, vector, DiskStreamer, add, mutex_, running_, serviceOnce, streams_ (+3 more)

### Community 100 - "vector"
Cohesion: 0.15
Nodes (6): Command, execute, id, name, undo, vector

### Community 101 - "string"
Cohesion: 0.16
Nodes (8): AddChannelCommand, channel_, execute, index_, minted_, undo, size_t, string

### Community 102 - "EntityId"
Cohesion: 0.17
Nodes (6): EntityId, SetChannelMutedCommand, channelId_, execute, muted_, undo

### Community 103 - "string"
Cohesion: 0.16
Nodes (6): string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 104 - "EntityId"
Cohesion: 0.16
Nodes (11): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+3 more)

### Community 105 - "QuantizeNotesCommand"
Cohesion: 0.14
Nodes (8): string, QuantizeNotesCommand, channel_, grid_, pattern_, previousEvents_, strength_, undo

### Community 106 - "Track"
Cohesion: 0.14
Nodes (14): process, findTrack, Track, colour, height, id, muted, name (+6 more)

### Community 107 - "ParameterRegistry"
Cohesion: 0.20
Nodes (11): Entry, string, Entry, size_t, vector, ParameterRegistry, entries_, find (+3 more)

### Community 108 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 109 - "renderArrangement"
Cohesion: 0.19
Nodes (12): AudioFileData, FrameCount, path, Project, Sample, size_t, vector, makeAudio() (+4 more)

### Community 110 - "Fixture"
Cohesion: 0.16
Nodes (12): EntityId, SequencedNote, Tick, vector, Fixture, channel, pattern, project (+4 more)

### Community 111 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 112 - "AutomationNode"
Cohesion: 0.19
Nodes (7): Binding, AutomationNode, bindings_, tempoMap_, size_t, TempoMap, vector

### Community 113 - "AddPatternClipCommand"
Cohesion: 0.18
Nodes (10): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+2 more)

### Community 114 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (9): Command, size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_ (+1 more)

### Community 115 - "InputMonitorNode"
Cohesion: 0.18
Nodes (10): FrameCount, Sample, SampleRate, SampleRingBuffer, size_t, vector, InputMonitorNode, channelCount_ (+2 more)

### Community 116 - "ClapLibrary.cpp"
Cohesion: 0.26
Nodes (11): close, create, open, clap_host_t, path, string, hostGetExtension(), hostRequestCallback() (+3 more)

### Community 117 - "Channel"
Cohesion: 0.15
Nodes (13): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+5 more)

### Community 118 - "main.mm"
Cohesion: 0.23
Nodes (12): -editorChanged, -openAudioAssetInEditor, -selectChannel, -selectPattern, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll (+4 more)

### Community 119 - "humanizeNoteStarts"
Cohesion: 0.26
Nodes (11): Kind, RecordedEvent, appendRecordedEvents(), MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts() (+3 more)

### Community 120 - "RemoveMixerNodeCommand"
Cohesion: 0.17
Nodes (10): RemovedRouting, vector, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_ (+2 more)

### Community 121 - "string"
Cohesion: 0.21
Nodes (8): Command, string, SetAutomationPointsCommand, canMergeWith, laneId_, mergeWith, points_, previous_

### Community 122 - "AudioBufferPool"
Cohesion: 0.23
Nodes (8): Node, AudioBufferPool, SequencedNote, Tick, vector, note(), shapeOf(), startsOf()

### Community 123 - "WavStreamWriterTests.cpp"
Cohesion: 0.18
Nodes (10): FrameCount, path, size_t, uint8_t, vector, fileBytes(), makeTestSignal(), ScratchFile (+2 more)

### Community 124 - "Denormals.h"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 125 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 126 - "MidiTests.cpp"
Cohesion: 0.20
Nodes (10): MidiInput, FrameCount, MidiMessage, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote() (+2 more)

### Community 127 - "AddMixerNodeCommand"
Cohesion: 0.20
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 128 - "AddNoteCommand"
Cohesion: 0.20
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, Command, size_t

### Community 129 - "vector"
Cohesion: 0.18
Nodes (6): vector, EntityId, Step, Tick, note(), stepAt()

### Community 130 - "captureAudioBlock"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 131 - "AudioFileData"
Cohesion: 0.18
Nodes (10): AudioFileData, channelCount, channels, frameCount, sampleRate, FrameCount, Sample, SampleRate (+2 more)

### Community 132 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 133 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 134 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 135 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 136 - "DisconnectMixerCommand"
Cohesion: 0.20
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 137 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (9): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo (+1 more)

### Community 138 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 139 - "SharedLibrary"
Cohesion: 0.33
Nodes (7): path, string, SharedLibrary, close, handle_, open, symbol

### Community 140 - "ScanOutcome"
Cohesion: 0.27
Nodes (7): string, vector, ScanOutcome, detail, plugins, status, Status

### Community 141 - "ClapDescriptor"
Cohesion: 0.20
Nodes (9): ClapDescriptor, id, name, vendor, version, descriptors, vector, string (+1 more)

### Community 142 - "RoutingConnection"
Cohesion: 0.20
Nodes (9): findRouting, RoutingConnection, destination, gain, id, isSend, preFader, sidechain (+1 more)

### Community 143 - "Fixture"
Cohesion: 0.20
Nodes (8): EntityId, Project, Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 144 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 145 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 146 - "SetMixerPanCommand"
Cohesion: 0.22
Nodes (6): SetMixerPanCommand, execute, nodeId_, pan_, previous_, undo

### Community 147 - "SetMixerVolumeCommand"
Cohesion: 0.22
Nodes (6): SetMixerVolumeCommand, execute, nodeId_, previous_, undo, volume_

### Community 148 - "MoveNotesCommand"
Cohesion: 0.22
Nodes (8): MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, channel_, indices_, keyDelta_, pattern_, tickDelta_

### Community 149 - "MidiDevice.h"
Cohesion: 0.22
Nodes (7): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback, midiMessageReceived

### Community 150 - "ClapInstance"
Cohesion: 0.25
Nodes (7): ClapInstance, host_, plugin_, processing_, steadyTime_, clap_host_t, clap_plugin_t

### Community 151 - "PianoRollView.mm"
Cohesion: 0.25
Nodes (8): -acceptsFirstResponder, -currentNotes, -displayLinkFired, -initWithFrameprojectregistry, -isFlipped, -setNeedsDisplay, -updateDrawableSize, -viewDidMoveToWindow

### Community 152 - "AutomationFixture"
Cohesion: 0.22
Nodes (7): AutomationFixture, channel, pattern, project, tempo, EntityId, TempoMap

### Community 153 - "RemoveClipsCommand"
Cohesion: 0.25
Nodes (6): RemovedClip, string, RemoveClipsCommand, clips_, name, removed_

### Community 154 - "RemoveChannelCommand"
Cohesion: 0.25
Nodes (7): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, index_

### Community 155 - "RenameChannelCommand"
Cohesion: 0.25
Nodes (5): RenameChannelCommand, channelId_, execute, previousName_, undo

### Community 156 - "SetChannelSoloedCommand"
Cohesion: 0.25
Nodes (5): SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 157 - "Command"
Cohesion: 0.25
Nodes (6): Command, SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 158 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 159 - "MixerCommands.h"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 160 - "ResizeNotesCommand"
Cohesion: 0.25
Nodes (7): ResizeNotesCommand, channel_, durationDelta_, indices_, pattern_, previousDurations_, undo

### Community 161 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 162 - "TimestampedMidiMessage"
Cohesion: 0.25
Nodes (8): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status

### Community 163 - "ProjectMetadata"
Cohesion: 0.25
Nodes (8): ProjectMetadata, artist, comment, created, createdWith, lastSavedWith, modified, title

### Community 164 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 165 - "ClapLibrary"
Cohesion: 0.29
Nodes (6): clap_plugin_entry_t, ClapLibrary, entry_, factory_, library_, clap_plugin_factory_t

### Community 166 - "AudioEditorView.mm"
Cohesion: 0.29
Nodes (6): -acceptsFirstResponder, -hasSelection, -initWithFrameprojectregistry, -isFlipped, -selectionFrom, -selectionTo

### Community 167 - "MixerFixture"
Cohesion: 0.29
Nodes (6): EntityId, TempoMap, MixerFixture, pattern, project, tempo

### Community 168 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 169 - "emptyInGet"
Cohesion: 0.33
Nodes (6): clap_event_header_t, clap_input_events_t, clap_output_events_t, emptyInGet(), emptyInSize(), emptyOutTryPush()

### Community 170 - "INCDAWMixerView"
Cohesion: 0.33
Nodes (5): incdaw, NSView, INCDAWMixerView, -initWithFrameprojectregistry, stripLookup

### Community 171 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 172 - "InstrumentTests.cpp"
Cohesion: 0.40
Nodes (5): SimpleSynth, AudioBufferPool, MidiBuffer, Sample, renderSynth()

### Community 173 - "SetChannelStepKeyCommand"
Cohesion: 0.29
Nodes (6): Command, SetChannelStepKeyCommand, channelId_, key_, previousKey_, undo

### Community 174 - "process"
Cohesion: 0.40
Nodes (5): FrameCount, SampleRate, prepare, process, triggerClick

### Community 175 - "collectForRange"
Cohesion: 0.33
Nodes (4): FrameCount, FramePosition, MidiBuffer, collectForRange

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

### Community 180 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 181 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 182 - "AutomationTests.cpp"
Cohesion: 0.60
Nodes (4): AutomationPoint, Tick, enginePoint(), modelPoint()

### Community 183 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 186 - "framesToSeconds"
Cohesion: 0.67
Nodes (4): framesToSeconds(), FrameCount, SampleRate, secondsToFrames()

### Community 187 - "INCDAWChannelRackView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWChannelRackView, -initWithFrameprojectregistry

### Community 188 - "INCDAWPatternListView"
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
- **867 isolated node(s):** `index_`, `minted_`, `pattern_`, `index_`, `minted_` (+862 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **38 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `AddNoteCommand`, `PatternCommands.cpp`, `RemoveTrackCommand`, `AudioAsset`, `AudioBufferPool`, `DisconnectMixerCommand`, `DeleteNotesCommand`, `CommandRegistry`, `MixerNode`, `RoutingConnection`, `SetChannelOutputCommand`, `SetMixerPanCommand`, `SetMixerVolumeCommand`, `Clip`, `AutomationFixture`, `RenameChannelCommand`, `SetChannelSoloedCommand`, `Command`, `SetMixerSoloedCommand`, `MixerCommands.h`, `ResizeNotesCommand`, `ProjectMetadata`, `MixerFixture`, `ProjectFile.cpp`, `SetChannelStepKeyCommand`, `NoteCommands.cpp`, `compileArrangement`, `CountingCommand`, `MixerCommands.cpp`, `Pattern`, `SetVelocityCommand`, `ChannelCommands.cpp`, `string`, `EntityId`, `string`, `EntityId`, `QuantizeNotesCommand`, `Track`, `Fixture`, `Channel`, `RemoveMixerNodeCommand`, `AddMixerNodeCommand`?**
  _High betweenness centrality (0.176) - this node is a cross-community bridge._