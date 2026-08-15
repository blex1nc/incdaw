# Graph Report - incdaw-phase-13-parameters-55b5b1  (2026-08-15)

## Corpus Check
- 235 files · ~178,923 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 3827 nodes · 6737 edges · 203 communities (199 shown, 4 thin omitted)
- Extraction: 95% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 298 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `5471465a`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- string
- RenameTrackCommand
- Transport
- WavStreamWriter
- PluginRegistry
- AudioBufferPool
- CoreAudioDevice
- PlaylistModel
- SimpleSynth
- RemoveInsertCommand
- CommandRegistry
- CompiledProjectGraph
- CommandRegistry.cpp
- AudioEngine
- TestLatencyPlugin.cpp
- EditAssetRegionCommand
- ChannelRackModel
- AudioLogger
- RecordingSession
- TempoMap
- InstrumentNode
- EntityId
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
- CountingCommand
- TestGainPlugin.cpp
- GraphCompileOptions
- Json
- CallbackProfiler
- MusicalPosition
- MetronomeNode
- AudioDevice
- WaveformOverview
- WriteAutomationCommand
- Sampler.cpp
- ResizeClipsCommand
- MoveNotesCommand
- GraphBuilder
- Smoother
- DelayLineNode
- CompiledGraph
- PluginPersistenceTests.cpp
- RealtimeGuard.cpp
- AutomationWriteSession
- AudioFileData
- BasicMidiBuffer
- SystemInfo
- EditFixture
- AudioEditCommands.cpp
- SamplerZone
- AudioClipNode
- Command
- AudioBufferView
- LevelMeter
- Project
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
- OrderRecordingNode
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
- PatternCommands.cpp
- SetPatternLengthCommand
- INCDAW — Audio Engine
- renderNode
- ConnectMixerCommand
- CoreMidiDevice.cpp
- Options
- ParameterRegistry
- ChannelRackView.mm
- renderArrangement
- vector
- AutomationPoint
- AutomationNode
- AddPatternClipCommand
- MidiEvent
- InputMonitorNode
- ClapLibrary.cpp
- humanizeNoteStarts
- main.mm
- ClipCommands.cpp
- AddPatternCommand
- ToggleStepCommand
- INCDAW — Performance Strategy
- INCDAW — Project Format
- RealtimeSafetyTests.cpp
- TimelineAnchor
- MidiTests.cpp
- build
- AutomationProbe
- DuplicatePatternCommand
- PluginFolder
- SimpleSynth.cpp
- TimeSignatureEvent
- MidiDevice
- RemovePatternCommand
- PatternListView.mm
- allocate
- AddTrackCommand
- readAt
- SharedLibrary
- ClapDescriptor
- Fixture
- ChildResult
- RemoveClipsCommand
- make-dmg.sh
- BlockSegment
- ScriptedFactory
- BlobReader
- ImpulseNode
- RemoveTrackCommand
- ClapInstance
- ConstantNode
- SetTrackMutedCommand
- SetTrackSoloedCommand
- MidiRecorder.cpp
- AddNoteCommand
- RecordedEvent
- PluginParameterInfo
- sampleWaveform
- capturePluginState
- ScratchDir
- Version
- TimestampedMidiMessage
- string
- makeTestSignal
- ClapLibrary
- AudioEditorView.mm
- Pattern
- ScratchDirectory
- ParameterFixture
- INCDAWMixerView
- INCDAWPianoRollView
- RecordingSink
- ParameterSink
- ProjectMetadata
- noteAtStep
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- renderBlock
- PluginStateTests.cpp
- INCDAWAudioEditorView
- INCDAWPlaylistView
- ImpulseNode
- check
- create
- AudioCaptureSink
- MidiDeviceInfo
- -applicationDidFinishLaunching
- StateFixture
- SetClipMutedCommand
- .operator==
- Fixture
- AutomationFixture
- emptyOutTryPush
- openEditor
- StateIO
- Fixture

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

## Communities (203 total, 4 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "string"
Cohesion: 0.16
Nodes (6): string, RenamePatternCommand, execute, patternId_, previousName_, undo

### Community 2 - "RenameTrackCommand"
Cohesion: 0.18
Nodes (6): string, RenameTrackCommand, execute, previousName_, trackId_, undo

### Community 3 - "Transport"
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, uint32_t (+17 more)

