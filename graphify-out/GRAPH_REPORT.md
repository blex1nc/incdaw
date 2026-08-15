# Graph Report - incdaw-phase-13-parameters-55b5b1  (2026-08-15)

## Corpus Check
- 235 files · ~179,734 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 3831 nodes · 6741 edges · 190 communities (188 shown, 2 thin omitted)
- Extraction: 95% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 298 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `debb64d8`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- PatternCommands.cpp
- RenameTrackCommand
- Transport
- WavStreamWriter
- PluginRegistry
- AudioBufferPool
- CoreAudioDevice
- PlaylistModel
- SimpleSynth
- AddInsertCommand
- Json
- CompiledProjectGraph
- CommandRegistry
- AudioEngine
- TestLatencyPlugin.cpp
- EditAssetRegionCommand
- ChannelRackModel
- AudioLogger
- RecordingSession
- TempoMap
- InstrumentNode
- Project
- WavStreamReader
- Sampler
- MixerStripNode
- AudioDeviceConfig
- CoreMidiDevice
- MixerView.mm
- AudioStream
- InsertRecordedTakeCommand
- CoreAudioDevice.cpp
- NoteSequence
- Clip
- atomic
- MidiMessage
- PlaylistView.mm
- AudioRecorder
- MidiInput
- ResizeNotesCommand
- TestGainPlugin.cpp
- GraphCompileOptions
- load
- CallbackProfiler
- MusicalPosition
- MetronomeNode
- AudioDevice
- WaveformOverview
- WriteAutomationCommand
- Sampler.cpp
- ResizeClipsCommand
- QuantizeNotesCommand
- GraphBuilder
- LevelMeter
- DelayLineNode
- CompiledGraph
- PluginPersistenceTests.cpp
- RealtimeGuard.cpp
- AutomationWriteSession
- AudioFileData
- BasicMidiBuffer
- SystemInfo
- EditFixture
- write
- SamplerZone
- AudioClipNode
- Command
- AudioBufferView
- SetVelocityCommand
- NoteCommands.cpp
- PluginIdentifier
- Node
- MidiRecorder
- INCDAW — Decision Log
- SamplerTests.cpp
- 2. INCDAW functional scope
- PianoRollModel
- LockFreeQueue
- SampleRingBuffer
- [Unreleased]
- LoopbackResult
- MixerTests.cpp
- ioProcTrampoline
- INCDAWAppDelegate
- INCDAW — Roadmap
- Json.cpp
- ParsedHeader
- GainNode
- SineOscillatorNode
- PluginInsertTests.cpp
- 4. Specialised tests
- TimingProbeInstrument
- PluginInstanceManager
- INCDAW — Plugin Host
- ChannelCommands.cpp
- INCDAW — Architecture
- ProcessContext
- DuplicateClipsCommand
- MoveClipsCommand
- TrackCommands.cpp
- Parser
- Transport.cpp
- INCDAW — Audio Engine
- renderNode
- ConnectMixerCommand
- CoreMidiDevice.cpp
- Options
- ParameterRegistry
- ChannelRackView.mm
- MoveNotesCommand
- vector
- AutomationPoint
- AutomationNode
- AddPatternClipCommand
- MidiEvent
- InputMonitorNode
- ClapLibrary.cpp
- humanizeNoteStarts
- main.mm
- renderClickFrames
- process
- ToggleStepCommand
- INCDAW — Performance Strategy
- INCDAW — Project Format
- thread_
- AudioEngine.h
- MidiTests.cpp
- build
- AutomationProbe
- PatternTests.cpp
- PluginFolder
- InsertFixture
- TimeSignatureEvent
- MidiDevice
- AutomationTests.cpp
- PatternListView.mm
- allocate
- AddTrackCommand
- readAt
- SharedLibrary
- ClapDescriptor
- ScratchDir
- ChildResult
- RemoveClipsCommand
- make-dmg.sh
- BlockSegment
- ScriptedFactory
- BlobReader
- RemoveTrackCommand
- ClapInstance
- ConstantNode
- SetTrackMutedCommand
- SetTrackSoloedCommand
- AddNoteCommand
- RecordedEvent
- PluginParameterInfo
- capturePluginState
- ScratchDir
- Version
- TimestampedMidiMessage
- makeTestSignal
- ClapLibrary
- AudioEditorView.mm
- MixerNode
- ScratchDirectory
- ParameterFixture
- INCDAWMixerView
- INCDAWPianoRollView
- RecordingSink
- ParameterSink
- Pattern
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- renderBlock
- PluginNode
- INCDAWAudioEditorView
- INCDAWPlaylistView
- check
- AudioCaptureSink
- MidiDeviceInfo
- -applicationDidFinishLaunching
- PluginStateTests.cpp
- AutomationFixture
- StateIO

