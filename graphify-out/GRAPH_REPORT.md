# Graph Report - phase-14-structures-4e203c  (2026-08-16)

## Corpus Check
- 285 files · ~212,363 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 4465 nodes · 7832 edges · 248 communities (243 shown, 5 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 382 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `aee257a3`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- PatternCommands.cpp
- RemoveTrackCommand
- Transport
- WavStreamWriter
- PluginRegistry
- AudioBufferPool
- CoreAudioDevice
- PlaylistModel.cpp
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
- DelayEffect
- AudioStream
- InsertRecordedTakeCommand
- CoreAudioDevice.cpp
- NoteSequence
- Clip
- atomic
- MidiMessage
- EqEffect
- AudioRecorder
- MidiInput
- EntityId
- TestGainPlugin.cpp
- GraphCompileOptions
- LoadSampleCommand
- CallbackProfiler
- MusicalPosition
- MetronomeNode
- AudioDevice
- WaveformOverview
- WriteAutomationCommand
- Sampler.cpp
- CompressorEffect
- SamplerZoneStream
- GraphBuilder
- LevelMeter
- load
- CompiledGraph
- Harness
- RealtimeGuard.cpp
- AutomationWriteSession
- AudioFileData
- BasicMidiBuffer
- SystemInfo
- EditFixture
- read
- SamplerZone
- AudioClipNode
- MixerCommands.cpp
- AudioBufferView
- MixerNode
- Pattern
- PluginIdentifier
- read
- MidiRecorder
- INCDAW — Decision Log
- SamplerTests.cpp
- 2. INCDAW functional scope
- PianoRollModel
- LockFreeQueue
- SampleRingBuffer
- [0.9.0] — 2026-08-16 — the core is complete
- LoopbackResult
- MixerTests.cpp
- ioProcTrampoline
- -applicationDidFinishLaunching
- INCDAW — Roadmap
- AddMidiMappingCommand
- ParsedHeader
- GainNode
- SineOscillatorNode
- PluginInsertTests.cpp
- 4. Specialised tests
- TimingProbeInstrument
- PluginInstanceManager
- INCDAW — Plugin Host
- string
- INCDAW — Architecture
- ProcessContext
- DuplicateClipsCommand
- MoveClipsCommand
- PlaylistView.mm
- BuiltinEffect
- Instrument
- INCDAW — Audio Engine
- RecordingPlacementTests.cpp
- ConnectMixerCommand
- CoreMidiDevice.cpp
- Options
- ParameterRegistry
- ChannelRackView.mm
- AddMixerNodeCommand
- Command
- AutomationPoint
- AutomationNode
- AddPatternClipCommand
- MidiEvent
- Channel
- ClapLibrary.cpp
- RenderOptions
- main.mm
- AddAutomationLaneCommand
- NoteCommands.cpp
- ToggleStepCommand
- INCDAW — Performance Strategy
- INCDAW — Project Format
- RealtimeSafetyTests.cpp
- TimelineAnchor
- MidiTests.cpp
- build
- AutomationProbe
- CountingCommand
- PluginFolder
- FuzzTests.cpp
- BuiltinEffectInfo
- MidiDevice
- CommandRegistry.cpp
- PatternListView.mm
- PluginParameterTests.cpp
- Smoother
- DelayLineNode
- SharedLibrary
- write
- SamplerStreamingTests.cpp
- ChildResult
- RemoveClipsCommand
- make-dmg.sh
- renderClickFrames
- InputMonitorNode
- BlobReader
- Json.cpp
- ClipCommands.cpp
- ClapInstance
- ConstantNode
- ResizeClipsCommand
- PlaylistModel
- Parser
- SampleCache
- RecordedEvent
- PluginNode
- RemoveMixerNodeCommand
- capturePluginState
- SamplerWiringTests.cpp
- Version
- TimestampedMidiMessage
- BuiltinEffect.cpp
- makeTestSignal
- ClapLibrary
- PluginParameterInfo
- TimeSignatureEvent
- ScratchDirectory
- MidiMapNode
- INCDAWMixerView
- Node
- Track
- ParameterSink
- v1.4/Fixture.incdaw/manifest.json
- allocate
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- MidiDeviceInfo
- AutomationCommands.cpp
- AnalyzerEffect
- INCDAWPlaylistView
- DiskStreamer
- check
- renderArrangement
- AudioCaptureSink
- SetAutomationPointsCommand
- ChannelCommands.cpp
- StateFixture
- RemoveAutomationLaneCommand
- RenderResult
- setParameter
- SetChannelOutputCommand
- Fixture
- AutomationFixture
- SetMixerSoloedCommand
- SimpleSynth.cpp
- PluginStateTests.cpp
- collectForBlock
- create
- humanizeNoteStarts
- INCDAW — Release Guide
- Fixture
- scanDirectory
- v1.2/Fixture.incdaw/manifest.json
- v1.3/Fixture.incdaw/manifest.json
- ImpulseNode
- RemoveChannelCommand
- Fixture
- ScanOutcome
- AddChannelCommand
- SetChannelStepKeyCommand
- SetClipMutedCommand
- StressTests.cpp
- ClapDescriptor
- exportArrangement
- SetChannelMutedCommand
- string
- StateIO
- SetChannelSoloedCommand
- sampleWaveform
- MidiImportResult
- ScriptedFactory
- RenderTests.cpp
- SequencedNote
- ScratchDirectory
- INCDAWPianoRollView
- setParameter
- collectForRange
- scanOutOfProcess
- renderProject
- renderBlock
- PluginLatencyTests.cpp
- PatternTests.cpp
- InsertFixture
- snapTick
- OrderRecordingNode
- KernelTable
- .zoneCount
- INCDAWAudioEditorView
- ScratchDir

## God Nodes (most connected - your core abstractions)
1. `Project` - 220 edges
2. `EntityId` - 192 edges
3. `Command` - 109 edges
4. `TempoMap` - 70 edges
5. `AudioEngine` - 68 edges
6. `Sampler` - 61 edges
7. `CoreAudioDevice` - 59 edges
8. `CommandRegistry` - 51 edges
9. `Node` - 50 edges
10. `AudioBufferPool` - 49 edges

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

## Communities (248 total, 5 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "PatternCommands.cpp"
Cohesion: 0.04
Nodes (43): AddPatternCommand, execute, index_, minted_, pattern_, undo, DuplicatePatternCommand, execute (+35 more)

### Community 2 - "RemoveTrackCommand"
Cohesion: 0.04
Nodes (40): AddTrackCommand, execute, index_, minted_, track_, undo, RemovedClip, size_t (+32 more)

### Community 3 - "Transport"
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, uint32_t (+17 more)

### Community 4 - "WavStreamWriter"
Cohesion: 0.05
Nodes (59): ofstream, appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample (+51 more)

### Community 5 - "PluginRegistry"
Cohesion: 0.22
Nodes (14): Library, Located, string, vector, field(), vector, PluginRegistry, clearBlacklist (+6 more)

### Community 6 - "AudioBufferPool"
Cohesion: 0.14
Nodes (11): AudioBufferPool, channelPointers_, reset, samples_, FrameCount, Sample, size_t, unique_ptr (+3 more)

### Community 7 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 8 - "PlaylistModel.cpp"
Cohesion: 0.20
Nodes (18): Rect, size_t, vector, addToSelection, clipAtPoint, clipRect, clipsInRectangle, collectVisibleClips (+10 more)

### Community 9 - "SimpleSynth"
Cohesion: 0.08
Nodes (18): array, atomic, maxVoices, ParameterSink, SampleRate, uint64_t, Voice, SimpleSynth (+10 more)

### Community 10 - "AddInsertCommand"
Cohesion: 0.08
Nodes (24): AddInsertCommand, execute, minted_, mixerNode_, plugin_, slot_, undo, findNode() (+16 more)

### Community 11 - "Json"
Cohesion: 0.10
Nodes (14): nullptr_t, int64_t, pair, string, vector, Json, asBool, boolean_ (+6 more)

### Community 12 - "CompiledProjectGraph"
Cohesion: 0.10
Nodes (24): CompiledProjectGraph, automation, channels, channelStripFor, channelStrips, error, graph, insertSlots (+16 more)

### Community 13 - "CommandRegistry"
Cohesion: 0.15
Nodes (9): CommandRegistry, actions_, project_, redoStack_, undoStack_, CommandPtr, Entry, size_t (+1 more)

### Community 14 - "AudioEngine"
Cohesion: 0.07
Nodes (48): RetiredGraph, AudioCaptureSink, AudioEngine, active_, anchor_, anchorVersion_, audioDeviceAboutToStart, audioDeviceStopped (+40 more)

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
Cohesion: 0.10
Nodes (32): execute, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+24 more)