### Community 4 - "WavStreamWriter"
Cohesion: 0.05
Nodes (59): ofstream, appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample (+51 more)

### Community 5 - "PluginRegistry"
Cohesion: 0.15
Nodes (21): Library, Located, int64_t, path, size_t, string, uint64_t, vector (+13 more)

### Community 6 - "AudioBufferPool"
Cohesion: 0.17
Nodes (9): AudioBufferPool, channelPointers_, reset, samples_, FrameCount, Sample, size_t, unique_ptr (+1 more)

### Community 7 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 8 - "PlaylistModel"
Cohesion: 0.10
Nodes (30): Rect, size_t, Tick, vector, size_t, Tick, vector, Viewport (+22 more)

### Community 9 - "SimpleSynth"
Cohesion: 0.08
Nodes (19): array, atomic, maxVoices, Sample, SampleRate, uint64_t, Voice, Waveform (+11 more)

### Community 10 - "RemoveInsertCommand"
Cohesion: 0.10
Nodes (19): execute, undo, findNode(), size_t, string, RemoveInsertCommand, execute, index_ (+11 more)

### Community 11 - "CommandRegistry"
Cohesion: 0.15
Nodes (9): CommandRegistry, actions_, project_, redoStack_, undoStack_, CommandPtr, Entry, size_t (+1 more)

### Community 12 - "CompiledProjectGraph"
Cohesion: 0.10
Nodes (24): CompiledProjectGraph, automation, channels, channelStripFor, channelStrips, error, graph, insertSlots (+16 more)

### Community 13 - "CommandRegistry.cpp"
Cohesion: 0.17
Nodes (19): clearHistory, execute, executeMerging, findAction, invoke, redo, redoName, registerAction (+11 more)

### Community 14 - "AudioEngine"
Cohesion: 0.07
Nodes (50): RetiredGraph, AudioCaptureSink, AudioEngine, active_, anchor_, anchorVersion_, audioDeviceAboutToStart, audioDeviceStopped (+42 more)

### Community 15 - "TestLatencyPlugin.cpp"
Cohesion: 0.11
Nodes (25): clap_host_t, clap_plugin_descriptor_t, clap_plugin_factory_t, clap_plugin_t, clap_process_status, clap_process_t, vector, factoryCreatePlugin() (+17 more)

### Community 16 - "EditAssetRegionCommand"
Cohesion: 0.09
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
Nodes (26): execute, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 21 - "InstrumentNode"
Cohesion: 0.08
Nodes (24): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+16 more)

### Community 22 - "EntityId"
Cohesion: 0.08
Nodes (34): AddInsertCommand, minted_, mixerNode_, plugin_, slot_, EntityId, invalidValue, friend (+26 more)

### Community 23 - "WavStreamReader"
Cohesion: 0.12
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
Cohesion: 0.14
Nodes (15): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+7 more)

### Community 28 - "MixerView.mm"
Cohesion: 0.25
Nodes (17): -acceptsFirstResponder, -addStripRect, -drawRect, -drawStripnode, -faderRectAt, -initWithFrameprojectregistry, -isFlipped, -meterRectAt (+9 more)

### Community 29 - "AudioStream"
Cohesion: 0.08
Nodes (33): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+25 more)

### Community 30 - "InsertRecordedTakeCommand"
Cohesion: 0.10
Nodes (16): Placement, size_t, string, vector, InsertRecordedTakeCommand, asset_, assetIndex_, clipIndices_ (+8 more)

### Community 31 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 32 - "NoteSequence"
Cohesion: 0.06
Nodes (45): Emit, FrameCount, FramePosition, MidiBuffer, Tick, vector, size_t, Tick (+37 more)

### Community 33 - "Clip"
Cohesion: 0.05
Nodes (39): ClipType, AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id (+31 more)

### Community 34 - "atomic"
Cohesion: 0.19
Nodes (3): atomic, MidiBuffer, array

### Community 35 - "MidiMessage"
Cohesion: 0.11
Nodes (9): FrameCount, uint8_t, MidiMessage, data1, data2, frameOffset, status, vector (+1 more)

### Community 36 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (25): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+17 more)

### Community 37 - "AudioRecorder"
Cohesion: 0.07
Nodes (29): AudioCaptureSink, AudioRecorder, captureAudioBlock, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_ (+21 more)

