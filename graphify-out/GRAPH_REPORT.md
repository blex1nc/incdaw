# Graph Report - onay-devam-6548af  (2026-08-16)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 4568 nodes · 7888 edges · 255 communities (230 shown, 25 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 377 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `b1d1d08f`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- NoteCommands.cpp
- ChannelCommands.cpp
- WavStreamWriter
- WriteAutomationCommand
- PatternCommands.cpp
- TestGainPlugin.cpp
- Track
- MixerStripNode
- AudioEngine
- Command
- PianoRollModel
- Project
- SimpleSynth
- CompiledProjectGraph
- read
- CoreAudioDevice
- [0.9.0] — 2026-08-16 — the core is complete
- ProcessContext
- Pattern
- BuiltinEffect
- EntityId
- MixerCommands.cpp
- PlaylistModel
- AudioStream
- Transport
- Clip
- GraphCompileOptions
- INCDAW — Decision Log
- AddInsertCommand
- Sampler
- GraphBuilder
- WavStreamReader
- ChannelRackModel
- ClapInstance
- EqEffect
- MusicalPosition
- PluginInsertTests.cpp
- 2. INCDAW functional scope
- TempoMap
- AudioFileData
- AddMidiMappingCommand
- AudioLogger
- MidiMessage
- TestLatencyPlugin.cpp
- PluginInstanceManager
- DelayEffect
- Json
- AddMixerNodeCommand
- atomic
- MetronomeNode
- AudioDeviceConfig
- Json.cpp
- RecordingSession
- ClapLibrary.cpp
- CoreAudioDevice.cpp
- INCDAWMixerView
- read
- AudioAsset
- EditAssetRegionCommand
- PlaylistView.mm
- PluginParameterTests.cpp
- AudioRecorder
- INCDAW — Roadmap
- Sampler.cpp
- ConstantNode
- AudioBufferView
- CallbackProfiler
- NoteSequence
- AudioDevice
- PluginRegistry
- RemoveMixerNodeCommand
- InsertRecordedTakeCommand
- ParameterRegistry
- LockFreeQueue
- PluginIdentifier
- MixerTests.cpp
- PluginStateTests.cpp
- SamplerStreamingTests.cpp
- capturePluginState
- LoadSampleCommand
- WaveformOverview
- InstrumentNode
- RealtimeGuard.cpp
- RenderOptions
- 4. Specialised tests
- SamplerZoneStream
- CommandRegistry
- CompiledGraph
- SamplerZone
- MidiInput
- load
- FuzzTests.cpp
- ClapLibrary
- write
- CoreMidiDevice
- SampleRingBuffer
- AudioBufferPool
- DelayLineNode
- CompressorEffect
- BasicMidiBuffer
- SystemInfo
- SamplerTests.cpp
- Options
- TimingProbeInstrument
- ioProcTrampoline
- INCDAW — Plugin Host
- -applicationDidFinishLaunching
- CommandRegistry.cpp
- AudioClipNode
- SampleCache
- SineOscillatorNode
- ClapDescriptor
- EditFixture
- LoopbackResult
- BuiltinEffectTests.cpp
- INCDAW — Architecture
- AutomationWriteSession
- BuiltinEffect.cpp
- GainNode
- AutomationPoint
- INCDAW — Performance Strategy
- Node
- BuiltinEffectInfo
- MidiEvent
- DuplicateClipsCommand
- ConnectMixerCommand
- SamplerWiringTests.cpp
- MidiRecorder
- INCDAW — Audio Engine
- ToggleStepCommand
- RenameTrackCommand
- TrackCommands.cpp
- InputMonitorNode
- .buffer
- renderProject
- ChannelRackView.mm
- main.mm
- ImpulseNode
- RecordingPlacementTests.cpp
- CoreMidiDevice.cpp
- AddPatternClipCommand
- KernelTable
- SequencedNote
- AutomationFixture
- renderArrangement
- Fixture
- RealtimeSafetyTests.cpp
- INCDAW — Project Format
- ChildResult
- RemoveClipsCommand
- captureAudioBlock
- TimelineAnchor
- AutomationNode
- TimeSignatureEvent
- BlockSegment
- MidiDevice
- SharedLibrary
- BlobReader
- PluginNode
- humanizeNoteStarts
- INCDAWInsertParameterPanel
- PatternListView.mm
- vector
- ClipCommands.cpp
- ResizeClipsCommand
- build
- ParsedHeader
- TimestampedMidiMessage
- RecordedEvent
- PluginParameterInfo
- RoutingConnection
- RenderResult
- Harness
- PluginFolder
- make-dmg.sh
- setParameter
- MoveClipsCommand
- SetChannelOutputCommand
- AddNoteCommand
- AddTrackCommand
- RemoveTrackCommand
- MidiMappingTests.cpp
- MidiMapNode
- exportArrangement
- SetClipMutedCommand
- SetMixerMutedCommand
- SetMixerSoloedCommand
- SetTrackMutedCommand
- SetTrackSoloedCommand
- Version
- MidiImportResult
- Fixture
- AutomationProbe
- MidiTests.cpp
- ScriptedFactory
- RenderTests.cpp
- makeTestSignal
- INCDAW — Release Guide
- collectForBlock
- ScratchDirectory
- SessionFixture
- ScratchDirectory
- StressTests.cpp
- emptyOutTryPush
- INCDAWPianoRollView
- string
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- v1.2/Fixture.incdaw/manifest.json
- v1.3/Fixture.incdaw/manifest.json
- v1.4/Fixture.incdaw/manifest.json
- renderBlock
- bench/main.cpp
- StateIO
- MidiDeviceInfo
- INCDAWAudioEditorView
- INCDAWPlaylistView
- Fixture
- check
- AudioCaptureSink
- ParameterSink
- INCDAWChannelRackView
- INCDAWPatternListView
- size_t
- vector
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
- NSObject
- NSScrollView
- NSView
- NSWindow
- path
- AnalyzerEffect
- process

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