### Community 21 - "InstrumentNode"
Cohesion: 0.11
Nodes (16): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, ParameterSink, unique_ptr, InstrumentNode (+8 more)

### Community 22 - "Project"
Cohesion: 0.07
Nodes (52): undo, size_t, vector, operator==(), events, totalEventCount, Project, audioAssets_ (+44 more)

### Community 23 - "WavStreamReader"
Cohesion: 0.08
Nodes (29): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+21 more)

### Community 24 - "Sampler"
Cohesion: 0.06
Nodes (26): array, atomic, maxVoices, ParameterSink, SampleRate, uint64_t, vector, Voice (+18 more)

### Community 25 - "MixerStripNode"
Cohesion: 0.13
Nodes (19): FrameCount, Sample, SampleRate, atomic, Sample, MixerStripNode, left_, meter_ (+11 more)

### Community 26 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 27 - "CoreMidiDevice"
Cohesion: 0.14
Nodes (15): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+7 more)

### Community 28 - "DelayEffect"
Cohesion: 0.08
Nodes (24): Allpass, Comb, FrameCount, SampleRate, DelayEffect, capacity_, lines_, maxChannels (+16 more)

### Community 29 - "AudioStream"
Cohesion: 0.10
Nodes (24): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+16 more)

### Community 30 - "InsertRecordedTakeCommand"
Cohesion: 0.10
Nodes (16): Placement, size_t, string, vector, InsertRecordedTakeCommand, asset_, assetIndex_, clipIndices_ (+8 more)