### Community 38 - "MidiInput"
Cohesion: 0.11
Nodes (18): FrameCount, MidiBuffer, SampleRate, uint64_t, atomic, queueCapacity, size_t, uint64_t (+10 more)

### Community 39 - "CountingCommand"
Cohesion: 0.12
Nodes (8): CountingCommand, counter_, delta_, string, Tick, makeProjectWithNotes(), NoOpCommand, note()

### Community 40 - "TestGainPlugin.cpp"
Cohesion: 0.06
Nodes (57): clap_gui_resize_hints_t, clap_id, clap_param_info_t, clap_window_t, applyParamEvents(), clap_host_t, clap_input_events_t, clap_istream_t (+49 more)

### Community 41 - "GraphCompileOptions"
Cohesion: 0.09
Nodes (23): PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, insertFactory, instrumentFactory, masterGain, maxBlockSize (+15 more)

### Community 42 - "Json"
Cohesion: 0.06
Nodes (60): nullptr_t, int64_t, size_t, string, escapeInto(), formatDouble(), int64_t, pair (+52 more)

### Community 43 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 44 - "MusicalPosition"
Cohesion: 0.10
Nodes (23): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+15 more)

### Community 45 - "MetronomeNode"
Cohesion: 0.08
Nodes (20): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+12 more)

### Community 46 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 47 - "WaveformOverview"
Cohesion: 0.17
Nodes (11): Bucket, FrameCount, SampleRate, size_t, vector, WaveformOverview, channelCount, channels (+3 more)

### Community 48 - "WriteAutomationCommand"
Cohesion: 0.04
Nodes (52): AddAutomationLaneCommand, execute, index_, key_, lane_, minted_, target_, undo (+44 more)

### Community 49 - "Sampler.cpp"
Cohesion: 0.17
Nodes (17): FrameCount, Sample, SampleRate, Voice, interpolate(), activeVoiceCount, allNotesOff, findVoiceToSteal (+9 more)

### Community 50 - "ResizeClipsCommand"
Cohesion: 0.20
Nodes (9): FrameCount, ResizeClipsCommand, canMergeWith, clips_, lengthDelta_, mergeWith, previousFrameLengths_, previousLengths_ (+1 more)

### Community 51 - "MoveNotesCommand"
Cohesion: 0.04
Nodes (43): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, NoteIndices (+35 more)

### Community 52 - "GraphBuilder"
Cohesion: 0.08
Nodes (32): Connection, process, FrameCount, FramePosition, MidiBuffer, NodeIndex, SampleRate, size_t (+24 more)

### Community 53 - "Smoother"
Cohesion: 0.18
Nodes (9): atomic, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds, sampleRate_ (+1 more)

### Community 54 - "DelayLineNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_, prepare (+6 more)

### Community 55 - "CompiledGraph"
Cohesion: 0.13
Nodes (13): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+5 more)

### Community 56 - "PluginPersistenceTests.cpp"
Cohesion: 0.15
Nodes (10): path, string, uint8_t, vector, gainBlob(), Harness, folder, registry (+2 more)

### Community 57 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 58 - "AutomationWriteSession"
Cohesion: 0.15
Nodes (12): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, string, Tick (+4 more)

### Community 59 - "AudioFileData"
Cohesion: 0.15
Nodes (25): applyGain(), applyRamp(), clampedRegion(), Sample, fadeIn(), fadeOut(), FrameCount, normalize() (+17 more)

### Community 60 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 61 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 62 - "EditFixture"
Cohesion: 0.15
Nodes (13): FrameCount, path, Sample, size_t, EditFixture, assetId, file, project (+5 more)

### Community 63 - "AudioEditCommands.cpp"
Cohesion: 0.32
Nodes (12): assetFilePath(), Sample, string, vector, execute, name, undo, findAsset() (+4 more)

### Community 64 - "SamplerZone"
Cohesion: 0.12
Nodes (16): vector, FrameCount, shared_ptr, handleMessage, setZones, SamplerZone, end, gain (+8 more)

### Community 65 - "AudioClipNode"
Cohesion: 0.13
Nodes (13): AudioClipNode, addClip, clips_, fetchScratch_, prepare, process, FrameCount, PlacedClip (+5 more)

