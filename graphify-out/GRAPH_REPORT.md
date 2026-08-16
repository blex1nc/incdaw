# Graph Report - onay-devam-6548af  (2026-08-16)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 4564 nodes · 7887 edges · 264 communities (236 shown, 28 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 377 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `d6d6e07e`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Command
- INCDAW
- NoteCommands.cpp
- ChannelCommands.cpp
- Project
- WavStreamWriter
- PatternCommands.cpp
- TestGainPlugin.cpp
- RemoveTrackCommand
- MixerStripNode
- AudioEngine
- PianoRollModel
- ConnectMixerCommand
- read
- CoreAudioDevice
- [0.9.0] — 2026-08-16 — the core is complete
- AudioStream
- TempoMap
- Transport
- AddInsertCommand
- GraphBuilder
- INCDAW — Decision Log
- Pattern
- WavStreamReader
- CompressorEffect
- MixerTests.cpp
- ProcessContext
- Sampler
- ChannelRackModel
- AudioRecorder
- DelayEffect
- ClapLibrary.cpp
- SampleRingBuffer
- SampleCache
- SimpleSynth
- 2. INCDAW functional scope
- AudioFileData
- MetronomeNode
- AudioLogger
- TestLatencyPlugin.cpp
- ClapInstance
- AudioDeviceConfig
- PluginInstanceManager
- INCDAWMixerView
- MusicalPosition
- RecordingSession
- EditAssetRegionCommand
- CoreAudioDevice.cpp
- GraphCompileOptions
- AddMidiMappingCommand
- BuiltinEffect
- vector
- Clip
- PlaylistView.mm
- INCDAW — Roadmap
- Sampler.cpp
- MidiEvent
- ConstantNode
- -applicationDidFinishLaunching
- PluginRegistry
- Json
- CallbackProfiler
- MidiMessage
- AudioDevice
- PluginIdentifier
- InsertRecordedTakeCommand
- Model.h
- EqEffect
- LoadSampleCommand
- read
- AudioBufferView
- CompiledProjectGraph
- PluginInsertTests.cpp
- PluginStateTests.cpp
- SamplerStreamingTests.cpp
- compileProjectGraph
- ClapLibrary
- CommandRegistry.cpp
- WaveformOverview
- InstrumentNode
- MixerNode
- RealtimeGuard.cpp
- ParameterRegistry
- RenderOptions
- 4. Specialised tests
- SamplerZoneStream
- PlaylistModel.cpp
- CompiledGraph
- SamplerZone
- BasicMidiBuffer
- MidiInput
- load
- FuzzTests.cpp
- write
- PlaylistModel
- DelayLineNode
- SimpleSynth.cpp
- SystemInfo
- SamplerTests.cpp
- TimingProbeInstrument
- ioProcTrampoline
- INCDAW — Plugin Host
- capturePluginState
- CommandRegistry
- AudioClipNode
- LockFreeQueue
- GainNode
- SineOscillatorNode
- NoteSequence
- EditFixture
- LoopbackResult
- BuiltinEffectTests.cpp
- INCDAW — Architecture
- CoreMidiDevice
- AutomationWriteSession
- WriteAutomationCommand
- atomic
- BuiltinEffect.cpp
- EffectParameter
- BuiltinEffectInfo
- Json.cpp
- AutomationPoint
- INCDAW — Performance Strategy
- write
- AddAutomationLaneCommand
- AudioEngine.h
- AudioBufferPool
- .buffer
- Node
- Instrument
- Parser
- renderClickFrames
- ChannelSamplerZone
- Options
- INCDAW — Audio Engine
- AutomationCommands.cpp
- ToggleStepCommand
- Channel
- ChannelRackView.mm
- PluginPersistenceTests.cpp
- RecordingPlacementTests.cpp
- MidiRecorder
- CoreMidiDevice.cpp
- allocate
- KernelTable
- ScanOutcome
- Track
- renderArrangement
- Fixture
- INCDAW — Project Format
- ChildResult
- AddPatternClipCommand
- AutomationNode
- TimeSignatureEvent
- MidiDevice
- SharedLibrary
- BlobReader
- PluginNode
- AudioAsset
- renderProject
- INCDAWInsertParameterPanel
- main.mm
- PatternListView.mm
- ClipCommands.cpp
- RemoveClipsCommand
- build
- TimestampedMidiMessage
- RecordedEvent
- PluginParameterInfo
- RenderResult
- PluginFolder
- make-dmg.sh
- setParameter
- MoveClipsCommand
- ProjectGraphCompiler.h
- SetAutomationPointsCommand
- RemoveAutomationLaneCommand
- DuplicateClipsCommand
- ResizeClipsCommand
- MidiMappingTests.cpp
- MidiMapNode
- exportArrangement
- friend
- MidiMapping
- AutomationFixture
- ImpulseNode
- ParameterFixture
- ClipIds
- Denormals.h
- SetClipMutedCommand
- updatedRecents
- Version
- AnalyzerEffect
- MidiDeviceInfo
- MidiImportResult
- ProjectMetadata
- Fixture
- AutomationProbe
- MidiTests.cpp
- ScriptedFactory
- RenderTests.cpp
- makeTestSignal
- INCDAW — Release Guide
- collectForBlock
- SequencedNote
- CarriedInsertState
- ScratchDirectory
- ScratchDirectory
- StressTests.cpp
- INCDAWPianoRollView
- string
- collectForRange
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- v1.2/Fixture.incdaw/manifest.json
- v1.3/Fixture.incdaw/manifest.json
- v1.4/Fixture.incdaw/manifest.json
- renderBlock
- PatternTests.cpp
- InsertFixture
- SessionFixture
- StateIO
- MidiRecorder.cpp
- INCDAWAudioEditorView
- INCDAWPlaylistView
- AutomationTests.cpp
- Fixture
- check
- AudioCaptureSink
- ParameterSink
- INCDAWPatternListView
- ScratchDir
- snapTick
- .zoneCount
- .operator==
- .size
- NSObject
- NSScrollView
- NSWindow
- atomic
- array
- clap_input_events_t
- clap_istream_t
- clap_ostream_t
- clap_output_events_t
- path
- clap_plugin_factory_t
- clap_plugin_t
- int64_t
- path
- FrameCount
- InsertFactory
- SampleRate
- uint64_t
- path
- EntityId
- arrangementEndFrames