## Communities (255 total, 25 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "NoteCommands.cpp"
Cohesion: 0.04
Nodes (60): execute, undo, NoteIndices, size_t, string, vector, DeleteNotesCommand, channel_ (+52 more)

### Community 2 - "ChannelCommands.cpp"
Cohesion: 0.04
Nodes (46): RemovedContent, AddChannelCommand, channel_, execute, index_, minted_, undo, size_t (+38 more)

### Community 3 - "WavStreamWriter"
Cohesion: 0.05
Nodes (59): ofstream, appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample (+51 more)

### Community 4 - "WriteAutomationCommand"
Cohesion: 0.04
Nodes (52): AddAutomationLaneCommand, execute, index_, key_, lane_, minted_, target_, undo (+44 more)

### Community 5 - "PatternCommands.cpp"
Cohesion: 0.04
Nodes (43): AddPatternCommand, execute, index_, minted_, pattern_, undo, DuplicatePatternCommand, execute (+35 more)

### Community 6 - "TestGainPlugin.cpp"
Cohesion: 0.06
Nodes (57): clap_gui_resize_hints_t, clap_id, clap_param_info_t, clap_window_t, applyParamEvents(), clap_host_t, clap_input_events_t, clap_istream_t (+49 more)

### Community 7 - "Track"
Cohesion: 0.04
Nodes (53): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+45 more)

### Community 8 - "MixerStripNode"
Cohesion: 0.05
Nodes (40): atomic, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond, rmsWindowSeconds (+32 more)

### Community 9 - "AudioEngine"
Cohesion: 0.07
Nodes (50): RetiredGraph, AudioCaptureSink, AudioEngine, active_, anchor_, anchorVersion_, audioDeviceAboutToStart, audioDeviceStopped (+42 more)

### Community 10 - "Command"
Cohesion: 0.06
Nodes (15): vector, Command, execute, id, name, undo, string, unordered_map (+7 more)

### Community 11 - "PianoRollModel"
Cohesion: 0.07
Nodes (37): NoteList, size_t, Tick, vector, size_t, Tick, vector, Viewport (+29 more)

### Community 12 - "Project"
Cohesion: 0.06
Nodes (37): execute, undo, execute, undo, Project, audioAssets_, automation_, channels_ (+29 more)

### Community 13 - "SimpleSynth"
Cohesion: 0.06
Nodes (39): FrameCount, SampleRate, size_t, uint32_t, Voice, Waveform, frequencyForKey(), array (+31 more)

### Community 14 - "CompiledProjectGraph"
Cohesion: 0.06
Nodes (42): AutomationNode, Channel, ParameterSink, StateIO, CompiledProjectGraph, automation, channels, channelStripFor (+34 more)

### Community 15 - "read"
Cohesion: 0.08
Nodes (39): appendBigU16(), appendBigU32(), appendChunk(), appendVlq(), path, Result, size_t, Tick (+31 more)

### Community 16 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 17 - "[0.9.0] — 2026-08-16 — the core is complete"
Cohesion: 0.05
Nodes (43): [0.9.0] — 2026-08-16 — the core is complete, INCDAW — Changelog, Phase 0 — Research and architecture — 2026-08-14, Phase 10 — Mixer, routing and delay compensation — 2026-08-14, Phase 11a — Automation: the generic subsystem — 2026-08-15, Phase 11b — Automation placement and recording — 2026-08-15, Phase 12 (part 1) — WAV codec — 2026-08-15, Phase 12 (part 2) — Input capture and recording — 2026-08-15 (+35 more)

### Community 18 - "ProcessContext"
Cohesion: 0.08
Nodes (22): ProcessContext, size_t, sumInputsInto(), process, process, process, updateCoefficients, process (+14 more)

### Community 19 - "Pattern"
Cohesion: 0.09
Nodes (38): Emit, size_t, Tick, vector, noteAtStep(), execute, undo, vector (+30 more)

### Community 20 - "BuiltinEffect"
Cohesion: 0.07
Nodes (30): atomic, Node, ParameterRegistry, SampleCache, SampleRingBuffer, BuiltinEffect, values_, EffectParameter (+22 more)

### Community 21 - "EntityId"
Cohesion: 0.11
Nodes (29): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, size_t, vector (+21 more)

### Community 22 - "MixerCommands.cpp"
Cohesion: 0.07
Nodes (29): SetMixerPanCommand, canMergeWith, execute, mergeWith, nodeId_, pan_, previous_, undo (+21 more)

### Community 23 - "PlaylistModel"
Cohesion: 0.10
Nodes (30): Rect, size_t, Tick, vector, size_t, Tick, vector, Viewport (+22 more)