### Community 66 - "Command"
Cohesion: 0.02
Nodes (81): RemovedRouting, Command, execute, id, name, undo, AddMixerNodeCommand, execute (+73 more)

### Community 67 - "AudioBufferView"
Cohesion: 0.20
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 68 - "LevelMeter"
Cohesion: 0.15
Nodes (12): atomic, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond, rmsWindowSeconds (+4 more)

### Community 69 - "Project"
Cohesion: 0.11
Nodes (32): undo, NoteIndices, size_t, vector, execute, undo, findEvents(), execute (+24 more)

### Community 70 - "PluginIdentifier"
Cohesion: 0.15
Nodes (11): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+3 more)

### Community 71 - "Node"
Cohesion: 0.24
Nodes (4): Node, process, ParameterSink, StateIO

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
Nodes (37): NoteList, size_t, Tick, vector, size_t, Tick, vector, Viewport (+29 more)

### Community 77 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 78 - "SampleRingBuffer"
Cohesion: 0.19
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 79 - "[Unreleased]"
Cohesion: 0.06
Nodes (34): INCDAW — Changelog, Phase 0 — Research and architecture — 2026-08-14, Phase 10 — Mixer, routing and delay compensation — 2026-08-14, Phase 11a — Automation: the generic subsystem — 2026-08-15, Phase 11b — Automation placement and recording — 2026-08-15, Phase 12 (part 1) — WAV codec — 2026-08-15, Phase 12 (part 2) — Input capture and recording — 2026-08-15, Phase 12 (part 3) — Recording lands in the timeline — 2026-08-15 (+26 more)

### Community 80 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 81 - "MixerTests.cpp"
Cohesion: 0.15
Nodes (12): FrameCount, Sample, SampleRate, size_t, vector, LatentProcessorNode, delay_, latency_ (+4 more)

### Community 82 - "ioProcTrampoline"
Cohesion: 0.23
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 83 - "INCDAWAppDelegate"
Cohesion: 0.20
Nodes (9): NSApplicationDelegate, NSObject, NSSegmentedControl, NSTextField, NSWindow, NSView, INCDAWChannelRackView, -initWithFrameprojectregistry (+1 more)

### Community 84 - "INCDAW — Roadmap"
Cohesion: 0.08
Nodes (23): Deliberately out of scope, INCDAW — Roadmap, Phase 0 — Research and architecture ✅ COMPLETE, Phase 10 — Mixer and routing, Phase 11 — Automation, Phase 12 — Recording and audio editor, Phase 13 — Plugin hosting, Phase 14 — Sampler (+15 more)

### Community 85 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 86 - "ParsedHeader"
Cohesion: 0.16
Nodes (22): Format, path, Result, size_t, uint16_t, uint32_t, uint8_t, vector (+14 more)

### Community 87 - "GainNode"
Cohesion: 0.15
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 88 - "SineOscillatorNode"
Cohesion: 0.13
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 89 - "PluginInsertTests.cpp"
Cohesion: 0.13
Nodes (17): anyNonZero(), ClipInsert, threshold_, FrameCount, Sample, vector, GainInsert, factor_ (+9 more)

### Community 90 - "4. Specialised tests"
Cohesion: 0.11
Nodes (18): 1. Principle, 2. Framework, 3. Test levels, 4. Specialised tests, 5. What is not tested automatically, End-to-end, Fuzzing (from Phase 4), Golden-file audio (from Phase 7) (+10 more)

### Community 91 - "TimingProbeInstrument"
Cohesion: 0.14
Nodes (10): Applied, FrameCount, MidiBuffer, Sample, SampleRate, vector, renderSynth(), TimingProbeInstrument (+2 more)

### Community 92 - "PluginInstanceManager"
Cohesion: 0.12
Nodes (25): Held, size_t, string, uint32_t, uint64_t, unique_ptr, vector, mutex (+17 more)

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
Cohesion: 0.12
Nodes (15): FrameCount, FramePosition, MidiBuffer, SampleRate, size_t, ProcessContext, frameCount, inputCount (+7 more)

### Community 97 - "DuplicateClipsCommand"
Cohesion: 0.19
Nodes (10): ClipIds, DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_ (+2 more)

### Community 98 - "MoveClipsCommand"
Cohesion: 0.22
Nodes (8): MovedAudioClip, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, movedAudio_, tickDelta_, trackDelta_