## God Nodes (most connected - your core abstractions)
1. `Project` - 216 edges
2. `EntityId` - 184 edges
3. `Command` - 109 edges
4. `AudioEngine` - 68 edges
5. `TempoMap` - 66 edges
6. `Sampler` - 61 edges
7. `CoreAudioDevice` - 59 edges
8. `CommandRegistry` - 50 edges
9. `AudioBufferPool` - 50 edges
10. `Node` - 48 edges

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

## Communities (264 total, 28 thin omitted)

### Community 0 - "Command"
Cohesion: 0.02
Nodes (81): RemovedRouting, Command, execute, id, name, undo, AddMixerNodeCommand, execute (+73 more)

### Community 1 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 2 - "NoteCommands.cpp"
Cohesion: 0.04
Nodes (64): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, NoteIndices (+56 more)

### Community 3 - "ChannelCommands.cpp"
Cohesion: 0.04
Nodes (46): RemovedContent, AddChannelCommand, channel_, execute, index_, minted_, undo, size_t (+38 more)

### Community 4 - "Project"
Cohesion: 0.06
Nodes (33): Project, audioAssets_, automation_, channels_, clips_, findMixerNode, ids_, master_ (+25 more)

### Community 5 - "WavStreamWriter"
Cohesion: 0.05
Nodes (59): ofstream, appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample (+51 more)

### Community 6 - "PatternCommands.cpp"
Cohesion: 0.04
Nodes (43): AddPatternCommand, execute, index_, minted_, pattern_, undo, DuplicatePatternCommand, execute (+35 more)

### Community 7 - "TestGainPlugin.cpp"
Cohesion: 0.06
Nodes (57): clap_gui_resize_hints_t, clap_id, clap_param_info_t, clap_window_t, applyParamEvents(), clap_host_t, clap_input_events_t, clap_istream_t (+49 more)

### Community 8 - "RemoveTrackCommand"
Cohesion: 0.05
Nodes (40): AddTrackCommand, execute, index_, minted_, track_, undo, RemovedClip, size_t (+32 more)

### Community 9 - "MixerStripNode"
Cohesion: 0.05
Nodes (40): atomic, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond, rmsWindowSeconds (+32 more)

### Community 10 - "AudioEngine"
Cohesion: 0.07
Nodes (52): RetiredGraph, AudioEngine, active_, anchor_, anchorVersion_, audioDeviceAboutToStart, audioDeviceStopped, availableDevices (+44 more)

### Community 11 - "PianoRollModel"
Cohesion: 0.07
Nodes (39): NoteList, size_t, Tick, vector, size_t, Tick, vector, Viewport (+31 more)

### Community 12 - "ConnectMixerCommand"
Cohesion: 0.08
Nodes (20): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+12 more)

### Community 13 - "read"
Cohesion: 0.09
Nodes (39): appendBigU16(), appendBigU32(), appendChunk(), appendVlq(), path, Result, size_t, Tick (+31 more)

### Community 14 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 15 - "[0.9.0] — 2026-08-16 — the core is complete"
Cohesion: 0.05
Nodes (43): [0.9.0] — 2026-08-16 — the core is complete, INCDAW — Changelog, Phase 0 — Research and architecture — 2026-08-14, Phase 10 — Mixer, routing and delay compensation — 2026-08-14, Phase 11a — Automation: the generic subsystem — 2026-08-15, Phase 11b — Automation placement and recording — 2026-08-15, Phase 12 (part 1) — WAV codec — 2026-08-15, Phase 12 (part 2) — Input capture and recording — 2026-08-15 (+35 more)

### Community 16 - "AudioStream"
Cohesion: 0.08
Nodes (33): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+25 more)

### Community 17 - "TempoMap"
Cohesion: 0.11
Nodes (28): execute, execute, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition (+20 more)

### Community 18 - "Transport"
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, uint32_t (+17 more)

### Community 19 - "AddInsertCommand"
Cohesion: 0.08
Nodes (24): AddInsertCommand, execute, minted_, mixerNode_, plugin_, slot_, undo, findNode() (+16 more)

### Community 20 - "GraphBuilder"
Cohesion: 0.09
Nodes (29): Connection, process, FrameCount, FramePosition, MidiBuffer, NodeIndex, SampleRate, size_t (+21 more)

### Community 21 - "INCDAW — Decision Log"
Cohesion: 0.06
Nodes (34): D-001 — Core implementation language: C++20, D-002 — Build system: CMake + Ninja, D-003 — Audio I/O: CoreAudio HAL directly, no wrapper framework, D-004 — Realtime thread scheduling: os_workgroup / Audio Workgroups, D-005 — Platform strategy: macOS first, Windows later, Linux not precluded, D-006 — UI: AppKit shell + INCDAW-owned Metal-rendered widget layer, D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded), D-008 — Licensing: INCDAW is closed-source (+26 more)