### Community 31 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 32 - "NoteSequence"
Cohesion: 0.17
Nodes (13): Tick, vector, Tick, uint32_t, vector, NoteSequence, byEnd_, clear (+5 more)

### Community 33 - "Clip"
Cohesion: 0.08
Nodes (25): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+17 more)

### Community 34 - "atomic"
Cohesion: 0.20
Nodes (3): atomic, MidiBuffer, array

### Community 35 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 36 - "EqEffect"
Cohesion: 0.09
Nodes (22): Coefficients, FrameCount, SampleRate, EqEffect, bandCount, cached_, coefficients_, maxChannels (+14 more)

### Community 37 - "AudioRecorder"
Cohesion: 0.07
Nodes (28): AudioCaptureSink, AudioRecorder, captureAudioBlock, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_ (+20 more)

### Community 38 - "MidiInput"
Cohesion: 0.14
Nodes (13): atomic, queueCapacity, size_t, uint64_t, MidiInput, dropped_, hasPending_, lastControl_ (+5 more)

### Community 39 - "EntityId"
Cohesion: 0.11
Nodes (16): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, findRouting, RoutingConnection (+8 more)

### Community 40 - "TestGainPlugin.cpp"
Cohesion: 0.06
Nodes (57): clap_gui_resize_hints_t, clap_id, clap_param_info_t, clap_window_t, applyParamEvents(), clap_host_t, clap_input_events_t, clap_istream_t (+49 more)

### Community 41 - "GraphCompileOptions"
Cohesion: 0.08
Nodes (25): PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, insertFactory, instrumentFactory, masterGain, maxBlockSize (+17 more)

### Community 42 - "LoadSampleCommand"
Cohesion: 0.12
Nodes (15): size_t, string, vector, LoadSampleCommand, asset_, assetIndex_, channelId_, created_ (+7 more)

### Community 43 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 44 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 45 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 46 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 47 - "WaveformOverview"
Cohesion: 0.11
Nodes (17): Bucket, FrameCount, SampleRate, size_t, vector, WaveformOverview, channelCount, channels (+9 more)

### Community 48 - "WriteAutomationCommand"
Cohesion: 0.12
Nodes (15): WriteAutomationCommand, clip_, clipIndex_, key_, laneAfter_, laneCreated_, laneId_, laneIndex_ (+7 more)

### Community 49 - "Sampler.cpp"
Cohesion: 0.16
Nodes (22): FrameCount, Sample, SampleRate, vector, Voice, interpolate(), activeVoiceCount, allNotesOff (+14 more)

### Community 50 - "CompressorEffect"
Cohesion: 0.11
Nodes (18): CompressorEffect, envelope_, prepare, reduction_, sampleRate_, FrameCount, SampleRate, GateEffect (+10 more)

### Community 51 - "SamplerZoneStream"
Cohesion: 0.14
Nodes (16): Slot, uint64_t, array, FrameCount, shared_ptr, size_t, SamplerZoneStream, claimSlot (+8 more)

### Community 52 - "GraphBuilder"
Cohesion: 0.10
Nodes (25): Connection, NodeIndex, SampleRate, size_t, unique_ptr, GraphBuilder, addNode, analyse (+17 more)

### Community 53 - "LevelMeter"
Cohesion: 0.15
Nodes (12): atomic, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond, rmsWindowSeconds (+4 more)

### Community 54 - "load"
Cohesion: 0.23
Nodes (19): append, automationPointFrom(), bindUnassignedContent(), path, Result, string, idFrom(), midiEventFrom() (+11 more)

### Community 55 - "CompiledGraph"
Cohesion: 0.13
Nodes (13): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+5 more)

### Community 56 - "Harness"
Cohesion: 0.20
Nodes (7): path, string, Harness, folder, registry, ScratchDir, path

### Community 57 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 58 - "AutomationWriteSession"
Cohesion: 0.14
Nodes (12): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, string, Tick (+4 more)

### Community 59 - "AudioFileData"
Cohesion: 0.15
Nodes (25): applyGain(), applyRamp(), clampedRegion(), Sample, fadeIn(), fadeOut(), FrameCount, normalize() (+17 more)

### Community 60 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 61 - "SystemInfo"
Cohesion: 0.14
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 62 - "EditFixture"
Cohesion: 0.15
Nodes (13): FrameCount, path, Sample, size_t, EditFixture, assetId, file, project (+5 more)

### Community 63 - "read"
Cohesion: 0.19
Nodes (24): assetFilePath(), Sample, string, vector, execute, name, undo, findAsset() (+16 more)

### Community 64 - "SamplerZone"
Cohesion: 0.11
Nodes (18): FrameCount, shared_ptr, handleMessage, SamplerZone, end, gain, keyHigh, keyLow (+10 more)

### Community 65 - "AudioClipNode"
Cohesion: 0.13
Nodes (13): AudioClipNode, addClip, clips_, fetchScratch_, prepare, process, FrameCount, PlacedClip (+5 more)