### Community 99 - "TrackCommands.cpp"
Cohesion: 0.19
Nodes (10): execute, undo, SetTrackHeightCommand, canMergeWith, execute, height_, mergeWith, previousHeight_ (+2 more)

### Community 100 - "PatternCommands.cpp"
Cohesion: 0.24
Nodes (8): SetPatternSwingCommand, canMergeWith, execute, mergeWith, patternId_, previousSwing_, swing_, undo

### Community 101 - "SetPatternLengthCommand"
Cohesion: 0.20
Nodes (9): Tick, SetPatternLengthCommand, canMergeWith, execute, length_, mergeWith, patternId_, previousLength_ (+1 more)

### Community 102 - "INCDAW — Audio Engine"
Cohesion: 0.15
Nodes (12): 10. Audio correctness requirements, 11. Performance budget, 1. The prime directive, 2. Device layer, 3. Realtime thread scheduling, 4. Realtime safety enforcement, 5. Signal flow, 6. Block processing and sample-accurate events (+4 more)

### Community 103 - "renderNode"
Cohesion: 0.21
Nodes (11): FrameCount, path, Sample, shared_ptr, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 104 - "ConnectMixerCommand"
Cohesion: 0.08
Nodes (20): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+12 more)

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

### Community 109 - "renderArrangement"
Cohesion: 0.23
Nodes (10): FrameCount, path, Sample, size_t, vector, makeAudio(), renderArrangement(), ScratchDir (+2 more)

### Community 110 - "vector"
Cohesion: 0.13
Nodes (3): vector, string, mutex

### Community 111 - "AutomationPoint"
Cohesion: 0.17
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 112 - "AutomationNode"
Cohesion: 0.20
Nodes (6): Binding, AutomationNode, bindings_, tempoMap_, size_t, vector

### Community 113 - "AddPatternClipCommand"
Cohesion: 0.17
Nodes (9): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+1 more)

### Community 114 - "MidiEvent"
Cohesion: 0.13
Nodes (15): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+7 more)

### Community 115 - "InputMonitorNode"
Cohesion: 0.15
Nodes (9): FrameCount, Sample, SampleRate, size_t, vector, InputMonitorNode, channelCount_, ring_ (+1 more)

### Community 116 - "ClapLibrary.cpp"
Cohesion: 0.21
Nodes (12): blobRead(), closeEditor, close, open, clap_host_t, clap_istream_t, path, hostGetExtension() (+4 more)

### Community 117 - "humanizeNoteStarts"
Cohesion: 0.29
Nodes (10): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+2 more)

### Community 118 - "main.mm"
Cohesion: 0.29
Nodes (10): -editorChanged, -openAudioAssetInEditor, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll, -showPlaylist, -togglePlayback (+2 more)

### Community 119 - "ClipCommands.cpp"
Cohesion: 0.31
Nodes (8): execute, undo, execute, canMergeWith, execute, mergeWith, undo, trackAtOffset()

### Community 120 - "AddPatternCommand"
Cohesion: 0.20
Nodes (6): AddPatternCommand, execute, index_, minted_, pattern_, undo

### Community 121 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (8): size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_, step_

### Community 122 - "INCDAW — Performance Strategy"
Cohesion: 0.18
Nodes (10): 1. Reference machine, 2. Targets, 3. Instrumentation, 4. Method, 5. Known design-level performance decisions, 6. Profiling tooling, Audio, INCDAW — Performance Strategy (+2 more)

### Community 123 - "INCDAW — Project Format"
Cohesion: 0.18
Nodes (10): 1. Shape: a package directory, not a single file, 2. Versioning and migration, 3. Text vs binary, 4. Media: referenced or embedded, 5. Autosave, backup and recovery, 6. Archiving, 7. Determinism, 8. Tests (Phase 4 gate) (+2 more)

### Community 124 - "RealtimeSafetyTests.cpp"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 125 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 126 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped()

### Community 127 - "build"
Cohesion: 0.27
Nodes (9): bucketize(), Bucket, FrameCount, path, Result, Sample, vector, sizeBuckets() (+1 more)

### Community 128 - "AutomationProbe"
Cohesion: 0.29
Nodes (6): AutomationProbe, calls, registry, written, FramePosition, vector