### Community 22 - "Pattern"
Cohesion: 0.10
Nodes (33): Emit, size_t, Tick, vector, noteAtStep(), execute, undo, Pattern (+25 more)

### Community 23 - "WavStreamReader"
Cohesion: 0.07
Nodes (29): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+21 more)

### Community 24 - "CompressorEffect"
Cohesion: 0.09
Nodes (25): dbToGain(), coefficientFor(), CompressorEffect, envelope_, prepare, process, reduction_, sampleRate_ (+17 more)

### Community 25 - "MixerTests.cpp"
Cohesion: 0.09
Nodes (21): FrameCount, FramePosition, Sample, SampleRate, size_t, vector, ImpulseNode, latency_ (+13 more)

### Community 26 - "ProcessContext"
Cohesion: 0.08
Nodes (21): ProcessContext, size_t, sumInputsInto(), process, process, updateCoefficients, process, SaturatorEffect (+13 more)

### Community 27 - "Sampler"
Cohesion: 0.06
Nodes (26): array, atomic, maxVoices, ParameterSink, SampleRate, uint64_t, vector, Voice (+18 more)

### Community 28 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 29 - "AudioRecorder"
Cohesion: 0.08
Nodes (27): AudioRecorder, captureAudioBlock, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+19 more)

### Community 30 - "DelayEffect"
Cohesion: 0.08
Nodes (25): Allpass, Comb, FrameCount, SampleRate, DelayEffect, capacity_, lines_, maxChannels (+17 more)

### Community 31 - "ClapLibrary.cpp"
Cohesion: 0.09
Nodes (30): array, clap_event_header_t, clap_event_param_value_t, clap_input_events_t, clap_istream_t, clap_ostream_t, clap_output_events_t, blobRead() (+22 more)

### Community 32 - "SampleRingBuffer"
Cohesion: 0.10
Nodes (19): FrameCount, Sample, SampleRate, size_t, vector, InputMonitorNode, channelCount_, ring_ (+11 more)

### Community 33 - "SampleCache"
Cohesion: 0.08
Nodes (26): int64_t, path, shared_ptr, size_t, string, Entry, mutex, string (+18 more)

### Community 34 - "SimpleSynth"
Cohesion: 0.08
Nodes (22): uint32_t, array, atomic, maxVoices, ParameterSink, Sample, SampleRate, uint64_t (+14 more)

### Community 35 - "2. INCDAW functional scope"
Cohesion: 0.07
Nodes (29): 1.1 Findings that changed INCDAW's architecture, 1.2 Supported plugin formats (official), 1.3 Other FL Studio 2026 features, recorded for completeness, 1. Functional reference: FL Studio 2026, 2. INCDAW functional scope, 3. Non-functional requirements, Audio editor, Audio engine (+21 more)

### Community 36 - "AudioFileData"
Cohesion: 0.14
Nodes (25): applyGain(), applyRamp(), clampedRegion(), Sample, fadeIn(), fadeOut(), FrameCount, normalize() (+17 more)

### Community 37 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 38 - "AudioLogger"
Cohesion: 0.09
Nodes (22): AudioLogger, capacityFrames_, circle_, enabled_, grab, log, prepare, ready_ (+14 more)

### Community 39 - "TestLatencyPlugin.cpp"
Cohesion: 0.11
Nodes (25): clap_host_t, clap_plugin_descriptor_t, clap_plugin_factory_t, clap_plugin_t, clap_process_status, clap_process_t, vector, factoryCreatePlugin() (+17 more)

### Community 40 - "ClapInstance"
Cohesion: 0.08
Nodes (24): clap_plugin_gui_t, clap_plugin_params_t, clap_plugin_state_t, clap_plugin_t, int64_t, ParamEvent, ClapInstance, closeEditor (+16 more)

### Community 41 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 42 - "PluginInstanceManager"
Cohesion: 0.11
Nodes (26): Held, size_t, string, uint32_t, uint64_t, unique_ptr, vector, mutex (+18 more)

### Community 43 - "INCDAWMixerView"
Cohesion: 0.14
Nodes (25): incdaw, NSArray, NSDictionary, NSView, INCDAWMixerView, -acceptsFirstResponder, -addStripRect, -drawRect (+17 more)

### Community 44 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 45 - "RecordingSession"
Cohesion: 0.09
Nodes (20): path, Placement, string, vector, FrameCount, FramePosition, uint32_t, uint64_t (+12 more)

### Community 46 - "EditAssetRegionCommand"
Cohesion: 0.09
Nodes (21): AudioEditOp, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_, minted_ (+13 more)

### Community 47 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 48 - "GraphCompileOptions"
Cohesion: 0.08
Nodes (26): DiskStreamer, FrameCount, InsertFactory, PlaybackSource, SampleRate, GraphCompileOptions, channelCount, diskStreamer (+18 more)

### Community 49 - "AddMidiMappingCommand"
Cohesion: 0.10
Nodes (17): AddMidiMappingCommand, controller_, execute, mapping_, midiChannel_, minted_, parameterKey_, target_ (+9 more)

### Community 50 - "BuiltinEffect"
Cohesion: 0.12
Nodes (17): atomic, LockFreeQueue, Node, PluginParameterInfo, SharedLibrary, BuiltinEffect, values_, ParameterSink (+9 more)

### Community 51 - "vector"
Cohesion: 0.13
Nodes (8): AudioCaptureSink, vector, string, thread_, unordered_map, mutex, allocationSize(), size_t

### Community 52 - "Clip"
Cohesion: 0.08
Nodes (25): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+17 more)