### Community 24 - "AudioStream"
Cohesion: 0.08
Nodes (33): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+25 more)

### Community 25 - "Transport"
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, uint32_t (+17 more)

### Community 26 - "Clip"
Cohesion: 0.06
Nodes (35): AutomationCurve, ClipType, AutomationPoint, curve, tension, tick, value, Clip (+27 more)

### Community 27 - "GraphCompileOptions"
Cohesion: 0.08
Nodes (26): DiskStreamer, FrameCount, InsertFactory, PlaybackSource, SampleRate, GraphCompileOptions, channelCount, diskStreamer (+18 more)

### Community 28 - "INCDAW — Decision Log"
Cohesion: 0.06
Nodes (34): D-001 — Core implementation language: C++20, D-002 — Build system: CMake + Ninja, D-003 — Audio I/O: CoreAudio HAL directly, no wrapper framework, D-004 — Realtime thread scheduling: os_workgroup / Audio Workgroups, D-005 — Platform strategy: macOS first, Windows later, Linux not precluded, D-006 — UI: AppKit shell + INCDAW-owned Metal-rendered widget layer, D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded), D-008 — Licensing: INCDAW is closed-source (+26 more)

### Community 29 - "AddInsertCommand"
Cohesion: 0.08
Nodes (24): AddInsertCommand, execute, minted_, mixerNode_, plugin_, slot_, undo, findNode() (+16 more)

### Community 30 - "Sampler"
Cohesion: 0.06
Nodes (27): array, atomic, maxVoices, ParameterSink, SampleRate, size_t, uint64_t, vector (+19 more)

### Community 31 - "GraphBuilder"
Cohesion: 0.09
Nodes (28): Connection, NodeIndex, SampleRate, size_t, unique_ptr, GraphBuilder, addNode, analyse (+20 more)

### Community 32 - "WavStreamReader"
Cohesion: 0.08
Nodes (29): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+21 more)

### Community 33 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 34 - "ClapInstance"
Cohesion: 0.08
Nodes (25): clap_plugin_gui_t, clap_plugin_params_t, clap_plugin_state_t, clap_plugin_t, int64_t, ParamEvent, ClapInstance, closeEditor (+17 more)

### Community 35 - "EqEffect"
Cohesion: 0.11
Nodes (19): Coefficients, FrameCount, SampleRate, EqEffect, bandCount, cached_, coefficients_, maxChannels (+11 more)

### Community 36 - "MusicalPosition"
Cohesion: 0.10
Nodes (23): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+15 more)

### Community 37 - "PluginInsertTests.cpp"
Cohesion: 0.11
Nodes (20): anyNonZero(), ClipInsert, threshold_, FrameCount, path, Sample, vector, GainInsert (+12 more)

### Community 38 - "2. INCDAW functional scope"
Cohesion: 0.07
Nodes (29): 1.1 Findings that changed INCDAW's architecture, 1.2 Supported plugin formats (official), 1.3 Other FL Studio 2026 features, recorded for completeness, 1. Functional reference: FL Studio 2026, 2. INCDAW functional scope, 3. Non-functional requirements, Audio editor, Audio engine (+21 more)

### Community 39 - "TempoMap"
Cohesion: 0.12
Nodes (26): execute, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 40 - "AudioFileData"
Cohesion: 0.15
Nodes (25): applyGain(), applyRamp(), clampedRegion(), Sample, fadeIn(), fadeOut(), FrameCount, normalize() (+17 more)

### Community 41 - "AddMidiMappingCommand"
Cohesion: 0.08
Nodes (21): AddMidiMappingCommand, controller_, mapping_, midiChannel_, minted_, parameterKey_, target_, size_t (+13 more)

### Community 42 - "AudioLogger"
Cohesion: 0.09
Nodes (22): AudioLogger, capacityFrames_, circle_, enabled_, grab, log, prepare, ready_ (+14 more)

### Community 43 - "MidiMessage"
Cohesion: 0.09
Nodes (13): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+5 more)

### Community 44 - "TestLatencyPlugin.cpp"
Cohesion: 0.11
Nodes (25): clap_host_t, clap_plugin_descriptor_t, clap_plugin_factory_t, clap_plugin_t, clap_process_status, clap_process_t, vector, factoryCreatePlugin() (+17 more)

### Community 45 - "PluginInstanceManager"
Cohesion: 0.11
Nodes (26): Held, size_t, string, uint32_t, uint64_t, unique_ptr, vector, mutex (+18 more)

### Community 46 - "DelayEffect"
Cohesion: 0.08
Nodes (24): Allpass, Comb, FrameCount, SampleRate, DelayEffect, capacity_, lines_, maxChannels (+16 more)

### Community 47 - "Json"
Cohesion: 0.08
Nodes (18): nullptr_t, int64_t, int64_t, pair, string, vector, Json, append (+10 more)

### Community 48 - "AddMixerNodeCommand"
Cohesion: 0.09
Nodes (14): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType (+6 more)

### Community 49 - "atomic"
Cohesion: 0.10
Nodes (12): atomic, array, MidiBuffer, ParameterSink, Instrument, activeVoiceCount, allNotesOff, handleMessage (+4 more)

### Community 50 - "MetronomeNode"
Cohesion: 0.08
Nodes (20): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+12 more)