### Community 129 - "DuplicatePatternCommand"
Cohesion: 0.20
Nodes (7): DuplicatePatternCommand, execute, index_, minted_, pattern_, source_, undo

### Community 130 - "PluginFolder"
Cohesion: 0.24
Nodes (7): path, PluginFolder, crash, dir, gain, ScratchDir, path

### Community 131 - "SimpleSynth.cpp"
Cohesion: 0.27
Nodes (9): size_t, frequencyForKey(), activeVoiceCount, allNotesOff, findVoiceToSteal, handleMessage, releaseVoicesForKey, startVoice (+1 more)

### Community 132 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 133 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 134 - "RemovePatternCommand"
Cohesion: 0.22
Nodes (7): size_t, RemovePatternCommand, execute, index_, pattern_, patternId_, undo

### Community 135 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 136 - "allocate"
Cohesion: 0.19
Nodes (13): allocate, FrameCount, size_t, FramePosition, Sample, vector, render(), anyNonZero() (+5 more)

### Community 137 - "AddTrackCommand"
Cohesion: 0.22
Nodes (7): AddTrackCommand, execute, index_, minted_, track_, undo, size_t

### Community 138 - "readAt"
Cohesion: 0.25
Nodes (8): FrameCount, path, Result, Sample, size_t, close, open, readAt

### Community 139 - "SharedLibrary"
Cohesion: 0.25
Nodes (7): path, string, SharedLibrary, close, handle_, open, symbol

### Community 140 - "ClapDescriptor"
Cohesion: 0.12
Nodes (17): ClapDescriptor, id, name, vendor, version, string, path, string (+9 more)

### Community 141 - "Fixture"
Cohesion: 0.20
Nodes (10): Tick, vector, Fixture, channel, pattern, project, trackA, trackB (+2 more)

### Community 142 - "ChildResult"
Cohesion: 0.18
Nodes (10): End, ChildResult, code, end, output, path, string, vector (+2 more)

### Community 143 - "RemoveClipsCommand"
Cohesion: 0.18
Nodes (9): string, RemovedClip, vector, RemoveClipsCommand, clips_, execute, name, removed_ (+1 more)

### Community 144 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 145 - "BlockSegment"
Cohesion: 0.18
Nodes (9): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, size_t (+1 more)

### Community 146 - "ScriptedFactory"
Cohesion: 0.25
Nodes (7): function, InsertFactory, unique_ptr, ScriptedFactory, fail, makers, requests

### Community 147 - "BlobReader"
Cohesion: 0.22
Nodes (11): BlobReader, cursor, data, BlobWriter, out, overflowed, loadState, saveState (+3 more)

### Community 148 - "ImpulseNode"
Cohesion: 0.40
Nodes (3): FrameCount, ImpulseNode, at_

### Community 149 - "RemoveTrackCommand"
Cohesion: 0.22
Nodes (7): RemovedClip, vector, RemoveTrackCommand, clips_, index_, track_, trackId_

### Community 150 - "ClapInstance"
Cohesion: 0.09
Nodes (22): clap_plugin_gui_t, clap_plugin_state_t, ParamEvent, ClapInstance, editorOpen_, gui_, host_, latency_ (+14 more)

### Community 151 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 152 - "SetTrackMutedCommand"
Cohesion: 0.25
Nodes (5): SetTrackMutedCommand, execute, muted_, trackId_, undo

### Community 153 - "SetTrackSoloedCommand"
Cohesion: 0.25
Nodes (5): SetTrackSoloedCommand, execute, soloed_, trackId_, undo

### Community 154 - "MidiRecorder.cpp"
Cohesion: 0.40
Nodes (4): FramePosition, MidiBuffer, capture, reset

### Community 155 - "AddNoteCommand"
Cohesion: 0.22
Nodes (7): AddNoteCommand, channel_, execute, index_, note_, pattern_, size_t

### Community 156 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 157 - "PluginParameterInfo"
Cohesion: 0.12
Nodes (13): ParameterSink, string, uint32_t, PluginParameterInfo, defaultValue, id, maxValue, minValue (+5 more)

### Community 158 - "sampleWaveform"
Cohesion: 0.25
Nodes (8): FrameCount, SampleRate, Voice, Waveform, polyBlep(), prepare, renderRange, sampleWaveform

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

### Community 167 - "Pattern"
Cohesion: 0.03
Nodes (68): AutomationCurve, AutomationPoint, curve, tension, tick, value, Channel, colour (+60 more)

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