### Community 53 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (24): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+16 more)

### Community 54 - "INCDAW — Roadmap"
Cohesion: 0.08
Nodes (23): Deliberately out of scope, INCDAW — Roadmap, Phase 0 — Research and architecture ✅ COMPLETE, Phase 10 — Mixer and routing, Phase 11 — Automation, Phase 12 — Recording and audio editor, Phase 13 — Plugin hosting, Phase 14 — Sampler (+15 more)

### Community 55 - "Sampler.cpp"
Cohesion: 0.16
Nodes (22): FrameCount, Sample, SampleRate, vector, Voice, interpolate(), activeVoiceCount, allNotesOff (+14 more)

### Community 56 - "MidiEvent"
Cohesion: 0.11
Nodes (23): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+15 more)

### Community 57 - "ConstantNode"
Cohesion: 0.10
Nodes (14): ConstantNode, latency_, value_, FrameCount, Sample, size_t, vector, OrderRecordingNode (+6 more)

### Community 58 - "-applicationDidFinishLaunching"
Cohesion: 0.11
Nodes (22): INCDAWAudioEditorView, INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, INCDAWPlaylistView, NSAlert, NSApplicationDelegate (+14 more)

### Community 59 - "PluginRegistry"
Cohesion: 0.16
Nodes (21): Library, Located, int64_t, path, size_t, string, uint64_t, vector (+13 more)

### Community 60 - "Json"
Cohesion: 0.10
Nodes (14): nullptr_t, int64_t, pair, string, vector, Json, append, boolean_ (+6 more)

### Community 61 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 62 - "MidiMessage"
Cohesion: 0.11
Nodes (9): FrameCount, uint8_t, MidiMessage, data1, data2, frameOffset, status, vector (+1 more)

### Community 63 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 64 - "PluginIdentifier"
Cohesion: 0.12
Nodes (16): builtinSampler(), builtinSimpleSynth(), Format, string, formatName(), Format, friend, string (+8 more)

### Community 65 - "InsertRecordedTakeCommand"
Cohesion: 0.10
Nodes (16): Placement, size_t, string, vector, InsertRecordedTakeCommand, asset_, assetIndex_, clipIndices_ (+8 more)

### Community 66 - "Model.h"
Cohesion: 0.11
Nodes (17): AutomationCurve, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint, curve (+9 more)

### Community 67 - "EqEffect"
Cohesion: 0.11
Nodes (19): Coefficients, FrameCount, SampleRate, EqEffect, bandCount, cached_, coefficients_, maxChannels (+11 more)

### Community 68 - "LoadSampleCommand"
Cohesion: 0.11
Nodes (15): size_t, string, vector, LoadSampleCommand, asset_, assetIndex_, channelId_, created_ (+7 more)

### Community 69 - "read"
Cohesion: 0.17
Nodes (20): path, Result, size_t, uint16_t, uint32_t, uint8_t, vector, fillMetadata() (+12 more)

### Community 70 - "AudioBufferView"
Cohesion: 0.17
Nodes (9): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t, SampleRate (+1 more)

### Community 71 - "CompiledProjectGraph"
Cohesion: 0.10
Nodes (21): CompiledProjectGraph, automation, channels, channelStrips, error, graph, insertSinks, insertSlots (+13 more)

### Community 72 - "PluginInsertTests.cpp"
Cohesion: 0.18
Nodes (13): anyNonZero(), ClipInsert, threshold_, FrameCount, Sample, vector, GainInsert, factor_ (+5 more)

### Community 73 - "PluginStateTests.cpp"
Cohesion: 0.12
Nodes (17): anyNonZero(), compileLoaded(), InsertFactory, path, Sample, uint8_t, vector, factoryFor() (+9 more)

### Community 74 - "SamplerStreamingTests.cpp"
Cohesion: 0.13
Nodes (18): FrameCount, MidiBuffer, path, Sample, shared_ptr, size_t, string, vector (+10 more)

### Community 75 - "compileProjectGraph"
Cohesion: 0.14
Nodes (19): AutomationNode, Channel, channelStripFor, insertSinkFor, insertStateFor, instrumentFor, stripFor, compileProjectGraph() (+11 more)

### Community 76 - "ClapLibrary"
Cohesion: 0.12
Nodes (12): clap_plugin_entry_t, clap_plugin_factory_t, ClapLibrary, close, descriptors, entry_, factory_, library_ (+4 more)

### Community 77 - "CommandRegistry.cpp"
Cohesion: 0.17
Nodes (19): clearHistory, execute, executeMerging, findAction, invoke, redo, redoName, registerAction (+11 more)

### Community 78 - "WaveformOverview"
Cohesion: 0.11
Nodes (17): Bucket, FrameCount, SampleRate, size_t, vector, WaveformOverview, channelCount, channels (+9 more)

### Community 79 - "InstrumentNode"
Cohesion: 0.12
Nodes (15): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, ParameterSink, unique_ptr, InstrumentNode (+7 more)

### Community 80 - "MixerNode"
Cohesion: 0.11
Nodes (19): MixerNodeType, string, uint32_t, MixerNode, colour, id, inserts, muted (+11 more)

### Community 81 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 82 - "ParameterRegistry"
Cohesion: 0.21
Nodes (18): Applier, convertParameters(), Entry, size_t, string, uint32_t, vector, Entry (+10 more)

### Community 83 - "RenderOptions"
Cohesion: 0.11
Nodes (19): BitDepth, FramePosition, SampleRate, uint64_t, RenderOptions, bitDepth, blockSize, dither (+11 more)