## God Nodes (most connected - your core abstractions)
1. `Project` - 200 edges
2. `EntityId` - 175 edges
3. `Command` - 104 edges
4. `AudioEngine` - 68 edges
5. `TempoMap` - 59 edges
6. `CoreAudioDevice` - 59 edges
7. `CommandRegistry` - 49 edges
8. `Json` - 48 edges
9. `Node` - 44 edges
10. `AudioFileData` - 41 edges

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

## Communities (190 total, 2 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "PatternCommands.cpp"
Cohesion: 0.04
Nodes (43): AddPatternCommand, execute, index_, minted_, pattern_, undo, DuplicatePatternCommand, execute (+35 more)

### Community 2 - "RenameTrackCommand"
Cohesion: 0.16
Nodes (6): string, RenameTrackCommand, execute, previousName_, trackId_, undo

### Community 3 - "Transport"
Cohesion: 0.12
Nodes (12): atomic, FramePosition, size_t, Tick, uint32_t, Transport, loopEnabled_, maxSegmentsPerBlock (+4 more)

### Community 4 - "WavStreamWriter"
Cohesion: 0.05
Nodes (59): ofstream, appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample (+51 more)

### Community 5 - "PluginRegistry"
Cohesion: 0.16
Nodes (21): Library, Located, int64_t, path, size_t, string, uint64_t, vector (+13 more)

### Community 6 - "AudioBufferPool"
Cohesion: 0.17
Nodes (9): AudioBufferPool, channelPointers_, reset, samples_, FrameCount, Sample, size_t, unique_ptr (+1 more)

### Community 7 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 8 - "PlaylistModel"
Cohesion: 0.07
Nodes (42): Rect, size_t, Tick, vector, size_t, Tick, vector, Viewport (+34 more)

### Community 9 - "SimpleSynth"
Cohesion: 0.06
Nodes (36): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+28 more)

### Community 10 - "AddInsertCommand"
Cohesion: 0.08
Nodes (24): AddInsertCommand, execute, minted_, mixerNode_, plugin_, slot_, undo, findNode() (+16 more)

### Community 11 - "Json"
Cohesion: 0.10
Nodes (14): nullptr_t, int64_t, pair, string, vector, Json, asDouble, boolean_ (+6 more)

### Community 12 - "CompiledProjectGraph"
Cohesion: 0.10
Nodes (24): CompiledProjectGraph, automation, channels, channelStripFor, channelStrips, error, graph, insertSlots (+16 more)

### Community 13 - "CommandRegistry"
Cohesion: 0.05
Nodes (47): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+39 more)

### Community 14 - "AudioEngine"
Cohesion: 0.07
Nodes (49): RetiredGraph, AudioEngine, active_, anchor_, anchorVersion_, audioDeviceAboutToStart, audioDeviceStopped, availableDevices (+41 more)

### Community 15 - "TestLatencyPlugin.cpp"
Cohesion: 0.11
Nodes (25): clap_host_t, clap_plugin_descriptor_t, clap_plugin_factory_t, clap_plugin_t, clap_process_status, clap_process_t, vector, factoryCreatePlugin() (+17 more)

### Community 16 - "EditAssetRegionCommand"
Cohesion: 0.08
Nodes (21): AudioEditOp, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_, minted_ (+13 more)

### Community 17 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 18 - "AudioLogger"
Cohesion: 0.09
Nodes (22): AudioLogger, capacityFrames_, circle_, enabled_, grab, log, prepare, ready_ (+14 more)

### Community 19 - "RecordingSession"
Cohesion: 0.09
Nodes (20): path, Placement, string, vector, FrameCount, FramePosition, uint32_t, uint64_t (+12 more)

### Community 20 - "TempoMap"
Cohesion: 0.12
Nodes (27): execute, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+19 more)

### Community 21 - "InstrumentNode"
Cohesion: 0.08
Nodes (24): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+16 more)