### Community 174 - "ProjectMetadata"
Cohesion: 0.25
Nodes (8): ProjectMetadata, artist, comment, created, createdWith, lastSavedWith, modified, title

### Community 175 - "noteAtStep"
Cohesion: 0.33
Nodes (6): size_t, Tick, vector, noteAtStep(), execute, undo

### Community 176 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 177 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 178 - "renderBlock"
Cohesion: 0.47
Nodes (5): FrameCount, Sample, vector, renderBlock(), tone()

### Community 179 - "PluginStateTests.cpp"
Cohesion: 0.11
Nodes (10): FrameCount, StateIO, PluginNode, instance_, compileLoaded(), InsertFactory, path, factoryFor() (+2 more)

### Community 180 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 181 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 182 - "ImpulseNode"
Cohesion: 0.15
Nodes (9): FramePosition, ImpulseNode, latency_, position_, MixerFixture, channel, pattern, project (+1 more)

### Community 183 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 184 - "create"
Cohesion: 0.25
Nodes (8): clap_event_param_value_t, create, array, string, unique_ptr, PendingParamEvents, count, events

### Community 186 - "MidiDeviceInfo"
Cohesion: 0.40
Nodes (5): string, MidiDeviceInfo, identifier, isInput, name

### Community 187 - "-applicationDidFinishLaunching"
Cohesion: 0.20
Nodes (9): NSScrollView, NSSplitView, -applicationDidFinishLaunching, NSView, -selectChannel, -selectPattern, NSView, INCDAWPatternListView (+1 more)

### Community 188 - "StateFixture"
Cohesion: 0.17
Nodes (11): anyNonZero(), Sample, uint8_t, vector, gainBlob(), render(), StateFixture, channel (+3 more)

### Community 190 - "SetClipMutedCommand"
Cohesion: 0.25
Nodes (6): SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 193 - "Fixture"
Cohesion: 0.25
Nodes (6): Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 195 - "AutomationFixture"
Cohesion: 0.20
Nodes (9): AutomationFixture, channel, pattern, project, tempo, AutomationPoint, Tick, enginePoint() (+1 more)

### Community 196 - "emptyOutTryPush"
Cohesion: 0.33
Nodes (6): clap_event_header_t, clap_input_events_t, clap_output_events_t, emptyOutTryPush(), pendingInGet(), pendingInSize()

### Community 198 - "openEditor"
Cohesion: 0.25
Nodes (8): size, blobWrite(), hasEditor, openEditor, process, setParameter, clap_ostream_t, uint32_t

### Community 200 - "StateIO"
Cohesion: 0.40
Nodes (3): StateIO, loadState, saveState

### Community 203 - "Fixture"
Cohesion: 0.40
Nodes (3): Fixture, project, registry

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
- **1142 isolated node(s):** `streams_`, `enabled_`, `noRow`, `layout_`, `id` (+1137 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **4 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `EntityId` connect `EntityId` to `DuplicatePatternCommand`, `string`, `RenameTrackCommand`, `RemovePatternCommand`, `PlaylistModel`, `AddTrackCommand`, `RemoveInsertCommand`, `CompiledProjectGraph`, `Fixture`, `EditAssetRegionCommand`, `RemoveTrackCommand`, `InstrumentNode`, `SetTrackMutedCommand`, `SetTrackSoloedCommand`, `AddNoteCommand`, `InsertRecordedTakeCommand`, `NoteSequence`, `Clip`, `Pattern`, `CountingCommand`, `GraphCompileOptions`, `Json`, `ParameterFixture`, `WriteAutomationCommand`, `MoveNotesCommand`, `ImpulseNode`, `AutomationWriteSession`, `AudioFileData`, `StateFixture`, `EditFixture`, `AudioEditCommands.cpp`, `Fixture`, `Command`, `AutomationFixture`, `Project`, `PianoRollModel`, `PluginInsertTests.cpp`, `ChannelCommands.cpp`, `TrackCommands.cpp`, `PatternCommands.cpp`, `SetPatternLengthCommand`, `ConnectMixerCommand`, `AddPatternClipCommand`, `ClipCommands.cpp`, `AddPatternCommand`?**
  _High betweenness centrality (0.114) - this node is a cross-community bridge._