### Community 66 - "MixerCommands.cpp"
Cohesion: 0.05
Nodes (34): SetMixerMutedCommand, execute, muted_, nodeId_, undo, SetMixerPanCommand, canMergeWith, execute (+26 more)

### Community 67 - "AudioBufferView"
Cohesion: 0.08
Nodes (28): renderAudioBlock, uint64_t, AudioBufferView, channels_, frames_, offset_, FrameCount, Sample (+20 more)

### Community 68 - "MixerNode"
Cohesion: 0.04
Nodes (51): AutomationCurve, AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id (+43 more)

### Community 69 - "Pattern"
Cohesion: 0.09
Nodes (36): Emit, size_t, Tick, vector, noteAtStep(), execute, vector, Pattern (+28 more)

### Community 70 - "PluginIdentifier"
Cohesion: 0.13
Nodes (15): builtinSampler(), builtinSimpleSynth(), Format, string, formatName(), Format, friend, string (+7 more)

### Community 71 - "read"
Cohesion: 0.08
Nodes (39): appendBigU16(), appendBigU32(), appendChunk(), appendVlq(), path, Result, size_t, Tick (+31 more)

### Community 72 - "MidiRecorder"
Cohesion: 0.14
Nodes (14): CapturedMessage, FramePosition, MidiBuffer, atomic, queueCapacity, size_t, uint64_t, MidiRecorder (+6 more)

### Community 73 - "INCDAW — Decision Log"
Cohesion: 0.06
Nodes (34): D-001 — Core implementation language: C++20, D-002 — Build system: CMake + Ninja, D-003 — Audio I/O: CoreAudio HAL directly, no wrapper framework, D-004 — Realtime thread scheduling: os_workgroup / Audio Workgroups, D-005 — Platform strategy: macOS first, Windows later, Linux not precluded, D-006 — UI: AppKit shell + INCDAW-owned Metal-rendered widget layer, D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded), D-008 — Licensing: INCDAW is closed-source (+26 more)

### Community 74 - "SamplerTests.cpp"
Cohesion: 0.18
Nodes (17): constantSample(), FrameCount, MidiBuffer, Sample, shared_ptr, vector, makeEnvelopeTransparent(), nyquistSample() (+9 more)

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

### Community 79 - "[0.9.0] — 2026-08-16 — the core is complete"
Cohesion: 0.05
Nodes (43): [0.9.0] — 2026-08-16 — the core is complete, INCDAW — Changelog, Phase 0 — Research and architecture — 2026-08-14, Phase 10 — Mixer, routing and delay compensation — 2026-08-14, Phase 11a — Automation: the generic subsystem — 2026-08-15, Phase 11b — Automation placement and recording — 2026-08-15, Phase 12 (part 1) — WAV codec — 2026-08-15, Phase 12 (part 2) — Input capture and recording — 2026-08-15 (+35 more)

### Community 80 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 81 - "MixerTests.cpp"
Cohesion: 0.09
Nodes (21): FrameCount, FramePosition, Sample, SampleRate, size_t, vector, ImpulseNode, latency_ (+13 more)

### Community 82 - "ioProcTrampoline"
Cohesion: 0.21
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 83 - "-applicationDidFinishLaunching"
Cohesion: 0.12
Nodes (18): NSApplicationDelegate, NSObject, NSScrollView, NSSegmentedControl, NSSplitView, NSTextField, NSWindow, NSView (+10 more)

### Community 84 - "INCDAW — Roadmap"
Cohesion: 0.08
Nodes (23): Deliberately out of scope, INCDAW — Roadmap, Phase 0 — Research and architecture ✅ COMPLETE, Phase 10 — Mixer and routing, Phase 11 — Automation, Phase 12 — Recording and audio editor, Phase 13 — Plugin hosting, Phase 14 — Sampler (+15 more)

### Community 85 - "AddMidiMappingCommand"
Cohesion: 0.07
Nodes (25): AddMidiMappingCommand, controller_, execute, mapping_, midiChannel_, minted_, parameterKey_, target_ (+17 more)

### Community 86 - "ParsedHeader"
Cohesion: 0.20
Nodes (10): size_t, uint16_t, uint32_t, ParsedHeader, bitsPerSample, channels, dataOffset, dataSize (+2 more)

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

### Community 94 - "string"
Cohesion: 0.18
Nodes (6): string, RenameChannelCommand, channelId_, execute, previousName_, undo

### Community 95 - "INCDAW — Architecture"
Cohesion: 0.12
Nodes (15): 1. Guiding principle, 2. Layer model, 3. Proposed repository structure, 4. Threading model, 5. Data model, 6. Command architecture, 7. Engine boundary, 8. Plugin isolation (+7 more)

### Community 96 - "ProcessContext"
Cohesion: 0.11
Nodes (24): dbToGain(), sumInputsInto(), coefficientFor(), process, size_t, process, process, linkedPeakAt() (+16 more)

### Community 97 - "DuplicateClipsCommand"
Cohesion: 0.19
Nodes (10): ClipIds, DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_ (+2 more)

### Community 98 - "MoveClipsCommand"
Cohesion: 0.22
Nodes (8): MovedAudioClip, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, movedAudio_, tickDelta_, trackDelta_