### Community 22 - "Project"
Cohesion: 0.05
Nodes (52): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, size_t, vector (+44 more)

### Community 23 - "WavStreamReader"
Cohesion: 0.11
Nodes (16): ifstream, FrameCount, path, SampleRate, size_t, uint16_t, uint64_t, uint8_t (+8 more)

### Community 24 - "Sampler"
Cohesion: 0.08
Nodes (18): array, atomic, maxVoices, SampleRate, size_t, uint64_t, vector, Voice (+10 more)

### Community 25 - "MixerStripNode"
Cohesion: 0.13
Nodes (19): FrameCount, Sample, SampleRate, atomic, Sample, MixerStripNode, left_, meter_ (+11 more)

### Community 26 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 27 - "CoreMidiDevice"
Cohesion: 0.16
Nodes (13): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+5 more)

### Community 28 - "MixerView.mm"
Cohesion: 0.25
Nodes (17): -acceptsFirstResponder, -addStripRect, -drawRect, -drawStripnode, -faderRectAt, -initWithFrameprojectregistry, -isFlipped, -meterRectAt (+9 more)

### Community 29 - "AudioStream"
Cohesion: 0.06
Nodes (44): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+36 more)

### Community 30 - "InsertRecordedTakeCommand"
Cohesion: 0.10
Nodes (16): Placement, size_t, string, vector, InsertRecordedTakeCommand, asset_, assetIndex_, clipIndices_ (+8 more)

### Community 31 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 32 - "NoteSequence"
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, MidiBuffer, Tick, vector, size_t, Tick, uint32_t (+17 more)

### Community 33 - "Clip"
Cohesion: 0.07
Nodes (28): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+20 more)

### Community 34 - "atomic"
Cohesion: 0.17
Nodes (3): atomic, MidiBuffer, array

### Community 35 - "MidiMessage"
Cohesion: 0.09
Nodes (14): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+6 more)

### Community 36 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (24): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+16 more)

### Community 37 - "AudioRecorder"
Cohesion: 0.07
Nodes (28): AudioCaptureSink, AudioRecorder, captureAudioBlock, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_ (+20 more)

### Community 38 - "MidiInput"
Cohesion: 0.11
Nodes (18): FrameCount, MidiBuffer, SampleRate, uint64_t, atomic, queueCapacity, size_t, uint64_t (+10 more)

### Community 39 - "ResizeNotesCommand"
Cohesion: 0.14
Nodes (11): NoteIndices, Tick, ResizeNotesCommand, canMergeWith, channel_, durationDelta_, indices_, mergeWith (+3 more)

### Community 40 - "TestGainPlugin.cpp"
Cohesion: 0.06
Nodes (57): clap_gui_resize_hints_t, clap_id, clap_param_info_t, clap_window_t, applyParamEvents(), clap_host_t, clap_input_events_t, clap_istream_t (+49 more)

### Community 41 - "GraphCompileOptions"
Cohesion: 0.09
Nodes (23): PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, insertFactory, instrumentFactory, masterGain, maxBlockSize (+15 more)

### Community 42 - "load"
Cohesion: 0.20
Nodes (21): append, automationPointFrom(), bindUnassignedContent(), AutomationPoint, path, Result, string, idFrom() (+13 more)

### Community 43 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 44 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 45 - "MetronomeNode"
Cohesion: 0.09
Nodes (17): atomic, FrameCount, Sample, SampleRate, size_t, vector, MetronomeNode, amplitude_ (+9 more)

### Community 46 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 47 - "WaveformOverview"
Cohesion: 0.17
Nodes (11): Bucket, FrameCount, SampleRate, size_t, vector, WaveformOverview, channelCount, channels (+3 more)

### Community 48 - "WriteAutomationCommand"
Cohesion: 0.05
Nodes (47): AddAutomationLaneCommand, execute, index_, key_, lane_, minted_, target_, undo (+39 more)

### Community 49 - "Sampler.cpp"
Cohesion: 0.15
Nodes (19): FrameCount, Sample, SampleRate, vector, Voice, interpolate(), activeVoiceCount, allNotesOff (+11 more)

### Community 50 - "ResizeClipsCommand"
Cohesion: 0.15
Nodes (10): ClipIds, FrameCount, Tick, ResizeClipsCommand, canMergeWith, clips_, lengthDelta_, mergeWith (+2 more)