### Community 51 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 52 - "Json.cpp"
Cohesion: 0.19
Nodes (22): size_t, string, escapeInto(), formatDouble(), asString, contains, dump, dumpTo (+14 more)

### Community 53 - "RecordingSession"
Cohesion: 0.09
Nodes (20): path, Placement, string, vector, FrameCount, FramePosition, uint32_t, uint64_t (+12 more)

### Community 54 - "ClapLibrary.cpp"
Cohesion: 0.12
Nodes (23): array, clap_event_param_value_t, clap_istream_t, blobRead(), hasEditor, openEditor, readParameter, setParameter (+15 more)

### Community 55 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 56 - "INCDAWMixerView"
Cohesion: 0.15
Nodes (25): incdaw, NSArray, NSDictionary, NSView, INCDAWMixerView, -acceptsFirstResponder, -addStripRect, -drawRect (+17 more)

### Community 57 - "read"
Cohesion: 0.25
Nodes (17): assetFilePath(), Sample, string, vector, execute, name, undo, findAsset() (+9 more)

### Community 58 - "AudioAsset"
Cohesion: 0.08
Nodes (25): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+17 more)

### Community 59 - "EditAssetRegionCommand"
Cohesion: 0.09
Nodes (21): AudioEditOp, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_, minted_ (+13 more)

### Community 60 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (24): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+16 more)

### Community 61 - "PluginParameterTests.cpp"
Cohesion: 0.11
Nodes (20): anyNonZero(), CompiledGraph, EntityId, pair, ParameterSink, Project, Sample, TempoMap (+12 more)

### Community 62 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

### Community 63 - "INCDAW — Roadmap"
Cohesion: 0.08
Nodes (23): Deliberately out of scope, INCDAW — Roadmap, Phase 0 — Research and architecture ✅ COMPLETE, Phase 10 — Mixer and routing, Phase 11 — Automation, Phase 12 — Recording and audio editor, Phase 13 — Plugin hosting, Phase 14 — Sampler (+15 more)

### Community 64 - "Sampler.cpp"
Cohesion: 0.16
Nodes (22): FrameCount, Sample, SampleRate, vector, Voice, interpolate(), activeVoiceCount, allNotesOff (+14 more)

### Community 65 - "ConstantNode"
Cohesion: 0.10
Nodes (14): ConstantNode, latency_, value_, FrameCount, Sample, size_t, vector, OrderRecordingNode (+6 more)

### Community 66 - "AudioBufferView"
Cohesion: 0.16
Nodes (11): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t, process (+3 more)

### Community 67 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 68 - "NoteSequence"
Cohesion: 0.12
Nodes (17): FrameCount, FramePosition, MidiBuffer, Tick, vector, Tick, uint32_t, vector (+9 more)

### Community 69 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 70 - "PluginRegistry"
Cohesion: 0.17
Nodes (21): Library, Located, int64_t, path, size_t, string, uint64_t, vector (+13 more)

### Community 71 - "RemoveMixerNodeCommand"
Cohesion: 0.09
Nodes (17): RemovedRouting, DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t (+9 more)

### Community 72 - "InsertRecordedTakeCommand"
Cohesion: 0.10
Nodes (16): Placement, size_t, string, vector, InsertRecordedTakeCommand, asset_, assetIndex_, clipIndices_ (+8 more)

### Community 73 - "ParameterRegistry"
Cohesion: 0.18
Nodes (19): Applier, convertParameters(), Entry, size_t, string, uint32_t, vector, Entry (+11 more)

### Community 74 - "LockFreeQueue"
Cohesion: 0.12
Nodes (12): MidiBuffer, array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize (+4 more)

### Community 75 - "PluginIdentifier"
Cohesion: 0.13
Nodes (15): builtinSampler(), builtinSimpleSynth(), Format, string, formatName(), Format, friend, string (+7 more)

### Community 76 - "MixerTests.cpp"
Cohesion: 0.14
Nodes (12): FrameCount, Sample, SampleRate, size_t, vector, LatentProcessorNode, delay_, latency_ (+4 more)

### Community 77 - "PluginStateTests.cpp"
Cohesion: 0.12
Nodes (17): anyNonZero(), compileLoaded(), InsertFactory, path, Sample, uint8_t, vector, factoryFor() (+9 more)

### Community 78 - "SamplerStreamingTests.cpp"
Cohesion: 0.13
Nodes (18): FrameCount, MidiBuffer, path, Sample, shared_ptr, size_t, string, vector (+10 more)

### Community 79 - "capturePluginState"
Cohesion: 0.20
Nodes (19): PluginSlot, captureBuiltinInsertState(), capturePluginState(), CarriedInsertState, blob, slot, Project, string (+11 more)

### Community 80 - "LoadSampleCommand"
Cohesion: 0.12
Nodes (15): size_t, string, vector, LoadSampleCommand, asset_, assetIndex_, channelId_, created_ (+7 more)

### Community 81 - "WaveformOverview"
Cohesion: 0.11
Nodes (17): Bucket, FrameCount, SampleRate, size_t, vector, WaveformOverview, channelCount, channels (+9 more)

### Community 82 - "InstrumentNode"
Cohesion: 0.12
Nodes (15): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, ParameterSink, unique_ptr, InstrumentNode (+7 more)

### Community 83 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 84 - "RenderOptions"
Cohesion: 0.11
Nodes (19): BitDepth, FramePosition, SampleRate, uint64_t, RenderOptions, bitDepth, blockSize, dither (+11 more)