### Community 84 - "4. Specialised tests"
Cohesion: 0.11
Nodes (18): 1. Principle, 2. Framework, 3. Test levels, 4. Specialised tests, 5. What is not tested automatically, End-to-end, Fuzzing (from Phase 4), Golden-file audio (from Phase 7) (+10 more)

### Community 85 - "SamplerZoneStream"
Cohesion: 0.14
Nodes (16): Slot, uint64_t, array, FrameCount, shared_ptr, size_t, SamplerZoneStream, claimSlot (+8 more)

### Community 86 - "PlaylistModel.cpp"
Cohesion: 0.20
Nodes (18): Rect, size_t, vector, addToSelection, clipAtPoint, clipRect, clipsInRectangle, collectVisibleClips (+10 more)

### Community 87 - "CompiledGraph"
Cohesion: 0.13
Nodes (13): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+5 more)

### Community 88 - "SamplerZone"
Cohesion: 0.11
Nodes (18): FrameCount, shared_ptr, handleMessage, SamplerZone, end, gain, keyHigh, keyLow (+10 more)

### Community 89 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 90 - "MidiInput"
Cohesion: 0.14
Nodes (13): atomic, queueCapacity, size_t, uint64_t, MidiInput, dropped_, hasPending_, lastControl_ (+5 more)

### Community 91 - "load"
Cohesion: 0.27
Nodes (18): automationPointFrom(), bindUnassignedContent(), path, Result, string, idFrom(), midiEventFrom(), pluginFrom() (+10 more)

### Community 92 - "FuzzTests.cpp"
Cohesion: 0.16
Nodes (13): corrupt(), path, size_t, string, uint64_t, uint8_t, vector, Random (+5 more)

### Community 93 - "write"
Cohesion: 0.19
Nodes (16): int32_t, AiffFile, write, appendBigU16(), appendBigU32(), appendExtended(), appendId(), Format (+8 more)

### Community 94 - "PlaylistModel"
Cohesion: 0.16
Nodes (10): size_t, Tick, vector, Viewport, PlaylistModel, noClip, noTrack, resizeHandleWidth (+2 more)

### Community 95 - "DelayLineNode"
Cohesion: 0.14
Nodes (14): FrameCount, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_, prepare (+6 more)

### Community 96 - "SimpleSynth.cpp"
Cohesion: 0.15
Nodes (17): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), polyBlep(), activeVoiceCount (+9 more)

### Community 97 - "SystemInfo"
Cohesion: 0.14
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 98 - "SamplerTests.cpp"
Cohesion: 0.18
Nodes (17): constantSample(), FrameCount, MidiBuffer, Sample, shared_ptr, vector, makeEnvelopeTransparent(), nyquistSample() (+9 more)

### Community 99 - "TimingProbeInstrument"
Cohesion: 0.14
Nodes (10): Applied, FrameCount, MidiBuffer, Sample, SampleRate, vector, renderSynth(), TimingProbeInstrument (+2 more)

### Community 100 - "ioProcTrampoline"
Cohesion: 0.21
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 101 - "INCDAW — Plugin Host"
Cohesion: 0.12
Nodes (16): 10. Testing, 1. Supported formats, 2. Prime directive, 3. Pipeline, 4. Isolation strategy, 5. Parameter system, 6. State, 7. Editor / UI bridge (+8 more)

### Community 102 - "capturePluginState"
Cohesion: 0.25
Nodes (15): PluginSlot, open, resolveBinary(), captureBuiltinInsertState(), capturePluginState(), Project, string, uint8_t (+7 more)

### Community 103 - "CommandRegistry"
Cohesion: 0.14
Nodes (9): CommandRegistry, actions_, project_, redoStack_, undoStack_, CommandPtr, Entry, size_t (+1 more)

### Community 104 - "AudioClipNode"
Cohesion: 0.13
Nodes (13): AudioClipNode, addClip, clips_, fetchScratch_, prepare, process, FrameCount, PlacedClip (+5 more)

### Community 105 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 106 - "GainNode"
Cohesion: 0.15
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 107 - "SineOscillatorNode"
Cohesion: 0.13
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 108 - "NoteSequence"
Cohesion: 0.17
Nodes (13): Tick, vector, Tick, uint32_t, vector, NoteSequence, byEnd_, clear (+5 more)

### Community 109 - "EditFixture"
Cohesion: 0.15
Nodes (13): FrameCount, path, Sample, size_t, EditFixture, assetId, file, project (+5 more)

### Community 110 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 111 - "BuiltinEffectTests.cpp"
Cohesion: 0.19
Nodes (15): FrameCount, Sample, size_t, vector, processThrough(), RefAllpass, index, line (+7 more)

### Community 112 - "INCDAW — Architecture"
Cohesion: 0.12
Nodes (15): 1. Guiding principle, 2. Layer model, 3. Proposed repository structure, 4. Threading model, 5. Data model, 6. Command architecture, 7. Engine boundary, 8. Plugin isolation (+7 more)

### Community 113 - "CoreMidiDevice"
Cohesion: 0.15
Nodes (14): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+6 more)

### Community 114 - "AutomationWriteSession"
Cohesion: 0.15
Nodes (12): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, string, Tick (+4 more)

### Community 115 - "WriteAutomationCommand"
Cohesion: 0.12
Nodes (15): WriteAutomationCommand, clip_, clipIndex_, key_, laneAfter_, laneCreated_, laneId_, laneIndex_ (+7 more)

### Community 116 - "atomic"
Cohesion: 0.18
Nodes (3): atomic, MidiBuffer, array