### Community 51 - "QuantizeNotesCommand"
Cohesion: 0.12
Nodes (15): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo (+7 more)

### Community 52 - "GraphBuilder"
Cohesion: 0.10
Nodes (25): Connection, NodeIndex, SampleRate, size_t, unique_ptr, GraphBuilder, addNode, analyse (+17 more)

### Community 53 - "LevelMeter"
Cohesion: 0.08
Nodes (21): atomic, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond, rmsWindowSeconds (+13 more)

### Community 54 - "DelayLineNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_, prepare (+6 more)

### Community 55 - "CompiledGraph"
Cohesion: 0.13
Nodes (13): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+5 more)

### Community 56 - "PluginPersistenceTests.cpp"
Cohesion: 0.14
Nodes (11): path, string, uint8_t, vector, gainBlob(), Harness, folder, registry (+3 more)

### Community 57 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 58 - "AutomationWriteSession"
Cohesion: 0.15
Nodes (12): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, string, Tick (+4 more)

### Community 59 - "AudioFileData"
Cohesion: 0.16
Nodes (25): applyGain(), applyRamp(), clampedRegion(), Sample, fadeIn(), fadeOut(), FrameCount, normalize() (+17 more)

### Community 60 - "BasicMidiBuffer"
Cohesion: 0.13
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 61 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 62 - "EditFixture"
Cohesion: 0.15
Nodes (13): FrameCount, path, Sample, size_t, EditFixture, assetId, file, project (+5 more)

### Community 63 - "write"
Cohesion: 0.29
Nodes (14): assetFilePath(), Sample, string, vector, execute, name, undo, findAsset() (+6 more)

### Community 64 - "SamplerZone"
Cohesion: 0.11
Nodes (17): FrameCount, shared_ptr, handleMessage, SamplerZone, end, gain, keyHigh, keyLow (+9 more)

### Community 65 - "AudioClipNode"
Cohesion: 0.13
Nodes (13): AudioClipNode, addClip, clips_, fetchScratch_, prepare, process, FrameCount, PlacedClip (+5 more)

### Community 66 - "Command"
Cohesion: 0.03
Nodes (74): RemovedRouting, Command, execute, id, name, undo, AddMixerNodeCommand, execute (+66 more)

### Community 67 - "AudioBufferView"
Cohesion: 0.16
Nodes (11): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t, process (+3 more)

### Community 68 - "SetVelocityCommand"
Cohesion: 0.13
Nodes (9): string, SetVelocityCommand, canMergeWith, channel_, indices_, mergeWith, pattern_, previousVelocities_ (+1 more)

### Community 69 - "NoteCommands.cpp"
Cohesion: 0.27
Nodes (13): NoteIndices, size_t, vector, execute, findEvents(), execute, undo, execute (+5 more)

### Community 70 - "PluginIdentifier"
Cohesion: 0.16
Nodes (11): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+3 more)

### Community 71 - "Node"
Cohesion: 0.18
Nodes (6): FrameCount, SampleRate, Node, process, ParameterSink, StateIO

### Community 72 - "MidiRecorder"
Cohesion: 0.20
Nodes (10): CapturedMessage, atomic, queueCapacity, size_t, uint64_t, MidiRecorder, captured_, dropped_ (+2 more)

### Community 73 - "INCDAW — Decision Log"
Cohesion: 0.06
Nodes (32): D-001 — Core implementation language: C++20, D-002 — Build system: CMake + Ninja, D-003 — Audio I/O: CoreAudio HAL directly, no wrapper framework, D-004 — Realtime thread scheduling: os_workgroup / Audio Workgroups, D-005 — Platform strategy: macOS first, Windows later, Linux not precluded, D-006 — UI: AppKit shell + INCDAW-owned Metal-rendered widget layer, D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded), D-008 — Licensing: INCDAW is closed-source (+24 more)

### Community 74 - "SamplerTests.cpp"
Cohesion: 0.17
Nodes (15): constantSample(), FrameCount, MidiBuffer, Sample, shared_ptr, vector, makeEnvelopeTransparent(), rampSample() (+7 more)

### Community 75 - "2. INCDAW functional scope"
Cohesion: 0.07
Nodes (29): 1.1 Findings that changed INCDAW's architecture, 1.2 Supported plugin formats (official), 1.3 Other FL Studio 2026 features, recorded for completeness, 1. Functional reference: FL Studio 2026, 2. INCDAW functional scope, 3. Non-functional requirements, Audio editor, Audio engine (+21 more)