### Community 85 - "4. Specialised tests"
Cohesion: 0.11
Nodes (18): 1. Principle, 2. Framework, 3. Test levels, 4. Specialised tests, 5. What is not tested automatically, End-to-end, Fuzzing (from Phase 4), Golden-file audio (from Phase 7) (+10 more)

### Community 86 - "SamplerZoneStream"
Cohesion: 0.14
Nodes (16): Slot, uint64_t, array, FrameCount, shared_ptr, size_t, SamplerZoneStream, claimSlot (+8 more)

### Community 87 - "CommandRegistry"
Cohesion: 0.12
Nodes (12): CommandRegistry, actions_, clearHistory, project_, redo, redoStack_, undo, undoStack_ (+4 more)

### Community 88 - "CompiledGraph"
Cohesion: 0.13
Nodes (13): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+5 more)

### Community 89 - "SamplerZone"
Cohesion: 0.11
Nodes (18): FrameCount, shared_ptr, handleMessage, SamplerZone, end, gain, keyHigh, keyLow (+10 more)

### Community 90 - "MidiInput"
Cohesion: 0.14
Nodes (13): atomic, queueCapacity, size_t, uint64_t, MidiInput, dropped_, hasPending_, lastControl_ (+5 more)

### Community 91 - "load"
Cohesion: 0.27
Nodes (18): automationPointFrom(), bindUnassignedContent(), path, Result, string, idFrom(), midiEventFrom(), pluginFrom() (+10 more)

### Community 92 - "FuzzTests.cpp"
Cohesion: 0.16
Nodes (13): corrupt(), path, size_t, string, uint64_t, uint8_t, vector, Random (+5 more)

### Community 93 - "ClapLibrary"
Cohesion: 0.14
Nodes (11): clap_plugin_entry_t, clap_plugin_factory_t, LockFreeQueue, PluginParameterInfo, SharedLibrary, ClapLibrary, descriptors, entry_ (+3 more)

### Community 94 - "write"
Cohesion: 0.19
Nodes (16): int32_t, AiffFile, write, appendBigU16(), appendBigU32(), appendExtended(), appendId(), Format (+8 more)

### Community 95 - "CoreMidiDevice"
Cohesion: 0.14
Nodes (15): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+7 more)

### Community 96 - "SampleRingBuffer"
Cohesion: 0.19
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 97 - "AudioBufferPool"
Cohesion: 0.14
Nodes (11): AudioBufferPool, channelPointers_, reset, samples_, FrameCount, Sample, size_t, unique_ptr (+3 more)

### Community 98 - "DelayLineNode"
Cohesion: 0.14
Nodes (14): FrameCount, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_, prepare (+6 more)

### Community 99 - "CompressorEffect"
Cohesion: 0.09
Nodes (25): dbToGain(), coefficientFor(), CompressorEffect, envelope_, prepare, process, reduction_, sampleRate_ (+17 more)

### Community 100 - "BasicMidiBuffer"
Cohesion: 0.13
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 101 - "SystemInfo"
Cohesion: 0.14
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 102 - "SamplerTests.cpp"
Cohesion: 0.18
Nodes (17): constantSample(), FrameCount, MidiBuffer, Sample, shared_ptr, vector, makeEnvelopeTransparent(), nyquistSample() (+9 more)

### Community 103 - "Options"
Cohesion: 0.12
Nodes (17): int64_t, string, Options, amplitude, buffer, device, frequency, input (+9 more)

### Community 104 - "TimingProbeInstrument"
Cohesion: 0.14
Nodes (10): Applied, FrameCount, MidiBuffer, Sample, SampleRate, vector, renderSynth(), TimingProbeInstrument (+2 more)

### Community 105 - "ioProcTrampoline"
Cohesion: 0.21
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 106 - "INCDAW — Plugin Host"
Cohesion: 0.12
Nodes (16): 10. Testing, 1. Supported formats, 2. Prime directive, 3. Pipeline, 4. Isolation strategy, 5. Parameter system, 6. State, 7. Editor / UI bridge (+8 more)

### Community 107 - "-applicationDidFinishLaunching"
Cohesion: 0.16
Nodes (17): INCDAWAudioEditorView, INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, INCDAWPlaylistView, NSAlert, NSApplicationDelegate (+9 more)

### Community 108 - "CommandRegistry.cpp"
Cohesion: 0.21
Nodes (16): execute, executeMerging, findAction, invoke, redoName, registerAction, search, setMaximumDepth (+8 more)

### Community 109 - "AudioClipNode"
Cohesion: 0.13
Nodes (13): AudioClipNode, addClip, clips_, fetchScratch_, prepare, process, FrameCount, PlacedClip (+5 more)

### Community 110 - "SampleCache"
Cohesion: 0.15
Nodes (16): int64_t, path, shared_ptr, size_t, string, Entry, mutex, string (+8 more)

### Community 111 - "SineOscillatorNode"
Cohesion: 0.13
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 112 - "ClapDescriptor"
Cohesion: 0.14
Nodes (16): ClapDescriptor, id, name, vendor, version, path, string, string (+8 more)

### Community 113 - "EditFixture"
Cohesion: 0.15
Nodes (13): FrameCount, path, Sample, size_t, EditFixture, assetId, file, project (+5 more)