### Community 117 - "BuiltinEffect.cpp"
Cohesion: 0.32
Nodes (15): appendF64(), appendU32(), BuiltinEffect::BuiltinEffect(), decodeState, loadState, saveState, setParameter, value (+7 more)

### Community 118 - "EffectParameter"
Cohesion: 0.12
Nodes (14): EffectParameter, defaultValue, id, maxValue, minValue, name, stepped, uint32_t (+6 more)

### Community 119 - "BuiltinEffectInfo"
Cohesion: 0.16
Nodes (14): BuiltinEffectInfo, displayName, parameterCount, parameters, uid, CatalogueEntry, info, make (+6 more)

### Community 120 - "Json.cpp"
Cohesion: 0.20
Nodes (14): int64_t, size_t, string, escapeInto(), formatDouble(), asBool, asDouble, asInt (+6 more)

### Community 121 - "AutomationPoint"
Cohesion: 0.17
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 122 - "INCDAW — Performance Strategy"
Cohesion: 0.13
Nodes (14): 1. Reference machine, 2. Targets, 3. Instrumentation, 4. Method, 5. Known design-level performance decisions, 6. Profiling tooling, 7. Phase 18 — measured baseline and optimisations, Audio (+6 more)

### Community 123 - "write"
Cohesion: 0.29
Nodes (14): assetFilePath(), Sample, string, vector, execute, name, undo, findAsset() (+6 more)

### Community 124 - "AddAutomationLaneCommand"
Cohesion: 0.15
Nodes (8): AddAutomationLaneCommand, execute, index_, key_, lane_, minted_, target_, string

### Community 125 - "AudioEngine.h"
Cohesion: 0.15
Nodes (8): AudioCaptureSink, FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 126 - "AudioBufferPool"
Cohesion: 0.17
Nodes (9): AudioBufferPool, channelPointers_, reset, samples_, FrameCount, Sample, size_t, unique_ptr (+1 more)

### Community 127 - ".buffer"
Cohesion: 0.22
Nodes (12): anyNonZero(), CompiledGraph, pair, ParameterSink, Sample, uint32_t, vector, channel (+4 more)

### Community 128 - "Node"
Cohesion: 0.16
Nodes (6): FrameCount, SampleRate, Node, process, ParameterSink, StateIO

### Community 129 - "Instrument"
Cohesion: 0.14
Nodes (10): MidiBuffer, ParameterSink, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare (+2 more)

### Community 130 - "Parser"
Cohesion: 0.30
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 131 - "renderClickFrames"
Cohesion: 0.15
Nodes (12): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, FramePosition (+4 more)

### Community 132 - "ChannelSamplerZone"
Cohesion: 0.14
Nodes (14): ChannelSamplerZone, asset, end, gain, keyHigh, keyLow, loopCrossfade, loopEnd (+6 more)

### Community 133 - "Options"
Cohesion: 0.14
Nodes (14): int64_t, Options, amplitude, buffer, device, frequency, input, listOnly (+6 more)

### Community 134 - "INCDAW — Audio Engine"
Cohesion: 0.15
Nodes (12): 10. Audio correctness requirements, 11. Performance budget, 1. The prime directive, 2. Device layer, 3. Realtime thread scheduling, 4. Realtime safety enforcement, 5. Signal flow, 6. Block processing and sample-accurate events (+4 more)

### Community 135 - "AutomationCommands.cpp"
Cohesion: 0.19
Nodes (12): undo, AutomationPoint, vector, findLane(), execute, canMergeWith, execute, mergeWith (+4 more)

### Community 136 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (8): size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_, step_

### Community 137 - "Channel"
Cohesion: 0.15
Nodes (13): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+5 more)

### Community 138 - "ChannelRackView.mm"
Cohesion: 0.26
Nodes (12): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+4 more)

### Community 139 - "PluginPersistenceTests.cpp"
Cohesion: 0.17
Nodes (10): path, uint8_t, vector, gainBlob(), Harness, folder, registry, processOnes() (+2 more)

### Community 140 - "RecordingPlacementTests.cpp"
Cohesion: 0.21
Nodes (11): FrameCount, path, Sample, shared_ptr, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 141 - "MidiRecorder"
Cohesion: 0.20
Nodes (10): CapturedMessage, atomic, queueCapacity, size_t, uint64_t, MidiRecorder, captured_, dropped_ (+2 more)

### Community 142 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 143 - "allocate"
Cohesion: 0.20
Nodes (10): allocate, FrameCount, size_t, Sample, renderedPeak(), time_point, vector, main() (+2 more)

### Community 144 - "KernelTable"
Cohesion: 0.21
Nodes (9): SampleRate, vector, KernelTable, phases, weights_, width, resample(), sinc() (+1 more)

### Community 145 - "ScanOutcome"
Cohesion: 0.20
Nodes (11): path, string, string, vector, parseLine(), ScanOutcome, detail, plugins (+3 more)

### Community 146 - "Track"
Cohesion: 0.17
Nodes (12): findTrack, Track, colour, height, id, muted, name, outputMixerNode (+4 more)

### Community 147 - "renderArrangement"
Cohesion: 0.23
Nodes (10): FrameCount, path, Sample, size_t, vector, makeAudio(), renderArrangement(), ScratchDir (+2 more)

### Community 148 - "Fixture"
Cohesion: 0.20
Nodes (10): Tick, vector, Fixture, channel, pattern, project, trackA, trackB (+2 more)

### Community 149 - "INCDAW — Project Format"
Cohesion: 0.18
Nodes (10): 1. Shape: a package directory, not a single file, 2. Versioning and migration, 3. Text vs binary, 4. Media: referenced or embedded, 5. Autosave, backup and recovery, 6. Archiving, 7. Determinism, 8. Tests (Phase 4 gate) (+2 more)