### Community 76 - "PianoRollModel"
Cohesion: 0.07
Nodes (39): NoteList, size_t, Tick, vector, size_t, Tick, vector, Viewport (+31 more)

### Community 77 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 78 - "SampleRingBuffer"
Cohesion: 0.19
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 79 - "[Unreleased]"
Cohesion: 0.06
Nodes (35): INCDAW — Changelog, Phase 0 — Research and architecture — 2026-08-14, Phase 10 — Mixer, routing and delay compensation — 2026-08-14, Phase 11a — Automation: the generic subsystem — 2026-08-15, Phase 11b — Automation placement and recording — 2026-08-15, Phase 12 (part 1) — WAV codec — 2026-08-15, Phase 12 (part 2) — Input capture and recording — 2026-08-15, Phase 12 (part 3) — Recording lands in the timeline — 2026-08-15 (+27 more)

### Community 80 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 81 - "MixerTests.cpp"
Cohesion: 0.09
Nodes (21): FrameCount, FramePosition, Sample, SampleRate, size_t, vector, ImpulseNode, latency_ (+13 more)

### Community 82 - "ioProcTrampoline"
Cohesion: 0.23
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 83 - "INCDAWAppDelegate"
Cohesion: 0.20
Nodes (9): NSApplicationDelegate, NSObject, NSSegmentedControl, NSTextField, NSWindow, NSView, INCDAWChannelRackView, -initWithFrameprojectregistry (+1 more)

### Community 84 - "INCDAW — Roadmap"
Cohesion: 0.08
Nodes (23): Deliberately out of scope, INCDAW — Roadmap, Phase 0 — Research and architecture ✅ COMPLETE, Phase 10 — Mixer and routing, Phase 11 — Automation, Phase 12 — Recording and audio editor, Phase 13 — Plugin hosting, Phase 14 — Sampler (+15 more)

### Community 85 - "Json.cpp"
Cohesion: 0.22
Nodes (13): int64_t, size_t, string, escapeInto(), formatDouble(), asBool, asInt, asString (+5 more)

### Community 86 - "ParsedHeader"
Cohesion: 0.16
Nodes (20): path, Result, size_t, uint16_t, uint32_t, uint8_t, vector, fillMetadata() (+12 more)

### Community 87 - "GainNode"
Cohesion: 0.15
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 88 - "SineOscillatorNode"
Cohesion: 0.13
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 89 - "PluginInsertTests.cpp"
Cohesion: 0.18
Nodes (13): anyNonZero(), ClipInsert, threshold_, FrameCount, Sample, vector, GainInsert, factor_ (+5 more)

### Community 90 - "4. Specialised tests"
Cohesion: 0.11
Nodes (18): 1. Principle, 2. Framework, 3. Test levels, 4. Specialised tests, 5. What is not tested automatically, End-to-end, Fuzzing (from Phase 4), Golden-file audio (from Phase 7) (+10 more)

### Community 91 - "TimingProbeInstrument"
Cohesion: 0.14
Nodes (10): Applied, FrameCount, MidiBuffer, Sample, SampleRate, vector, renderSynth(), TimingProbeInstrument (+2 more)

### Community 92 - "PluginInstanceManager"
Cohesion: 0.11
Nodes (26): Held, size_t, string, uint32_t, uint64_t, unique_ptr, vector, mutex (+18 more)

### Community 93 - "INCDAW — Plugin Host"
Cohesion: 0.12
Nodes (16): 10. Testing, 1. Supported formats, 2. Prime directive, 3. Pipeline, 4. Isolation strategy, 5. Parameter system, 6. State, 7. Editor / UI bridge (+8 more)

### Community 94 - "ChannelCommands.cpp"
Cohesion: 0.04
Nodes (46): RemovedContent, AddChannelCommand, channel_, execute, index_, minted_, undo, size_t (+38 more)

### Community 95 - "INCDAW — Architecture"
Cohesion: 0.12
Nodes (15): 1. Guiding principle, 2. Layer model, 3. Proposed repository structure, 4. Threading model, 5. Data model, 6. Command architecture, 7. Engine boundary, 8. Plugin isolation (+7 more)