### Community 114 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 115 - "BuiltinEffectTests.cpp"
Cohesion: 0.19
Nodes (15): FrameCount, Sample, size_t, vector, processThrough(), RefAllpass, index, line (+7 more)

### Community 116 - "INCDAW — Architecture"
Cohesion: 0.12
Nodes (15): 1. Guiding principle, 2. Layer model, 3. Proposed repository structure, 4. Threading model, 5. Data model, 6. Command architecture, 7. Engine boundary, 8. Plugin isolation (+7 more)

### Community 117 - "AutomationWriteSession"
Cohesion: 0.15
Nodes (12): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, string, Tick (+4 more)

### Community 118 - "BuiltinEffect.cpp"
Cohesion: 0.32
Nodes (15): appendF64(), appendU32(), BuiltinEffect::BuiltinEffect(), decodeState, loadState, saveState, setParameter, value (+7 more)

### Community 119 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 120 - "AutomationPoint"
Cohesion: 0.17
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 121 - "INCDAW — Performance Strategy"
Cohesion: 0.13
Nodes (14): 1. Reference machine, 2. Targets, 3. Instrumentation, 4. Method, 5. Known design-level performance decisions, 6. Profiling tooling, 7. Phase 18 — measured baseline and optimisations, Audio (+6 more)

### Community 122 - "Node"
Cohesion: 0.15
Nodes (7): FrameCount, SampleRate, Node, process, FrameCount, ImpulseNode, at_

### Community 123 - "BuiltinEffectInfo"
Cohesion: 0.17
Nodes (14): BuiltinEffectInfo, displayName, parameterCount, parameters, uid, CatalogueEntry, info, make (+6 more)

### Community 124 - "MidiEvent"
Cohesion: 0.13
Nodes (15): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+7 more)

### Community 125 - "DuplicateClipsCommand"
Cohesion: 0.19
Nodes (10): ClipIds, DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_ (+2 more)

### Community 126 - "ConnectMixerCommand"
Cohesion: 0.14
Nodes (11): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+3 more)

### Community 127 - "SamplerWiringTests.cpp"
Cohesion: 0.19
Nodes (10): path, string, noteAtZero(), SamplerProject, asset, channel, project, ScratchDirectory (+2 more)

### Community 128 - "MidiRecorder"
Cohesion: 0.18
Nodes (11): CapturedMessage, atomic, queueCapacity, size_t, uint64_t, MidiRecorder, captured_, dropped_ (+3 more)

### Community 129 - "INCDAW — Audio Engine"
Cohesion: 0.15
Nodes (12): 10. Audio correctness requirements, 11. Performance budget, 1. The prime directive, 2. Device layer, 3. Realtime thread scheduling, 4. Realtime safety enforcement, 5. Signal flow, 6. Block processing and sample-accurate events (+4 more)

### Community 130 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (8): size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_, step_

### Community 131 - "RenameTrackCommand"
Cohesion: 0.18
Nodes (6): string, RenameTrackCommand, execute, previousName_, trackId_, undo

### Community 132 - "TrackCommands.cpp"
Cohesion: 0.19
Nodes (10): execute, undo, SetTrackHeightCommand, canMergeWith, execute, height_, mergeWith, previousHeight_ (+2 more)

### Community 133 - "InputMonitorNode"
Cohesion: 0.17
Nodes (9): FrameCount, Sample, SampleRate, size_t, vector, InputMonitorNode, channelCount_, ring_ (+1 more)

### Community 134 - ".buffer"
Cohesion: 0.19
Nodes (9): allocate, FrameCount, size_t, FramePosition, Sample, vector, render(), Sample (+1 more)

### Community 135 - "renderProject"
Cohesion: 0.22
Nodes (9): findChannel, arrangementEndFrames(), FrameCount, path, uint64_t, DitherSource, state_, renderProject() (+1 more)

### Community 136 - "ChannelRackView.mm"
Cohesion: 0.26
Nodes (12): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+4 more)

### Community 137 - "main.mm"
Cohesion: 0.23
Nodes (12): -editorChanged, -openAudioAssetInEditor, -selectChannel, -selectPattern, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll (+4 more)

### Community 138 - "ImpulseNode"
Cohesion: 0.17
Nodes (9): FramePosition, ImpulseNode, latency_, position_, MixerFixture, channel, pattern, project (+1 more)

### Community 139 - "RecordingPlacementTests.cpp"
Cohesion: 0.21
Nodes (11): FrameCount, path, Sample, shared_ptr, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 140 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 141 - "AddPatternClipCommand"
Cohesion: 0.17
Nodes (9): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+1 more)

### Community 142 - "KernelTable"
Cohesion: 0.21
Nodes (9): SampleRate, vector, KernelTable, phases, weights_, width, resample(), sinc() (+1 more)

### Community 143 - "SequencedNote"
Cohesion: 0.23
Nodes (11): SequencedNote, channel, key, lengthTicks, startTick, velocity, Tick, vector (+3 more)

### Community 144 - "AutomationFixture"
Cohesion: 0.20
Nodes (9): AutomationFixture, channel, pattern, project, tempo, AutomationPoint, Tick, enginePoint() (+1 more)

### Community 145 - "renderArrangement"
Cohesion: 0.23
Nodes (10): FrameCount, path, Sample, size_t, vector, makeAudio(), renderArrangement(), ScratchDir (+2 more)