### Community 99 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (24): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+16 more)

### Community 100 - "BuiltinEffect"
Cohesion: 0.08
Nodes (20): BuiltinEffect, values_, EffectParameter, defaultValue, id, maxValue, minValue, name (+12 more)

### Community 101 - "Instrument"
Cohesion: 0.14
Nodes (10): MidiBuffer, ParameterSink, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare (+2 more)

### Community 102 - "INCDAW — Audio Engine"
Cohesion: 0.15
Nodes (12): 10. Audio correctness requirements, 11. Performance budget, 1. The prime directive, 2. Device layer, 3. Realtime thread scheduling, 4. Realtime safety enforcement, 5. Signal flow, 6. Block processing and sample-accurate events (+4 more)

### Community 103 - "RecordingPlacementTests.cpp"
Cohesion: 0.21
Nodes (11): FrameCount, path, Sample, shared_ptr, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 104 - "ConnectMixerCommand"
Cohesion: 0.13
Nodes (11): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+3 more)

### Community 105 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 106 - "Options"
Cohesion: 0.12
Nodes (17): int64_t, string, Options, amplitude, buffer, device, frequency, input (+9 more)

### Community 107 - "ParameterRegistry"
Cohesion: 0.18
Nodes (19): Applier, convertParameters(), Entry, size_t, string, uint32_t, vector, Entry (+11 more)

### Community 108 - "ChannelRackView.mm"
Cohesion: 0.26
Nodes (12): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+4 more)

### Community 109 - "AddMixerNodeCommand"
Cohesion: 0.11
Nodes (14): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType (+6 more)

### Community 110 - "Command"
Cohesion: 0.08
Nodes (12): vector, Command, execute, id, name, undo, string, ParameterSink (+4 more)

### Community 111 - "AutomationPoint"
Cohesion: 0.17
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 112 - "AutomationNode"
Cohesion: 0.20
Nodes (6): AutomationNode, bindings_, tempoMap_, Binding, size_t, vector

### Community 113 - "AddPatternClipCommand"
Cohesion: 0.17
Nodes (9): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+1 more)

### Community 114 - "MidiEvent"
Cohesion: 0.09
Nodes (20): AddNoteCommand, channel_, execute, index_, note_, pattern_, size_t, MidiEventType (+12 more)

### Community 115 - "Channel"
Cohesion: 0.07
Nodes (28): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+20 more)

### Community 116 - "ClapLibrary.cpp"
Cohesion: 0.12
Nodes (22): clap_event_header_t, blobRead(), size, blobWrite(), closeEditor, hasEditor, openEditor, process (+14 more)

### Community 117 - "RenderOptions"
Cohesion: 0.11
Nodes (19): BitDepth, FramePosition, SampleRate, uint64_t, RenderOptions, bitDepth, blockSize, dither (+11 more)

### Community 118 - "main.mm"
Cohesion: 0.29
Nodes (10): -editorChanged, -openAudioAssetInEditor, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll, -showPlaylist, -togglePlayback (+2 more)

### Community 119 - "AddAutomationLaneCommand"
Cohesion: 0.15
Nodes (8): AddAutomationLaneCommand, execute, index_, key_, lane_, minted_, target_, string

### Community 120 - "NoteCommands.cpp"
Cohesion: 0.04
Nodes (59): undo, NoteIndices, size_t, string, vector, DeleteNotesCommand, channel_, execute (+51 more)

### Community 121 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (8): size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_, step_

### Community 122 - "INCDAW — Performance Strategy"
Cohesion: 0.13
Nodes (14): 1. Reference machine, 2. Targets, 3. Instrumentation, 4. Method, 5. Known design-level performance decisions, 6. Profiling tooling, 7. Phase 18 — measured baseline and optimisations, Audio (+6 more)

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

### Community 129 - "CountingCommand"
Cohesion: 0.12
Nodes (8): CountingCommand, counter_, delta_, string, Tick, makeProjectWithNotes(), NoOpCommand, note()

### Community 130 - "PluginFolder"
Cohesion: 0.24
Nodes (7): path, PluginFolder, crash, dir, gain, ScratchDir, path

### Community 131 - "FuzzTests.cpp"
Cohesion: 0.16
Nodes (13): corrupt(), path, size_t, string, uint64_t, uint8_t, vector, Random (+5 more)

### Community 132 - "BuiltinEffectInfo"
Cohesion: 0.17
Nodes (14): BuiltinEffectInfo, displayName, parameterCount, parameters, uid, CatalogueEntry, info, make (+6 more)

### Community 133 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 134 - "CommandRegistry.cpp"
Cohesion: 0.17
Nodes (19): clearHistory, execute, executeMerging, findAction, invoke, redo, redoName, registerAction (+11 more)

### Community 135 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 136 - "PluginParameterTests.cpp"
Cohesion: 0.12
Nodes (18): anyNonZero(), pair, ParameterSink, path, Sample, uint32_t, vector, ParameterFixture (+10 more)

### Community 137 - "Smoother"
Cohesion: 0.18
Nodes (9): atomic, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds, sampleRate_ (+1 more)