### Community 96 - "ProcessContext"
Cohesion: 0.10
Nodes (16): FramePosition, MidiBuffer, size_t, ProcessContext, frameCount, inputCount, inputs, liveMidi (+8 more)

### Community 97 - "DuplicateClipsCommand"
Cohesion: 0.13
Nodes (9): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_, undo (+1 more)

### Community 98 - "MoveClipsCommand"
Cohesion: 0.15
Nodes (16): MovedAudioClip, execute, execute, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, canMergeWith, clips_ (+8 more)

### Community 99 - "TrackCommands.cpp"
Cohesion: 0.18
Nodes (11): execute, execute, undo, SetTrackHeightCommand, canMergeWith, execute, height_, mergeWith (+3 more)

### Community 100 - "Parser"
Cohesion: 0.30
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 101 - "Transport.cpp"
Cohesion: 0.18
Nodes (13): FrameCount, FramePosition, size_t, applyPendingSeek, pause, play, processBlock, seek (+5 more)

### Community 102 - "INCDAW — Audio Engine"
Cohesion: 0.15
Nodes (12): 10. Audio correctness requirements, 11. Performance budget, 1. The prime directive, 2. Device layer, 3. Realtime thread scheduling, 4. Realtime safety enforcement, 5. Signal flow, 6. Block processing and sample-accurate events (+4 more)

### Community 103 - "renderNode"
Cohesion: 0.21
Nodes (11): FrameCount, path, Sample, shared_ptr, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 104 - "ConnectMixerCommand"
Cohesion: 0.06
Nodes (27): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+19 more)

### Community 105 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 106 - "Options"
Cohesion: 0.12
Nodes (17): int64_t, string, Options, amplitude, buffer, device, frequency, input (+9 more)

### Community 107 - "ParameterRegistry"
Cohesion: 0.20
Nodes (15): Applier, Entry, string, uint32_t, vector, Entry, size_t, vector (+7 more)

### Community 108 - "ChannelRackView.mm"
Cohesion: 0.23
Nodes (13): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+5 more)

### Community 109 - "MoveNotesCommand"
Cohesion: 0.18
Nodes (10): MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, canMergeWith, channel_, indices_, keyDelta_, mergeWith (+2 more)

### Community 110 - "vector"
Cohesion: 0.19
Nodes (3): vector, string, ParameterSink

### Community 111 - "AutomationPoint"
Cohesion: 0.17
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 112 - "AutomationNode"
Cohesion: 0.15
Nodes (7): Binding, AutomationNode, bindings_, tempoMap_, size_t, vector, StateIO

### Community 113 - "AddPatternClipCommand"
Cohesion: 0.17
Nodes (10): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+2 more)

### Community 114 - "MidiEvent"
Cohesion: 0.15
Nodes (13): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+5 more)

### Community 115 - "InputMonitorNode"
Cohesion: 0.15
Nodes (9): FrameCount, Sample, SampleRate, size_t, vector, InputMonitorNode, channelCount_, ring_ (+1 more)

### Community 116 - "ClapLibrary.cpp"
Cohesion: 0.08
Nodes (33): clap_event_header_t, clap_event_param_value_t, blobRead(), size, blobWrite(), hasEditor, openEditor, process (+25 more)

### Community 117 - "humanizeNoteStarts"
Cohesion: 0.29
Nodes (10): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+2 more)

### Community 118 - "main.mm"
Cohesion: 0.29
Nodes (10): -editorChanged, -openAudioAssetInEditor, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll, -showPlaylist, -togglePlayback (+2 more)

### Community 119 - "renderClickFrames"
Cohesion: 0.33
Nodes (6): FrameCount, FramePosition, size_t, vector, plan(), renderClickFrames()

### Community 120 - "process"
Cohesion: 0.40
Nodes (5): FrameCount, SampleRate, prepare, process, triggerClick

### Community 121 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (8): size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_, step_

### Community 122 - "INCDAW — Performance Strategy"
Cohesion: 0.18
Nodes (10): 1. Reference machine, 2. Targets, 3. Instrumentation, 4. Method, 5. Known design-level performance decisions, 6. Profiling tooling, Audio, INCDAW — Performance Strategy (+2 more)