### Community 146 - "Fixture"
Cohesion: 0.20
Nodes (10): Tick, vector, Fixture, channel, pattern, project, trackA, trackB (+2 more)

### Community 147 - "RealtimeSafetyTests.cpp"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 148 - "INCDAW — Project Format"
Cohesion: 0.18
Nodes (10): 1. Shape: a package directory, not a single file, 2. Versioning and migration, 3. Text vs binary, 4. Media: referenced or embedded, 5. Autosave, backup and recovery, 6. Archiving, 7. Determinism, 8. Tests (Phase 4 gate) (+2 more)

### Community 149 - "ChildResult"
Cohesion: 0.18
Nodes (10): End, ChildResult, code, end, output, path, string, vector (+2 more)

### Community 150 - "RemoveClipsCommand"
Cohesion: 0.18
Nodes (9): string, RemovedClip, vector, RemoveClipsCommand, clips_, execute, name, removed_ (+1 more)

### Community 151 - "captureAudioBlock"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 152 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 153 - "AutomationNode"
Cohesion: 0.20
Nodes (6): AutomationNode, bindings_, tempoMap_, Binding, size_t, vector

### Community 154 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 155 - "BlockSegment"
Cohesion: 0.18
Nodes (9): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, size_t (+1 more)

### Community 156 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 157 - "SharedLibrary"
Cohesion: 0.25
Nodes (7): path, string, SharedLibrary, close, handle_, open, symbol

### Community 158 - "BlobReader"
Cohesion: 0.22
Nodes (11): BlobReader, cursor, data, BlobWriter, out, overflowed, loadState, saveState (+3 more)

### Community 159 - "PluginNode"
Cohesion: 0.18
Nodes (5): FrameCount, ParameterSink, StateIO, PluginNode, instance_

### Community 160 - "humanizeNoteStarts"
Cohesion: 0.29
Nodes (10): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+2 more)

### Community 161 - "INCDAWInsertParameterPanel"
Cohesion: 0.20
Nodes (9): NSObject, INCDAWFlippedView, -isFlipped, INCDAWInsertParameterPanel, +makePanelWithTitlerowsonWrite, -sliderMoved, NSScrollView, NSView (+1 more)

### Community 162 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 163 - "vector"
Cohesion: 0.29
Nodes (8): size_t, autosaveIsNewer(), autosavePathFor(), path, string, exportFileName(), updatedRecents(), vector

### Community 164 - "ClipCommands.cpp"
Cohesion: 0.31
Nodes (8): execute, undo, execute, canMergeWith, execute, mergeWith, undo, trackAtOffset()

### Community 165 - "ResizeClipsCommand"
Cohesion: 0.20
Nodes (9): FrameCount, ResizeClipsCommand, canMergeWith, clips_, lengthDelta_, mergeWith, previousFrameLengths_, previousLengths_ (+1 more)

### Community 166 - "build"
Cohesion: 0.27
Nodes (9): bucketize(), Bucket, FrameCount, path, Result, Sample, vector, sizeBuckets() (+1 more)

### Community 167 - "ParsedHeader"
Cohesion: 0.18
Nodes (17): Result, size_t, uint16_t, uint32_t, uint8_t, vector, fillMetadata(), loadAndParse() (+9 more)

### Community 168 - "TimestampedMidiMessage"
Cohesion: 0.22
Nodes (9): midiMessageReceived, sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos (+1 more)

### Community 169 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 170 - "PluginParameterInfo"
Cohesion: 0.20
Nodes (9): string, uint32_t, PluginParameterInfo, defaultValue, id, maxValue, minValue, name (+1 more)

### Community 171 - "RoutingConnection"
Cohesion: 0.20
Nodes (9): findRouting, RoutingConnection, destination, gain, id, isSend, preFader, sidechain (+1 more)

### Community 172 - "RenderResult"
Cohesion: 0.20
Nodes (9): FrameCount, string, vector, RenderResult, arrangementFrames, audio, error, succeeded (+1 more)

### Community 173 - "Harness"
Cohesion: 0.20
Nodes (7): path, string, Harness, folder, registry, ScratchDir, path

### Community 174 - "PluginFolder"
Cohesion: 0.24
Nodes (7): path, PluginFolder, crash, dir, gain, ScratchDir, path

### Community 175 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 176 - "setParameter"
Cohesion: 0.22
Nodes (3): FilterMode, uint32_t, setParameter

### Community 177 - "MoveClipsCommand"
Cohesion: 0.22
Nodes (8): MovedAudioClip, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, movedAudio_, tickDelta_, trackDelta_

### Community 178 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 179 - "AddNoteCommand"
Cohesion: 0.25
Nodes (6): AddNoteCommand, channel_, index_, note_, pattern_, size_t

### Community 180 - "AddTrackCommand"
Cohesion: 0.22
Nodes (7): AddTrackCommand, execute, index_, minted_, track_, undo, size_t

### Community 181 - "RemoveTrackCommand"
Cohesion: 0.22
Nodes (7): RemovedClip, vector, RemoveTrackCommand, clips_, index_, track_, trackId_

### Community 182 - "MidiMappingTests.cpp"
Cohesion: 0.22
Nodes (5): controlChange(), path, string, ScratchDirectory, path