### Community 138 - "DelayLineNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_, prepare (+6 more)

### Community 139 - "SharedLibrary"
Cohesion: 0.25
Nodes (7): path, string, SharedLibrary, close, handle_, open, symbol

### Community 140 - "write"
Cohesion: 0.19
Nodes (16): int32_t, AiffFile, write, appendBigU16(), appendBigU32(), appendExtended(), appendId(), Format (+8 more)

### Community 141 - "SamplerStreamingTests.cpp"
Cohesion: 0.13
Nodes (18): FrameCount, MidiBuffer, path, Sample, shared_ptr, size_t, string, vector (+10 more)

### Community 142 - "ChildResult"
Cohesion: 0.18
Nodes (10): End, ChildResult, code, end, output, path, string, vector (+2 more)

### Community 143 - "RemoveClipsCommand"
Cohesion: 0.18
Nodes (9): string, RemovedClip, vector, RemoveClipsCommand, clips_, execute, name, removed_ (+1 more)

### Community 144 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 145 - "renderClickFrames"
Cohesion: 0.15
Nodes (12): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, FramePosition (+4 more)

### Community 146 - "InputMonitorNode"
Cohesion: 0.15
Nodes (9): FrameCount, Sample, SampleRate, size_t, vector, InputMonitorNode, channelCount_, ring_ (+1 more)

### Community 147 - "BlobReader"
Cohesion: 0.22
Nodes (11): BlobReader, cursor, data, BlobWriter, out, overflowed, loadState, saveState (+3 more)

### Community 148 - "Json.cpp"
Cohesion: 0.22
Nodes (13): int64_t, size_t, string, escapeInto(), formatDouble(), asDouble, asInt, asString (+5 more)

### Community 149 - "ClipCommands.cpp"
Cohesion: 0.31
Nodes (8): execute, undo, execute, canMergeWith, execute, mergeWith, undo, trackAtOffset()

### Community 150 - "ClapInstance"
Cohesion: 0.09
Nodes (22): clap_plugin_gui_t, clap_plugin_state_t, ParamEvent, ClapInstance, editorOpen_, gui_, host_, latency_ (+14 more)

### Community 151 - "ConstantNode"
Cohesion: 0.22
Nodes (6): ConstantNode, latency_, value_, FrameCount, Sample, size_t

### Community 152 - "ResizeClipsCommand"
Cohesion: 0.20
Nodes (9): FrameCount, ResizeClipsCommand, canMergeWith, clips_, lengthDelta_, mergeWith, previousFrameLengths_, previousLengths_ (+1 more)

### Community 153 - "PlaylistModel"
Cohesion: 0.16
Nodes (10): size_t, Tick, vector, Viewport, PlaylistModel, noClip, noTrack, resizeHandleWidth (+2 more)

### Community 154 - "Parser"
Cohesion: 0.30
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 155 - "SampleCache"
Cohesion: 0.15
Nodes (16): int64_t, path, shared_ptr, size_t, string, Entry, mutex, string (+8 more)

### Community 156 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 157 - "PluginNode"
Cohesion: 0.20
Nodes (5): FrameCount, ParameterSink, StateIO, PluginNode, instance_

### Community 158 - "RemoveMixerNodeCommand"
Cohesion: 0.09
Nodes (17): RemovedRouting, DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t (+9 more)

### Community 159 - "capturePluginState"
Cohesion: 0.51
Nodes (9): capturePluginState(), path, string, uint8_t, vector, readBlobFile(), restorePluginState(), stateFileNameFor() (+1 more)

### Community 160 - "SamplerWiringTests.cpp"
Cohesion: 0.16
Nodes (12): path, Sample, string, noteAtZero(), renderedPeak(), SamplerProject, asset, channel (+4 more)

### Community 161 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 162 - "TimestampedMidiMessage"
Cohesion: 0.22
Nodes (9): midiMessageReceived, sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos (+1 more)

### Community 163 - "BuiltinEffect.cpp"
Cohesion: 0.34
Nodes (13): appendF64(), appendU32(), BuiltinEffect::BuiltinEffect(), loadState, saveState, setParameter, value, size_t (+5 more)

### Community 164 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 165 - "ClapLibrary"
Cohesion: 0.14
Nodes (12): clap_plugin_entry_t, ClapLibrary, close, descriptors, entry_, factory_, library_, open (+4 more)

### Community 166 - "PluginParameterInfo"
Cohesion: 0.14
Nodes (11): unordered_map, mutex, string, uint32_t, PluginParameterInfo, defaultValue, id, maxValue (+3 more)

### Community 167 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 168 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 169 - "MidiMapNode"
Cohesion: 0.12
Nodes (10): Binding, size_t, vector, MidiMapNode, bindings_, controlChange(), path, string (+2 more)

### Community 170 - "INCDAWMixerView"
Cohesion: 0.15
Nodes (25): incdaw, NSArray, NSDictionary, NSView, INCDAWMixerView, -acceptsFirstResponder, -addStripRect, -drawRect (+17 more)

### Community 171 - "Node"
Cohesion: 0.19
Nodes (6): FrameCount, SampleRate, Node, process, ParameterSink, StateIO