### Community 150 - "ChildResult"
Cohesion: 0.18
Nodes (10): End, ChildResult, code, end, output, path, string, vector (+2 more)

### Community 151 - "AddPatternClipCommand"
Cohesion: 0.18
Nodes (9): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+1 more)

### Community 152 - "AutomationNode"
Cohesion: 0.20
Nodes (6): AutomationNode, bindings_, tempoMap_, Binding, size_t, vector

### Community 153 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 154 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 155 - "SharedLibrary"
Cohesion: 0.25
Nodes (7): path, string, SharedLibrary, close, handle_, open, symbol

### Community 156 - "BlobReader"
Cohesion: 0.22
Nodes (11): BlobReader, cursor, data, BlobWriter, out, overflowed, loadState, saveState (+3 more)

### Community 157 - "PluginNode"
Cohesion: 0.18
Nodes (5): FrameCount, ParameterSink, StateIO, PluginNode, instance_

### Community 158 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 159 - "renderProject"
Cohesion: 0.25
Nodes (7): findChannel, path, uint64_t, DitherSource, state_, renderProject(), renderProjectToFile()

### Community 160 - "INCDAWInsertParameterPanel"
Cohesion: 0.20
Nodes (9): NSObject, INCDAWFlippedView, -isFlipped, INCDAWInsertParameterPanel, +makePanelWithTitlerowsonWrite, -sliderMoved, NSScrollView, NSView (+1 more)

### Community 161 - "main.mm"
Cohesion: 0.29
Nodes (10): -editorChanged, -openAudioAssetInEditor, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll, -showPlaylist, -togglePlayback (+2 more)

### Community 162 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 163 - "ClipCommands.cpp"
Cohesion: 0.27
Nodes (8): execute, undo, execute, canMergeWith, mergeWith, undo, canMergeWith, trackAtOffset()

### Community 164 - "RemoveClipsCommand"
Cohesion: 0.20
Nodes (9): string, RemovedClip, vector, RemoveClipsCommand, clips_, execute, name, removed_ (+1 more)

### Community 165 - "build"
Cohesion: 0.27
Nodes (9): bucketize(), Bucket, FrameCount, path, Result, Sample, vector, sizeBuckets() (+1 more)

### Community 166 - "TimestampedMidiMessage"
Cohesion: 0.22
Nodes (9): midiMessageReceived, sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos (+1 more)

### Community 167 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 168 - "PluginParameterInfo"
Cohesion: 0.20
Nodes (9): string, uint32_t, PluginParameterInfo, defaultValue, id, maxValue, minValue, name (+1 more)

### Community 169 - "RenderResult"
Cohesion: 0.20
Nodes (9): FrameCount, string, vector, RenderResult, arrangementFrames, audio, error, succeeded (+1 more)

### Community 170 - "PluginFolder"
Cohesion: 0.24
Nodes (7): path, PluginFolder, crash, dir, gain, ScratchDir, path

### Community 171 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 172 - "setParameter"
Cohesion: 0.22
Nodes (3): FilterMode, uint32_t, setParameter

### Community 173 - "MoveClipsCommand"
Cohesion: 0.22
Nodes (8): MovedAudioClip, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, movedAudio_, tickDelta_, trackDelta_

### Community 174 - "ProjectGraphCompiler.h"
Cohesion: 0.22
Nodes (7): ParameterRegistry, SampleCache, SampleRingBuffer, InstrumentNode, StateIO, uint32_t, decodedValue()

### Community 175 - "SetAutomationPointsCommand"
Cohesion: 0.31
Nodes (6): AutomationPoint, vector, SetAutomationPointsCommand, laneId_, points_, previous_

### Community 176 - "RemoveAutomationLaneCommand"
Cohesion: 0.22
Nodes (6): size_t, RemoveAutomationLaneCommand, index_, lane_, laneId_, undo

### Community 177 - "DuplicateClipsCommand"
Cohesion: 0.22
Nodes (8): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_, undo

### Community 178 - "ResizeClipsCommand"
Cohesion: 0.22
Nodes (8): FrameCount, ResizeClipsCommand, clips_, lengthDelta_, mergeWith, previousFrameLengths_, previousLengths_, undo

### Community 179 - "MidiMappingTests.cpp"
Cohesion: 0.22
Nodes (5): controlChange(), path, string, ScratchDirectory, path

### Community 180 - "MidiMapNode"
Cohesion: 0.25
Nodes (5): Binding, size_t, vector, MidiMapNode, bindings_

### Community 181 - "exportArrangement"
Cohesion: 0.25
Nodes (7): size_t, notes_, path, Result, uint64_t, exportArrangement(), importAsPattern()

### Community 183 - "MidiMapping"
Cohesion: 0.22
Nodes (8): MidiMapping, controller, id, maxValue, midiChannel, minValue, parameterKey, targetEntity

### Community 184 - "AutomationFixture"
Cohesion: 0.22
Nodes (6): AutomationFixture, channel, pattern, project, tempo, FramePosition

### Community 185 - "ImpulseNode"
Cohesion: 0.25
Nodes (6): FrameCount, Sample, vector, ImpulseNode, at_, render()

### Community 186 - "ParameterFixture"
Cohesion: 0.22
Nodes (7): EntityId, Project, TempoMap, ParameterFixture, pattern, project, tempo

### Community 188 - "Denormals.h"
Cohesion: 0.39
Nodes (5): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister()