### Community 183 - "MidiMapNode"
Cohesion: 0.25
Nodes (5): Binding, size_t, vector, MidiMapNode, bindings_

### Community 184 - "exportArrangement"
Cohesion: 0.25
Nodes (7): size_t, notes_, path, Result, uint64_t, exportArrangement(), importAsPattern()

### Community 185 - "SetClipMutedCommand"
Cohesion: 0.25
Nodes (6): SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 186 - "SetMixerMutedCommand"
Cohesion: 0.25
Nodes (5): SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 187 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 188 - "SetTrackMutedCommand"
Cohesion: 0.25
Nodes (5): SetTrackMutedCommand, execute, muted_, trackId_, undo

### Community 189 - "SetTrackSoloedCommand"
Cohesion: 0.25
Nodes (5): SetTrackSoloedCommand, execute, soloed_, trackId_, undo

### Community 190 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 191 - "MidiImportResult"
Cohesion: 0.25
Nodes (7): string, vector, MidiImportResult, error, newChannels, pattern, succeeded

### Community 192 - "Fixture"
Cohesion: 0.25
Nodes (6): Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 193 - "AutomationProbe"
Cohesion: 0.29
Nodes (6): AutomationProbe, calls, registry, written, FramePosition, vector

### Community 194 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped()

### Community 195 - "ScriptedFactory"
Cohesion: 0.25
Nodes (7): function, InsertFactory, unique_ptr, ScriptedFactory, fail, makers, requests

### Community 196 - "RenderTests.cpp"
Cohesion: 0.25
Nodes (5): path, string, makeArrangedProject(), ScratchDirectory, path

### Community 197 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 198 - "INCDAW — Release Guide"
Cohesion: 0.29
Nodes (6): 1. What a release is, 2. Cutting a release, 3. Installing (first launch on another Mac), 4. Updating, 5. Release notes — 0.9.0 (2026-08-16), INCDAW — Release Guide

### Community 199 - "collectForBlock"
Cohesion: 0.29
Nodes (6): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock, resetCounters

### Community 200 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 201 - "SessionFixture"
Cohesion: 0.38
Nodes (4): path, string, SessionFixture, root

### Community 202 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 203 - "StressTests.cpp"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 204 - "emptyOutTryPush"
Cohesion: 0.33
Nodes (6): clap_event_header_t, clap_input_events_t, clap_output_events_t, emptyOutTryPush(), pendingInGet(), pendingInSize()

### Community 205 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 207 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 208 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 209 - "v1.2/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 210 - "v1.3/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 211 - "v1.4/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 212 - "renderBlock"
Cohesion: 0.47
Nodes (5): FrameCount, Sample, vector, renderBlock(), tone()

### Community 213 - "bench/main.cpp"
Cohesion: 0.47
Nodes (5): time_point, vector, main(), median(), millisecondsSince()

### Community 214 - "StateIO"
Cohesion: 0.40
Nodes (3): StateIO, loadState, saveState

### Community 215 - "MidiDeviceInfo"
Cohesion: 0.40
Nodes (5): string, MidiDeviceInfo, identifier, isInput, name

### Community 216 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 217 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 218 - "Fixture"
Cohesion: 0.40
Nodes (3): Fixture, project, registry

### Community 219 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 222 - "INCDAWChannelRackView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWChannelRackView, -initWithFrameprojectregistry

### Community 223 - "INCDAWPatternListView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWPatternListView, -initWithFrameprojectregistry

### Community 253 - "AnalyzerEffect"
Cohesion: 0.32
Nodes (5): AnalyzerEffect, maxChannels, process, atomic, size_t

### Community 254 - "process"
Cohesion: 0.50
Nodes (4): clap_ostream_t, size, blobWrite(), process

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
- **1344 isolated node(s):** `channel_`, `indices_`, `pattern_`, `removed_`, `appliedKeyDelta_` (+1339 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **25 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `NoteCommands.cpp`, `ChannelCommands.cpp`, `RenameTrackCommand`, `WriteAutomationCommand`, `PatternCommands.cpp`, `TrackCommands.cpp`, `Track`, `renderProject`, `Command`, `ImpulseNode`, `AutomationFixture`, `renderArrangement`, `Fixture`, `Pattern`, `EntityId`, `RemoveClipsCommand`, `MixerCommands.cpp`, `PlaylistModel`, `Clip`, `AddInsertCommand`, `ClipCommands.cpp`, `ResizeClipsCommand`, `PluginInsertTests.cpp`, `TempoMap`, `AddMidiMappingCommand`, `RoutingConnection`, `AddMixerNodeCommand`, `SetChannelOutputCommand`, `AddTrackCommand`, `exportArrangement`, `SetClipMutedCommand`, `read`, `SetMixerMutedCommand`, `SetMixerSoloedCommand`, `SetTrackMutedCommand`, `SetTrackSoloedCommand`, `AudioAsset`, `Fixture`, `AutomationProbe`, `RenderTests.cpp`, `RemoveMixerNodeCommand`, `InsertRecordedTakeCommand`, `PluginStateTests.cpp`, `LoadSampleCommand`, `CommandRegistry`, `Fixture`, `load`, `EditFixture`, `DuplicateClipsCommand`, `ConnectMixerCommand`, `SamplerWiringTests.cpp`?**
  _High betweenness centrality (0.124) - this node is a cross-community bridge._