### Community 172 - "Track"
Cohesion: 0.17
Nodes (12): findTrack, Track, colour, height, id, muted, name, outputMixerNode (+4 more)

### Community 174 - "v1.4/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 175 - "allocate"
Cohesion: 0.22
Nodes (7): allocate, FrameCount, size_t, FramePosition, Sample, vector, render()

### Community 176 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 177 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 178 - "MidiDeviceInfo"
Cohesion: 0.40
Nodes (5): string, MidiDeviceInfo, identifier, isInput, name

### Community 179 - "AutomationCommands.cpp"
Cohesion: 0.24
Nodes (10): undo, AutomationPoint, vector, findLane(), canMergeWith, execute, undo, sortPoints() (+2 more)

### Community 180 - "AnalyzerEffect"
Cohesion: 0.22
Nodes (5): AnalyzerEffect, maxChannels, process, atomic, size_t

### Community 181 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 182 - "DiskStreamer"
Cohesion: 0.20
Nodes (10): DiskStreamer, mutex_, running_, serviceOnce, streams_, thread_, atomic, mutex (+2 more)

### Community 183 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 184 - "renderArrangement"
Cohesion: 0.23
Nodes (10): FrameCount, path, Sample, size_t, vector, makeAudio(), renderArrangement(), ScratchDir (+2 more)

### Community 186 - "SetAutomationPointsCommand"
Cohesion: 0.27
Nodes (7): AutomationPoint, vector, SetAutomationPointsCommand, laneId_, mergeWith, points_, previous_

### Community 187 - "ChannelCommands.cpp"
Cohesion: 0.21
Nodes (9): execute, SetChannelVolumeCommand, canMergeWith, channelId_, execute, mergeWith, previousVolume_, undo (+1 more)

### Community 188 - "StateFixture"
Cohesion: 0.17
Nodes (11): anyNonZero(), Sample, uint8_t, vector, gainBlob(), render(), StateFixture, channel (+3 more)

### Community 190 - "RemoveAutomationLaneCommand"
Cohesion: 0.22
Nodes (7): size_t, RemoveAutomationLaneCommand, execute, index_, lane_, laneId_, undo

### Community 191 - "RenderResult"
Cohesion: 0.20
Nodes (9): FrameCount, string, vector, RenderResult, arrangementFrames, audio, error, succeeded (+1 more)

### Community 192 - "setParameter"
Cohesion: 0.22
Nodes (3): FilterMode, uint32_t, setParameter

### Community 193 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 194 - "Fixture"
Cohesion: 0.20
Nodes (10): Tick, vector, Fixture, channel, pattern, project, trackA, trackB (+2 more)

### Community 195 - "AutomationFixture"
Cohesion: 0.20
Nodes (9): AutomationFixture, channel, pattern, project, tempo, AutomationPoint, Tick, enginePoint() (+1 more)

### Community 196 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 197 - "SimpleSynth.cpp"
Cohesion: 0.27
Nodes (9): size_t, frequencyForKey(), activeVoiceCount, allNotesOff, findVoiceToSteal, handleMessage, releaseVoicesForKey, startVoice (+1 more)

### Community 198 - "PluginStateTests.cpp"
Cohesion: 0.29
Nodes (6): compileLoaded(), InsertFactory, path, factoryFor(), ScratchDir, path

### Community 199 - "collectForBlock"
Cohesion: 0.29
Nodes (6): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock, resetCounters

### Community 200 - "create"
Cohesion: 0.25
Nodes (8): clap_event_param_value_t, create, array, string, unique_ptr, PendingParamEvents, count, events

### Community 201 - "humanizeNoteStarts"
Cohesion: 0.29
Nodes (10): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+2 more)

### Community 202 - "INCDAW — Release Guide"
Cohesion: 0.29
Nodes (6): 1. What a release is, 2. Cutting a release, 3. Installing (first launch on another Mac), 4. Updating, 5. Release notes — 0.9.0 (2026-08-16), INCDAW — Release Guide

### Community 203 - "Fixture"
Cohesion: 0.25
Nodes (6): Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 204 - "scanDirectory"
Cohesion: 0.38
Nodes (7): int64_t, path, size_t, uint64_t, mtimeSecondsOf(), scanDirectory, sizeOf()

### Community 205 - "v1.2/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 206 - "v1.3/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 207 - "ImpulseNode"
Cohesion: 0.50
Nodes (3): FrameCount, ImpulseNode, at_

### Community 208 - "RemoveChannelCommand"
Cohesion: 0.20
Nodes (8): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, index_, undo

### Community 209 - "Fixture"
Cohesion: 0.40
Nodes (3): Fixture, project, registry

### Community 211 - "ScanOutcome"
Cohesion: 0.29
Nodes (7): string, vector, ScanOutcome, detail, plugins, status, Status

### Community 212 - "AddChannelCommand"
Cohesion: 0.22
Nodes (7): AddChannelCommand, channel_, execute, index_, minted_, undo, size_t