### Community 123 - "INCDAW — Project Format"
Cohesion: 0.18
Nodes (10): 1. Shape: a package directory, not a single file, 2. Versioning and migration, 3. Text vs binary, 4. Media: referenced or embedded, 5. Autosave, backup and recovery, 6. Archiving, 7. Determinism, 8. Tests (Phase 4 gate) (+2 more)

### Community 124 - "thread_"
Cohesion: 0.23
Nodes (8): ControlRegister, thread_, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 125 - "AudioEngine.h"
Cohesion: 0.15
Nodes (8): AudioCaptureSink, FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 126 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped()

### Community 127 - "build"
Cohesion: 0.27
Nodes (9): bucketize(), Bucket, FrameCount, path, Result, Sample, vector, sizeBuckets() (+1 more)

### Community 128 - "AutomationProbe"
Cohesion: 0.29
Nodes (6): AutomationProbe, calls, registry, written, FramePosition, vector

### Community 129 - "PatternTests.cpp"
Cohesion: 0.53
Nodes (5): Tick, vector, note(), shapeOf(), startsOf()

### Community 130 - "PluginFolder"
Cohesion: 0.24
Nodes (7): path, PluginFolder, crash, dir, gain, ScratchDir, path

### Community 131 - "InsertFixture"
Cohesion: 0.33
Nodes (4): InsertFixture, pattern, project, tempo

### Community 132 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 133 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 134 - "AutomationTests.cpp"
Cohesion: 0.60
Nodes (4): AutomationPoint, Tick, enginePoint(), modelPoint()

### Community 135 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 136 - "allocate"
Cohesion: 0.19
Nodes (13): allocate, FrameCount, size_t, FramePosition, Sample, vector, render(), anyNonZero() (+5 more)

### Community 137 - "AddTrackCommand"
Cohesion: 0.25
Nodes (6): AddTrackCommand, index_, minted_, track_, undo, size_t

### Community 138 - "readAt"
Cohesion: 0.25
Nodes (8): FrameCount, path, Result, Sample, size_t, close, open, readAt

### Community 139 - "SharedLibrary"
Cohesion: 0.25
Nodes (7): path, string, SharedLibrary, close, handle_, open, symbol

### Community 140 - "ClapDescriptor"
Cohesion: 0.13
Nodes (17): ClapDescriptor, id, name, vendor, version, string, path, string (+9 more)

### Community 141 - "ScratchDir"
Cohesion: 0.50
Nodes (3): path, ScratchDir, path

### Community 142 - "ChildResult"
Cohesion: 0.18
Nodes (10): End, ChildResult, code, end, output, path, string, vector (+2 more)

### Community 143 - "RemoveClipsCommand"
Cohesion: 0.12
Nodes (15): string, RemovedClip, vector, RemoveClipsCommand, clips_, execute, name, removed_ (+7 more)

### Community 144 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 145 - "BlockSegment"
Cohesion: 0.29
Nodes (6): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount

### Community 146 - "ScriptedFactory"
Cohesion: 0.25
Nodes (7): function, InsertFactory, unique_ptr, ScriptedFactory, fail, makers, requests

### Community 147 - "BlobReader"
Cohesion: 0.22
Nodes (11): BlobReader, cursor, data, BlobWriter, out, overflowed, loadState, saveState (+3 more)

### Community 149 - "RemoveTrackCommand"
Cohesion: 0.22
Nodes (7): RemovedClip, vector, RemoveTrackCommand, clips_, index_, track_, trackId_

### Community 150 - "ClapInstance"
Cohesion: 0.09
Nodes (22): clap_plugin_gui_t, clap_plugin_state_t, ParamEvent, ClapInstance, closeEditor, editorOpen_, gui_, host_ (+14 more)

### Community 151 - "ConstantNode"
Cohesion: 0.10
Nodes (14): ConstantNode, latency_, value_, FrameCount, Sample, size_t, vector, OrderRecordingNode (+6 more)

### Community 152 - "SetTrackMutedCommand"
Cohesion: 0.25
Nodes (5): SetTrackMutedCommand, execute, muted_, trackId_, undo

### Community 153 - "SetTrackSoloedCommand"
Cohesion: 0.25
Nodes (5): SetTrackSoloedCommand, execute, soloed_, trackId_, undo

### Community 155 - "AddNoteCommand"
Cohesion: 0.20
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, size_t

### Community 156 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 157 - "PluginParameterInfo"
Cohesion: 0.20
Nodes (9): string, uint32_t, PluginParameterInfo, defaultValue, id, maxValue, minValue, name (+1 more)