### Community 189 - "SetClipMutedCommand"
Cohesion: 0.25
Nodes (6): SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 190 - "updatedRecents"
Cohesion: 0.32
Nodes (7): autosaveIsNewer(), autosavePathFor(), path, size_t, string, vector, updatedRecents()

### Community 191 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 192 - "AnalyzerEffect"
Cohesion: 0.32
Nodes (5): AnalyzerEffect, maxChannels, process, atomic, size_t

### Community 193 - "MidiDeviceInfo"
Cohesion: 0.25
Nodes (6): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback

### Community 194 - "MidiImportResult"
Cohesion: 0.25
Nodes (7): string, vector, MidiImportResult, error, newChannels, pattern, succeeded

### Community 195 - "ProjectMetadata"
Cohesion: 0.25
Nodes (8): ProjectMetadata, artist, comment, created, createdWith, lastSavedWith, modified, title

### Community 196 - "Fixture"
Cohesion: 0.25
Nodes (6): Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 197 - "AutomationProbe"
Cohesion: 0.29
Nodes (6): AutomationProbe, calls, registry, written, FramePosition, vector

### Community 198 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped()

### Community 199 - "ScriptedFactory"
Cohesion: 0.25
Nodes (7): function, InsertFactory, unique_ptr, ScriptedFactory, fail, makers, requests

### Community 200 - "RenderTests.cpp"
Cohesion: 0.25
Nodes (5): path, string, makeArrangedProject(), ScratchDirectory, path

### Community 201 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 202 - "INCDAW — Release Guide"
Cohesion: 0.29
Nodes (6): 1. What a release is, 2. Cutting a release, 3. Installing (first launch on another Mac), 4. Updating, 5. Release notes — 0.9.0 (2026-08-16), INCDAW — Release Guide

### Community 203 - "collectForBlock"
Cohesion: 0.29
Nodes (6): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock, resetCounters

### Community 204 - "SequencedNote"
Cohesion: 0.29
Nodes (6): SequencedNote, channel, key, lengthTicks, startTick, velocity

### Community 205 - "CarriedInsertState"
Cohesion: 0.29
Nodes (7): CarriedInsertState, blob, slot, EntityId, uint8_t, vector, restoreBuiltinInsertState()

### Community 206 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 207 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 208 - "StressTests.cpp"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 209 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 211 - "collectForRange"
Cohesion: 0.33
Nodes (4): FrameCount, FramePosition, MidiBuffer, collectForRange

### Community 212 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 213 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 214 - "v1.2/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 215 - "v1.3/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 216 - "v1.4/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 217 - "renderBlock"
Cohesion: 0.47
Nodes (5): FrameCount, Sample, vector, renderBlock(), tone()

### Community 218 - "PatternTests.cpp"
Cohesion: 0.53
Nodes (5): Tick, vector, note(), shapeOf(), startsOf()

### Community 219 - "InsertFixture"
Cohesion: 0.33
Nodes (4): InsertFixture, pattern, project, tempo

### Community 220 - "SessionFixture"
Cohesion: 0.40
Nodes (4): path, string, SessionFixture, root

### Community 221 - "StateIO"
Cohesion: 0.40
Nodes (3): StateIO, loadState, saveState

### Community 222 - "MidiRecorder.cpp"
Cohesion: 0.40
Nodes (4): FramePosition, MidiBuffer, capture, reset

### Community 223 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 224 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 225 - "AutomationTests.cpp"
Cohesion: 0.60
Nodes (4): AutomationPoint, Tick, enginePoint(), modelPoint()

### Community 226 - "Fixture"
Cohesion: 0.40
Nodes (3): Fixture, project, registry

### Community 227 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 230 - "INCDAWPatternListView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWPatternListView, -initWithFrameprojectregistry

### Community 232 - "ScratchDir"
Cohesion: 0.50
Nodes (3): path, ScratchDir, path

### Community 262 - "EntityId"
Cohesion: 0.09
Nodes (31): NoteIndices, Tick, EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId> (+23 more)

### Community 263 - "arrangementEndFrames"
Cohesion: 0.50
Nodes (5): clipLengthTicks(), clipStartTicks(), Tick, arrangementEndFrames(), FrameCount

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
- **1344 isolated node(s):** `execute`, `id`, `name`, `undo`, `index_` (+1339 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **28 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `Command`, `NoteCommands.cpp`, `ChannelCommands.cpp`, `PatternCommands.cpp`, `AutomationCommands.cpp`, `RemoveTrackCommand`, `EntityId`, `Channel`, `arrangementEndFrames`, `ConnectMixerCommand`, `TempoMap`, `Track`, `AddInsertCommand`, `renderArrangement`, `Fixture`, `Pattern`, `MixerTests.cpp`, `AudioAsset`, `renderProject`, `SampleCache`, `ClipCommands.cpp`, `RemoveClipsCommand`, `RemoveAutomationLaneCommand`, `DuplicateClipsCommand`, `ResizeClipsCommand`, `AddMidiMappingCommand`, `Clip`, `exportArrangement`, `MidiMapping`, `AutomationFixture`, `SetClipMutedCommand`, `InsertRecordedTakeCommand`, `Model.h`, `ProjectMetadata`, `LoadSampleCommand`, `Fixture`, `AutomationProbe`, `RenderTests.cpp`, `PluginStateTests.cpp`, `MixerNode`, `PlaylistModel.cpp`, `load`, `InsertFixture`, `Fixture`, `CommandRegistry`, `EditFixture`, `write`, `AddAutomationLaneCommand`?**
  _High betweenness centrality (0.087) - this node is a cross-community bridge._