### Community 214 - "SetChannelStepKeyCommand"
Cohesion: 0.22
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 215 - "SetClipMutedCommand"
Cohesion: 0.25
Nodes (6): SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 216 - "StressTests.cpp"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 217 - "ClapDescriptor"
Cohesion: 0.33
Nodes (6): ClapDescriptor, id, name, vendor, version, string

### Community 218 - "exportArrangement"
Cohesion: 0.25
Nodes (7): size_t, notes_, path, Result, uint64_t, exportArrangement(), importAsPattern()

### Community 219 - "SetChannelMutedCommand"
Cohesion: 0.25
Nodes (5): SetChannelMutedCommand, channelId_, execute, muted_, undo

### Community 221 - "StateIO"
Cohesion: 0.40
Nodes (3): StateIO, loadState, saveState

### Community 222 - "SetChannelSoloedCommand"
Cohesion: 0.25
Nodes (5): SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 223 - "sampleWaveform"
Cohesion: 0.25
Nodes (8): FrameCount, SampleRate, Voice, Waveform, polyBlep(), prepare, renderRange, sampleWaveform

### Community 224 - "MidiImportResult"
Cohesion: 0.25
Nodes (7): string, vector, MidiImportResult, error, newChannels, pattern, succeeded

### Community 225 - "ScriptedFactory"
Cohesion: 0.25
Nodes (7): function, InsertFactory, unique_ptr, ScriptedFactory, fail, makers, requests

### Community 227 - "RenderTests.cpp"
Cohesion: 0.25
Nodes (5): path, string, makeArrangedProject(), ScratchDirectory, path

### Community 229 - "SequencedNote"
Cohesion: 0.29
Nodes (6): SequencedNote, channel, key, lengthTicks, startTick, velocity

### Community 231 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 232 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 233 - "setParameter"
Cohesion: 0.33
Nodes (4): uint32_t, Sample, Waveform, setParameter

### Community 234 - "collectForRange"
Cohesion: 0.33
Nodes (4): FrameCount, FramePosition, MidiBuffer, collectForRange

### Community 235 - "scanOutOfProcess"
Cohesion: 0.60
Nodes (4): path, string, parseLine(), scanOutOfProcess()

### Community 236 - "renderProject"
Cohesion: 0.29
Nodes (6): path, uint64_t, DitherSource, state_, renderProject(), renderProjectToFile()

### Community 237 - "renderBlock"
Cohesion: 0.47
Nodes (5): FrameCount, Sample, vector, renderBlock(), tone()

### Community 238 - "PluginLatencyTests.cpp"
Cohesion: 0.40
Nodes (3): path, ScratchDir, path

### Community 239 - "PatternTests.cpp"
Cohesion: 0.53
Nodes (5): Tick, vector, note(), shapeOf(), startsOf()

### Community 240 - "InsertFixture"
Cohesion: 0.33
Nodes (4): InsertFixture, pattern, project, tempo

### Community 242 - "OrderRecordingNode"
Cohesion: 0.17
Nodes (8): vector, OrderRecordingNode, identifier_, log_, SometimesSilentNode, writeThisBlock, TestOutput, pool

### Community 243 - "KernelTable"
Cohesion: 0.21
Nodes (9): SampleRate, vector, KernelTable, phases, weights_, width, resample(), sinc() (+1 more)

### Community 245 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 246 - "ScratchDir"
Cohesion: 0.50
Nodes (3): path, ScratchDir, path

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
- **1337 isolated node(s):** `streams_`, `enabled_`, `noRow`, `layout_`, `id` (+1332 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **5 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `AutomationProbe`, `PatternCommands.cpp`, `RemoveTrackCommand`, `CountingCommand`, `PlaylistModel.cpp`, `PluginParameterTests.cpp`, `AddInsertCommand`, `CompiledProjectGraph`, `CommandRegistry`, `RemoveClipsCommand`, `TempoMap`, `ClipCommands.cpp`, `ResizeClipsCommand`, `RemoveMixerNodeCommand`, `InsertRecordedTakeCommand`, `capturePluginState`, `Clip`, `SamplerWiringTests.cpp`, `EntityId`, `LoadSampleCommand`, `Track`, `AutomationCommands.cpp`, `load`, `renderArrangement`, `ChannelCommands.cpp`, `StateFixture`, `RemoveAutomationLaneCommand`, `read`, `EditFixture`, `SetChannelOutputCommand`, `MixerCommands.cpp`, `AutomationFixture`, `SetMixerSoloedCommand`, `Pattern`, `MixerNode`, `Fixture`, `PluginStateTests.cpp`, `Fixture`, `RemoveChannelCommand`, `Fixture`, `MixerTests.cpp`, `AddChannelCommand`, `AddMidiMappingCommand`, `SetChannelStepKeyCommand`, `SetClipMutedCommand`, `exportArrangement`, `SetChannelMutedCommand`, `string`, `SetChannelSoloedCommand`, `DuplicateClipsCommand`, `RenderTests.cpp`, `ConnectMixerCommand`, `renderProject`, `AddMixerNodeCommand`, `InsertFixture`, `MidiEvent`, `Channel`, `AddAutomationLaneCommand`, `NoteCommands.cpp`?**
  _High betweenness centrality (0.134) - this node is a cross-community bridge._