### Community 159 - "capturePluginState"
Cohesion: 0.51
Nodes (9): capturePluginState(), path, string, uint8_t, vector, readBlobFile(), restorePluginState(), stateFileNameFor() (+1 more)

### Community 160 - "ScratchDir"
Cohesion: 0.50
Nodes (3): path, ScratchDir, path

### Community 161 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 162 - "TimestampedMidiMessage"
Cohesion: 0.22
Nodes (9): midiMessageReceived, sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos (+1 more)

### Community 164 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 165 - "ClapLibrary"
Cohesion: 0.13
Nodes (11): clap_plugin_entry_t, ClapLibrary, descriptors, entry_, factory_, library_, clap_plugin_factory_t, path (+3 more)

### Community 166 - "AudioEditorView.mm"
Cohesion: 0.29
Nodes (6): -acceptsFirstResponder, -hasSelection, -initWithFrameprojectregistry, -isFlipped, -selectionFrom, -selectionTo

### Community 167 - "MixerNode"
Cohesion: 0.03
Nodes (63): AutomationCurve, AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id (+55 more)

### Community 168 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 169 - "ParameterFixture"
Cohesion: 0.33
Nodes (4): ParameterFixture, pattern, project, tempo

### Community 170 - "INCDAWMixerView"
Cohesion: 0.25
Nodes (7): incdaw, NSArray, NSDictionary, NSView, INCDAWMixerView, -initWithFrameprojectregistry, stripLookup

### Community 171 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 172 - "RecordingSink"
Cohesion: 0.40
Nodes (5): pair, ParameterSink, uint32_t, RecordingSink, received

### Community 175 - "Pattern"
Cohesion: 0.08
Nodes (39): Emit, size_t, Tick, vector, noteAtStep(), execute, undo, Tick (+31 more)

### Community 176 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 177 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 178 - "renderBlock"
Cohesion: 0.47
Nodes (5): FrameCount, Sample, vector, renderBlock(), tone()

### Community 179 - "PluginNode"
Cohesion: 0.25
Nodes (3): FrameCount, PluginNode, instance_

### Community 180 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 181 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 183 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 186 - "MidiDeviceInfo"
Cohesion: 0.22
Nodes (7): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback, midiMessageReceived

### Community 187 - "-applicationDidFinishLaunching"
Cohesion: 0.20
Nodes (9): NSScrollView, NSSplitView, -applicationDidFinishLaunching, NSView, -selectChannel, -selectPattern, NSView, INCDAWPatternListView (+1 more)

### Community 188 - "PluginStateTests.cpp"
Cohesion: 0.12
Nodes (17): anyNonZero(), compileLoaded(), InsertFactory, path, Sample, uint8_t, vector, factoryFor() (+9 more)

### Community 195 - "AutomationFixture"
Cohesion: 0.29
Nodes (5): AutomationFixture, channel, pattern, project, tempo

### Community 200 - "StateIO"
Cohesion: 0.40
Nodes (3): StateIO, loadState, saveState

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
- **1146 isolated node(s):** `streams_`, `enabled_`, `noRow`, `layout_`, `id` (+1141 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **2 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `AutomationProbe`, `PatternCommands.cpp`, `RenameTrackCommand`, `InsertFixture`, `PlaylistModel`, `AddTrackCommand`, `AddInsertCommand`, `CompiledProjectGraph`, `CommandRegistry`, `RemoveClipsCommand`, `TempoMap`, `SetTrackMutedCommand`, `SetTrackSoloedCommand`, `AddNoteCommand`, `AudioStream`, `InsertRecordedTakeCommand`, `capturePluginState`, `Clip`, `ResizeNotesCommand`, `MixerNode`, `ParameterFixture`, `load`, `Pattern`, `WriteAutomationCommand`, `QuantizeNotesCommand`, `PluginStateTests.cpp`, `EditFixture`, `write`, `Command`, `AutomationFixture`, `NoteCommands.cpp`, `MixerTests.cpp`, `ChannelCommands.cpp`, `DuplicateClipsCommand`, `MoveClipsCommand`, `TrackCommands.cpp`, `ConnectMixerCommand`, `AddPatternClipCommand`?**
  _High betweenness centrality (0.123) - this node is a cross-community bridge._