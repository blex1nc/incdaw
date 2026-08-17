# Graph Report - fl-garageband-ui-design-99cf4d  (2026-08-17)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 4496 nodes · 7873 edges · 233 communities (224 shown, 9 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 382 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `c47506db`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Command
- NoteCommands.cpp
- INCDAW
- ChannelCommands.cpp
- WavStreamWriter
- PatternCommands.cpp
- EntityId
- TestGainPlugin.cpp
- RemoveTrackCommand
- PianoRollModel
- AudioEngine
- read
- CoreAudioDevice
- [0.9.0] — 2026-08-16 — the core is complete
- DuplicateClipsCommand
- RenderOptions
- Pattern
- AddInsertCommand
- GraphBuilder
- INCDAW — Decision Log
- WavStreamReader
- InstrumentNode
- AudioRecorder
- MixerTests.cpp
- ConnectMixerCommand
- ProcessContext
- Sampler
- ChannelRackModel
- vector
- TempoMap
- SimpleSynth
- EqEffect
- 2. INCDAW functional scope
- AudioFileData
- AudioLogger
- AudioStream
- TestLatencyPlugin.cpp
- AudioDeviceConfig
- DelayEffect
- PluginLatencyTests.cpp
- PluginInstanceManager
- INCDAWMixerView
- MusicalPosition
- RecordingSession
- EditAssetRegionCommand
- AddMidiMappingCommand
- MixerStripNode
- CompiledProjectGraph
- CoreAudioDevice.cpp
- Clip
- GraphCompileOptions
- EffectParameter
- PlaylistView.mm
- ClapInstance
- INCDAW — Roadmap
- PluginRegistry
- MetronomeNode
- Sampler.cpp
- load
- MidiEvent
- INCDAWControlBarView
- Json
- read
- CallbackProfiler
- CompressorEffect
- MidiMessage
- Transport
- AudioDevice
- allocate
- InsertRecordedTakeCommand
- ParameterRegistry
- Model.h
- MoveClipsCommand
- INCDAWAppDelegate
- LoadSampleCommand
- PluginStateTests.cpp
- SamplerStreamingTests.cpp
- CommandRegistry
- WaveformOverview
- atomic
- AutomationNode
- AudioBufferView
- MixerNode
- PluginInsertTests.cpp
- RealtimeGuard.cpp
- 4. Specialised tests
- SamplerZoneStream
- PlaylistModel.cpp
- DelayLineNode
- CompiledGraph
- SamplerZone
- BasicMidiBuffer
- MidiInput
- ClapDescriptor
- FuzzTests.cpp
- write
- AddPatternClipCommand
- PlaylistModel
- SampleRingBuffer
- AudioBufferPool
- Instrument
- SimpleSynth.cpp
- SystemInfo
- PluginIdentifier
- Project
- ConstantNode
- SamplerTests.cpp
- Options
- TimingProbeInstrument
- ioProcTrampoline
- INCDAW — Plugin Host
- AutomationWriteSession
- ClapLibrary
- AudioClipNode
- SampleCache
- LevelMeter
- LockFreeQueue
- Smoother
- GainNode
- SineOscillatorNode
- NoteSequence
- EditFixture
- LoopbackResult
- INCDAW — Architecture
- CoreMidiDevice
- WriteAutomationCommand
- BuiltinEffectInfo
- BuiltinEffectTests.cpp
- INCDAW — Performance Strategy
- AddAutomationLaneCommand
- AudioEngine.h
- OrderRecordingNode
- ClapLibrary.cpp
- Json.cpp
- Parser
- PluginPersistenceTests.cpp
- InputMonitorNode
- BuiltinEffect.cpp
- Transport.cpp
- ChannelSamplerZone
- SamplerWiringTests.cpp
- AutomationPoint
- INCDAW — Audio Engine
- write
- AutomationCommands.cpp
- ToggleStepCommand
- Channel
- PluginParameterInfo
- main.mm
- RecordingPlacementTests.cpp
- MidiRecorder
- CoreMidiDevice.cpp
- ChildResult
- DiskStreamer
- KernelTable
- Track
- Fixture
- RealtimeSafetyTests.cpp
- INCDAW — Project Format
- BuiltinEffect
- TimeSignatureEvent
- MidiDevice
- SharedLibrary
- BlobReader
- PluginNode
- AudioAsset
- RecordingSink
- PatternListView.mm
- build
- TimestampedMidiMessage
- RecordedEvent
- capturePluginState
- PluginFolder
- make-dmg.sh
- setParameter
- SetAutomationPointsCommand
- RemoveAutomationLaneCommand
- MidiMapNode
- exportArrangement
- friend
- MidiMapping
- AutomationFixture
- create
- Version
- MidiDeviceInfo
- FrameCount
- MidiImportResult
- ProjectMetadata
- ScratchDir
- renderArrangement
- MidiTests.cpp
- Node
- RenderTests.cpp
- makeTestSignal
- INCDAW — Release Guide
- collectForBlock
- SequencedNote
- BlockSegment
- ScratchDirectory
- ScratchDirectory
- StressTests.cpp
- renderClickFrames
- process
- collectForRange
- renderProject
- INCDAWPianoRollView
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- v1.2/Fixture.incdaw/manifest.json
- v1.3/Fixture.incdaw/manifest.json
- v1.4/Fixture.incdaw/manifest.json
- renderBlock
- PatternTests.cpp
- bench/main.cpp
- StateIO
- check
- AudioCaptureSink
- ParameterSink
- ScratchDir
- snapTick
- .zoneCount
- NSSegmentedControl
- NSString
- NSTextField

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

## Communities (233 total, 9 thin omitted)

### Community 0 - "Command"
Cohesion: 0.03
Nodes (74): RemovedRouting, Command, execute, id, name, undo, AddMixerNodeCommand, execute (+66 more)

### Community 1 - "NoteCommands.cpp"
Cohesion: 0.04
Nodes (66): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, NoteIndices (+58 more)

### Community 2 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 3 - "ChannelCommands.cpp"
Cohesion: 0.04
Nodes (46): RemovedContent, AddChannelCommand, channel_, execute, index_, minted_, undo, size_t (+38 more)

### Community 4 - "WavStreamWriter"
Cohesion: 0.05
Nodes (59): ofstream, appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample (+51 more)

### Community 5 - "PatternCommands.cpp"
Cohesion: 0.04
Nodes (43): AddPatternCommand, execute, index_, minted_, pattern_, undo, DuplicatePatternCommand, execute (+35 more)

### Community 6 - "EntityId"
Cohesion: 0.10
Nodes (32): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, colourForIndex(), size_t (+24 more)

### Community 7 - "TestGainPlugin.cpp"
Cohesion: 0.06
Nodes (57): clap_gui_resize_hints_t, clap_id, clap_param_info_t, clap_window_t, applyParamEvents(), clap_host_t, clap_input_events_t, clap_istream_t (+49 more)

### Community 8 - "RemoveTrackCommand"
Cohesion: 0.05
Nodes (40): AddTrackCommand, execute, index_, minted_, track_, undo, RemovedClip, size_t (+32 more)

### Community 9 - "PianoRollModel"
Cohesion: 0.07
Nodes (40): NoteList, size_t, Tick, vector, size_t, Tick, vector, Viewport (+32 more)

### Community 10 - "AudioEngine"
Cohesion: 0.07
Nodes (49): RetiredGraph, AudioEngine, active_, anchor_, anchorVersion_, audioDeviceAboutToStart, audioDeviceStopped, availableDevices (+41 more)

### Community 11 - "read"
Cohesion: 0.09
Nodes (39): appendBigU16(), appendBigU32(), appendChunk(), appendVlq(), path, Result, size_t, Tick (+31 more)

### Community 12 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 13 - "[0.9.0] — 2026-08-16 — the core is complete"
Cohesion: 0.05
Nodes (43): [0.9.0] — 2026-08-16 — the core is complete, INCDAW — Changelog, Phase 0 — Research and architecture — 2026-08-14, Phase 10 — Mixer, routing and delay compensation — 2026-08-14, Phase 11a — Automation: the generic subsystem — 2026-08-15, Phase 11b — Automation placement and recording — 2026-08-15, Phase 12 (part 1) — WAV codec — 2026-08-15, Phase 12 (part 2) — Input capture and recording — 2026-08-15 (+35 more)

### Community 14 - "DuplicateClipsCommand"
Cohesion: 0.06
Nodes (31): ClipIds, DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_ (+23 more)

### Community 15 - "RenderOptions"
Cohesion: 0.07
Nodes (28): BitDepth, FrameCount, FramePosition, SampleRate, string, uint64_t, vector, RenderOptions (+20 more)

### Community 16 - "Pattern"
Cohesion: 0.11
Nodes (32): Emit, size_t, Tick, vector, noteAtStep(), execute, undo, Pattern (+24 more)

### Community 17 - "AddInsertCommand"
Cohesion: 0.08
Nodes (24): AddInsertCommand, execute, minted_, mixerNode_, plugin_, slot_, undo, findNode() (+16 more)

### Community 18 - "GraphBuilder"
Cohesion: 0.09
Nodes (29): Connection, process, FrameCount, FramePosition, MidiBuffer, NodeIndex, SampleRate, size_t (+21 more)

### Community 19 - "INCDAW — Decision Log"
Cohesion: 0.06
Nodes (34): D-001 — Core implementation language: C++20, D-002 — Build system: CMake + Ninja, D-003 — Audio I/O: CoreAudio HAL directly, no wrapper framework, D-004 — Realtime thread scheduling: os_workgroup / Audio Workgroups, D-005 — Platform strategy: macOS first, Windows later, Linux not precluded, D-006 — UI: AppKit shell + INCDAW-owned Metal-rendered widget layer, D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded), D-008 — Licensing: INCDAW is closed-source (+26 more)

### Community 20 - "WavStreamReader"
Cohesion: 0.07
Nodes (29): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+21 more)

### Community 21 - "InstrumentNode"
Cohesion: 0.11
Nodes (16): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, ParameterSink, unique_ptr, InstrumentNode (+8 more)

### Community 22 - "AudioRecorder"
Cohesion: 0.07
Nodes (28): AudioCaptureSink, AudioRecorder, captureAudioBlock, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_ (+20 more)

### Community 23 - "MixerTests.cpp"
Cohesion: 0.08
Nodes (21): FrameCount, FramePosition, Sample, SampleRate, size_t, vector, ImpulseNode, latency_ (+13 more)

### Community 24 - "ConnectMixerCommand"
Cohesion: 0.06
Nodes (27): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+19 more)

### Community 25 - "ProcessContext"
Cohesion: 0.10
Nodes (25): dbToGain(), sumInputsInto(), coefficientFor(), process, size_t, process, process, linkedPeakAt() (+17 more)

### Community 26 - "Sampler"
Cohesion: 0.06
Nodes (26): array, atomic, maxVoices, ParameterSink, SampleRate, uint64_t, vector, Voice (+18 more)

### Community 27 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 28 - "vector"
Cohesion: 0.12
Nodes (10): vector, string, unordered_map, mutex, ParameterSink, StateIO, AutomationPoint, Tick (+2 more)

### Community 29 - "TempoMap"
Cohesion: 0.12
Nodes (27): execute, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+19 more)

### Community 30 - "SimpleSynth"
Cohesion: 0.08
Nodes (22): uint32_t, array, atomic, maxVoices, ParameterSink, Sample, SampleRate, uint64_t (+14 more)

### Community 31 - "EqEffect"
Cohesion: 0.09
Nodes (22): Coefficients, FrameCount, SampleRate, EqEffect, bandCount, cached_, coefficients_, maxChannels (+14 more)

### Community 32 - "2. INCDAW functional scope"
Cohesion: 0.07
Nodes (29): 1.1 Findings that changed INCDAW's architecture, 1.2 Supported plugin formats (official), 1.3 Other FL Studio 2026 features, recorded for completeness, 1. Functional reference: FL Studio 2026, 2. INCDAW functional scope, 3. Non-functional requirements, Audio editor, Audio engine (+21 more)

### Community 33 - "AudioFileData"
Cohesion: 0.14
Nodes (25): applyGain(), applyRamp(), clampedRegion(), Sample, fadeIn(), fadeOut(), FrameCount, normalize() (+17 more)

### Community 34 - "AudioLogger"
Cohesion: 0.09
Nodes (22): AudioLogger, capacityFrames_, circle_, enabled_, grab, log, prepare, ready_ (+14 more)

### Community 35 - "AudioStream"
Cohesion: 0.10
Nodes (23): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+15 more)

### Community 36 - "TestLatencyPlugin.cpp"
Cohesion: 0.11
Nodes (25): clap_host_t, clap_plugin_descriptor_t, clap_plugin_factory_t, clap_plugin_t, clap_process_status, clap_process_t, vector, factoryCreatePlugin() (+17 more)

### Community 37 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 38 - "DelayEffect"
Cohesion: 0.08
Nodes (24): Allpass, Comb, FrameCount, SampleRate, DelayEffect, capacity_, lines_, maxChannels (+16 more)

### Community 39 - "PluginLatencyTests.cpp"
Cohesion: 0.16
Nodes (9): FrameCount, path, Sample, vector, ImpulseNode, at_, render(), ScratchDir (+1 more)

### Community 40 - "PluginInstanceManager"
Cohesion: 0.11
Nodes (26): Held, size_t, string, uint32_t, uint64_t, unique_ptr, vector, mutex (+18 more)

### Community 41 - "INCDAWMixerView"
Cohesion: 0.14
Nodes (25): incdaw, NSArray, NSDictionary, NSView, INCDAWMixerView, -acceptsFirstResponder, -addStripRect, -drawRect (+17 more)

### Community 42 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 43 - "RecordingSession"
Cohesion: 0.09
Nodes (20): path, Placement, string, vector, FrameCount, FramePosition, uint32_t, uint64_t (+12 more)

### Community 44 - "EditAssetRegionCommand"
Cohesion: 0.09
Nodes (21): AudioEditOp, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_, minted_ (+13 more)

### Community 45 - "AddMidiMappingCommand"
Cohesion: 0.10
Nodes (17): AddMidiMappingCommand, controller_, execute, mapping_, midiChannel_, minted_, parameterKey_, target_ (+9 more)

### Community 46 - "MixerStripNode"
Cohesion: 0.13
Nodes (19): FrameCount, Sample, SampleRate, atomic, Sample, MixerStripNode, left_, meter_ (+11 more)

### Community 47 - "CompiledProjectGraph"
Cohesion: 0.10
Nodes (24): CompiledProjectGraph, automation, channels, channelStripFor, channelStrips, error, graph, insertSlots (+16 more)

### Community 48 - "CoreAudioDevice.cpp"
Cohesion: 0.29
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 49 - "Clip"
Cohesion: 0.08
Nodes (25): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+17 more)

### Community 50 - "GraphCompileOptions"
Cohesion: 0.08
Nodes (25): PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, insertFactory, instrumentFactory, masterGain, maxBlockSize (+17 more)

### Community 51 - "EffectParameter"
Cohesion: 0.12
Nodes (15): BuiltinEffect::BuiltinEffect(), EffectParameter, defaultValue, id, maxValue, minValue, name, stepped (+7 more)

### Community 52 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (24): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinBody, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+16 more)

### Community 53 - "ClapInstance"
Cohesion: 0.08
Nodes (25): clap_plugin_gui_t, clap_plugin_state_t, ParamEvent, ClapInstance, editorOpen_, gui_, hasEditor, host_ (+17 more)

### Community 54 - "INCDAW — Roadmap"
Cohesion: 0.08
Nodes (23): Deliberately out of scope, INCDAW — Roadmap, Phase 0 — Research and architecture ✅ COMPLETE, Phase 10 — Mixer and routing, Phase 11 — Automation, Phase 12 — Recording and audio editor, Phase 13 — Plugin hosting, Phase 14 — Sampler (+15 more)

### Community 55 - "PluginRegistry"
Cohesion: 0.15
Nodes (21): Library, Located, int64_t, path, size_t, string, uint64_t, vector (+13 more)

### Community 56 - "MetronomeNode"
Cohesion: 0.09
Nodes (17): atomic, FrameCount, Sample, SampleRate, size_t, vector, MetronomeNode, amplitude_ (+9 more)

### Community 57 - "Sampler.cpp"
Cohesion: 0.16
Nodes (22): FrameCount, Sample, SampleRate, vector, Voice, interpolate(), activeVoiceCount, allNotesOff (+14 more)

### Community 58 - "load"
Cohesion: 0.27
Nodes (18): automationPointFrom(), bindUnassignedContent(), path, Result, string, idFrom(), midiEventFrom(), pluginFrom() (+10 more)

### Community 59 - "MidiEvent"
Cohesion: 0.10
Nodes (25): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+17 more)

### Community 60 - "INCDAWControlBarView"
Cohesion: 0.11
Nodes (21): NSInteger, NSString, NSView, INCDAWControlBarView, -drawDisplay, -drawReadouts, -drawRect, -initWithFrame (+13 more)

### Community 61 - "Json"
Cohesion: 0.09
Nodes (14): nullptr_t, int64_t, pair, string, vector, Json, asBool, boolean_ (+6 more)

### Community 62 - "read"
Cohesion: 0.17
Nodes (20): path, Result, size_t, uint16_t, uint32_t, uint8_t, vector, fillMetadata() (+12 more)

### Community 63 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 64 - "CompressorEffect"
Cohesion: 0.11
Nodes (18): CompressorEffect, envelope_, prepare, reduction_, sampleRate_, FrameCount, SampleRate, GateEffect (+10 more)

### Community 65 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 66 - "Transport"
Cohesion: 0.12
Nodes (12): atomic, FramePosition, size_t, Tick, uint32_t, Transport, loopEnabled_, maxSegmentsPerBlock (+4 more)

### Community 67 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 68 - "allocate"
Cohesion: 0.14
Nodes (16): allocate, FrameCount, size_t, FramePosition, anyNonZero(), Sample, vector, ParameterFixture (+8 more)

### Community 69 - "InsertRecordedTakeCommand"
Cohesion: 0.10
Nodes (16): Placement, size_t, string, vector, InsertRecordedTakeCommand, asset_, assetIndex_, clipIndices_ (+8 more)

### Community 70 - "ParameterRegistry"
Cohesion: 0.18
Nodes (19): Applier, convertParameters(), Entry, size_t, string, uint32_t, vector, Entry (+11 more)

### Community 71 - "Model.h"
Cohesion: 0.11
Nodes (17): AutomationCurve, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint, curve (+9 more)

### Community 72 - "MoveClipsCommand"
Cohesion: 0.13
Nodes (18): MovedAudioClip, execute, string, execute, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, canMergeWith (+10 more)

### Community 73 - "INCDAWAppDelegate"
Cohesion: 0.09
Nodes (18): NSApplicationDelegate, NSObject, NSWindow, NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform, NSView (+10 more)

### Community 74 - "LoadSampleCommand"
Cohesion: 0.11
Nodes (15): size_t, string, vector, LoadSampleCommand, asset_, assetIndex_, channelId_, created_ (+7 more)

### Community 75 - "PluginStateTests.cpp"
Cohesion: 0.12
Nodes (17): anyNonZero(), compileLoaded(), InsertFactory, path, Sample, uint8_t, vector, factoryFor() (+9 more)

### Community 76 - "SamplerStreamingTests.cpp"
Cohesion: 0.13
Nodes (18): FrameCount, MidiBuffer, path, Sample, shared_ptr, size_t, string, vector (+10 more)

### Community 77 - "CommandRegistry"
Cohesion: 0.05
Nodes (49): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+41 more)

### Community 78 - "WaveformOverview"
Cohesion: 0.11
Nodes (17): Bucket, FrameCount, SampleRate, size_t, vector, WaveformOverview, channelCount, channels (+9 more)

### Community 79 - "atomic"
Cohesion: 0.20
Nodes (3): atomic, MidiBuffer, array

### Community 80 - "AutomationNode"
Cohesion: 0.11
Nodes (11): AutomationNode, bindings_, tempoMap_, Binding, size_t, vector, controlChange(), path (+3 more)

### Community 81 - "AudioBufferView"
Cohesion: 0.20
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 82 - "MixerNode"
Cohesion: 0.11
Nodes (19): MixerNodeType, string, uint32_t, MixerNode, colour, id, inserts, muted (+11 more)

### Community 83 - "PluginInsertTests.cpp"
Cohesion: 0.11
Nodes (20): anyNonZero(), ClipInsert, threshold_, FrameCount, path, Sample, vector, GainInsert (+12 more)

### Community 84 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 85 - "4. Specialised tests"
Cohesion: 0.11
Nodes (18): 1. Principle, 2. Framework, 3. Test levels, 4. Specialised tests, 5. What is not tested automatically, End-to-end, Fuzzing (from Phase 4), Golden-file audio (from Phase 7) (+10 more)

### Community 86 - "SamplerZoneStream"
Cohesion: 0.14
Nodes (16): Slot, uint64_t, array, FrameCount, shared_ptr, size_t, SamplerZoneStream, claimSlot (+8 more)

### Community 87 - "PlaylistModel.cpp"
Cohesion: 0.20
Nodes (18): Rect, size_t, vector, addToSelection, clipAtPoint, clipRect, clipsInRectangle, collectVisibleClips (+10 more)

### Community 88 - "DelayLineNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_, prepare (+6 more)

### Community 89 - "CompiledGraph"
Cohesion: 0.13
Nodes (13): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+5 more)

### Community 90 - "SamplerZone"
Cohesion: 0.11
Nodes (18): FrameCount, shared_ptr, handleMessage, SamplerZone, end, gain, keyHigh, keyLow (+10 more)

### Community 91 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 92 - "MidiInput"
Cohesion: 0.14
Nodes (13): atomic, queueCapacity, size_t, uint64_t, MidiInput, dropped_, hasPending_, lastControl_ (+5 more)

### Community 93 - "ClapDescriptor"
Cohesion: 0.12
Nodes (17): ClapDescriptor, id, name, vendor, version, string, path, string (+9 more)

### Community 94 - "FuzzTests.cpp"
Cohesion: 0.16
Nodes (13): corrupt(), path, size_t, string, uint64_t, uint8_t, vector, Random (+5 more)

### Community 95 - "write"
Cohesion: 0.19
Nodes (16): int32_t, AiffFile, write, appendBigU16(), appendBigU32(), appendExtended(), appendId(), Format (+8 more)

### Community 96 - "AddPatternClipCommand"
Cohesion: 0.11
Nodes (11): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+3 more)

### Community 97 - "PlaylistModel"
Cohesion: 0.16
Nodes (10): size_t, Tick, vector, Viewport, PlaylistModel, noClip, noTrack, resizeHandleWidth (+2 more)

### Community 98 - "SampleRingBuffer"
Cohesion: 0.19
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 99 - "AudioBufferPool"
Cohesion: 0.14
Nodes (11): AudioBufferPool, channelPointers_, reset, samples_, FrameCount, Sample, size_t, unique_ptr (+3 more)

### Community 100 - "Instrument"
Cohesion: 0.14
Nodes (10): MidiBuffer, ParameterSink, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare (+2 more)

### Community 101 - "SimpleSynth.cpp"
Cohesion: 0.15
Nodes (17): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), polyBlep(), activeVoiceCount (+9 more)

### Community 102 - "SystemInfo"
Cohesion: 0.14
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 103 - "PluginIdentifier"
Cohesion: 0.13
Nodes (15): builtinSampler(), builtinSimpleSynth(), Format, string, formatName(), Format, friend, string (+7 more)

### Community 104 - "Project"
Cohesion: 0.06
Nodes (37): Project, audioAssets_, automation_, channels_, clips_, findMixerNode, ids_, master_ (+29 more)

### Community 105 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 106 - "SamplerTests.cpp"
Cohesion: 0.18
Nodes (17): constantSample(), FrameCount, MidiBuffer, Sample, shared_ptr, vector, makeEnvelopeTransparent(), nyquistSample() (+9 more)

### Community 107 - "Options"
Cohesion: 0.12
Nodes (17): int64_t, string, Options, amplitude, buffer, device, frequency, input (+9 more)

### Community 108 - "TimingProbeInstrument"
Cohesion: 0.14
Nodes (10): Applied, FrameCount, MidiBuffer, Sample, SampleRate, vector, renderSynth(), TimingProbeInstrument (+2 more)

### Community 109 - "ioProcTrampoline"
Cohesion: 0.21
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 110 - "INCDAW — Plugin Host"
Cohesion: 0.12
Nodes (16): 10. Testing, 1. Supported formats, 2. Prime directive, 3. Pipeline, 4. Isolation strategy, 5. Parameter system, 6. State, 7. Editor / UI bridge (+8 more)

### Community 111 - "AutomationWriteSession"
Cohesion: 0.14
Nodes (12): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, string, Tick (+4 more)

### Community 112 - "ClapLibrary"
Cohesion: 0.18
Nodes (8): clap_plugin_entry_t, ClapLibrary, descriptors, entry_, factory_, library_, clap_plugin_factory_t, main()

### Community 113 - "AudioClipNode"
Cohesion: 0.13
Nodes (13): AudioClipNode, addClip, clips_, fetchScratch_, prepare, process, FrameCount, PlacedClip (+5 more)

### Community 114 - "SampleCache"
Cohesion: 0.15
Nodes (16): int64_t, path, shared_ptr, size_t, string, Entry, mutex, string (+8 more)

### Community 115 - "LevelMeter"
Cohesion: 0.15
Nodes (12): atomic, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond, rmsWindowSeconds (+4 more)

### Community 116 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 117 - "Smoother"
Cohesion: 0.18
Nodes (9): atomic, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds, sampleRate_ (+1 more)

### Community 118 - "GainNode"
Cohesion: 0.15
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 119 - "SineOscillatorNode"
Cohesion: 0.13
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 120 - "NoteSequence"
Cohesion: 0.17
Nodes (13): Tick, vector, Tick, uint32_t, vector, NoteSequence, byEnd_, clear (+5 more)

### Community 121 - "EditFixture"
Cohesion: 0.15
Nodes (13): FrameCount, path, Sample, size_t, EditFixture, assetId, file, project (+5 more)

### Community 122 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 123 - "INCDAW — Architecture"
Cohesion: 0.12
Nodes (15): 1. Guiding principle, 2. Layer model, 3. Proposed repository structure, 4. Threading model, 5. Data model, 6. Command architecture, 7. Engine boundary, 8. Plugin isolation (+7 more)

### Community 124 - "CoreMidiDevice"
Cohesion: 0.15
Nodes (14): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+6 more)

### Community 125 - "WriteAutomationCommand"
Cohesion: 0.12
Nodes (15): WriteAutomationCommand, clip_, clipIndex_, key_, laneAfter_, laneCreated_, laneId_, laneIndex_ (+7 more)

### Community 126 - "BuiltinEffectInfo"
Cohesion: 0.16
Nodes (14): BuiltinEffectInfo, displayName, parameterCount, parameters, uid, CatalogueEntry, info, make (+6 more)

### Community 127 - "BuiltinEffectTests.cpp"
Cohesion: 0.19
Nodes (15): FrameCount, Sample, size_t, vector, processThrough(), RefAllpass, index, line (+7 more)

### Community 128 - "INCDAW — Performance Strategy"
Cohesion: 0.13
Nodes (14): 1. Reference machine, 2. Targets, 3. Instrumentation, 4. Method, 5. Known design-level performance decisions, 6. Profiling tooling, 7. Phase 18 — measured baseline and optimisations, Audio (+6 more)

### Community 129 - "AddAutomationLaneCommand"
Cohesion: 0.15
Nodes (8): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, size_t, string

### Community 130 - "AudioEngine.h"
Cohesion: 0.15
Nodes (8): AudioCaptureSink, FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 131 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 132 - "ClapLibrary.cpp"
Cohesion: 0.14
Nodes (18): clap_event_header_t, blobRead(), closeEditor, close, open, clap_host_t, clap_input_events_t, clap_istream_t (+10 more)

### Community 133 - "Json.cpp"
Cohesion: 0.20
Nodes (14): int64_t, size_t, string, escapeInto(), formatDouble(), append, asDouble, asInt (+6 more)

### Community 134 - "Parser"
Cohesion: 0.30
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 135 - "PluginPersistenceTests.cpp"
Cohesion: 0.14
Nodes (11): path, string, uint8_t, vector, gainBlob(), Harness, folder, registry (+3 more)

### Community 136 - "InputMonitorNode"
Cohesion: 0.15
Nodes (9): FrameCount, Sample, SampleRate, size_t, vector, InputMonitorNode, channelCount_, ring_ (+1 more)

### Community 137 - "BuiltinEffect.cpp"
Cohesion: 0.37
Nodes (12): appendF64(), appendU32(), loadState, saveState, setParameter, value, size_t, uint32_t (+4 more)

### Community 138 - "Transport.cpp"
Cohesion: 0.18
Nodes (13): FrameCount, FramePosition, size_t, applyPendingSeek, pause, play, processBlock, seek (+5 more)

### Community 139 - "ChannelSamplerZone"
Cohesion: 0.14
Nodes (14): ChannelSamplerZone, asset, end, gain, keyHigh, keyLow, loopCrossfade, loopEnd (+6 more)

### Community 140 - "SamplerWiringTests.cpp"
Cohesion: 0.19
Nodes (10): path, string, noteAtZero(), SamplerProject, asset, channel, project, ScratchDirectory (+2 more)

### Community 141 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 142 - "INCDAW — Audio Engine"
Cohesion: 0.15
Nodes (12): 10. Audio correctness requirements, 11. Performance budget, 1. The prime directive, 2. Device layer, 3. Realtime thread scheduling, 4. Realtime safety enforcement, 5. Signal flow, 6. Block processing and sample-accurate events (+4 more)

### Community 143 - "write"
Cohesion: 0.29
Nodes (14): assetFilePath(), Sample, string, vector, execute, name, undo, findAsset() (+6 more)

### Community 144 - "AutomationCommands.cpp"
Cohesion: 0.19
Nodes (12): execute, undo, AutomationPoint, vector, findLane(), canMergeWith, execute, mergeWith (+4 more)

### Community 145 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (8): size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_, step_

### Community 146 - "Channel"
Cohesion: 0.15
Nodes (13): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+5 more)

### Community 147 - "PluginParameterInfo"
Cohesion: 0.20
Nodes (9): string, uint32_t, PluginParameterInfo, defaultValue, id, maxValue, minValue, name (+1 more)

### Community 148 - "main.mm"
Cohesion: 0.19
Nodes (16): NSScrollView, NSSplitView, -applicationDidFinishLaunching, -handleTransportAction, NSView, -openAudioAssetInEditor, -selectChannel, -selectPattern (+8 more)

### Community 149 - "RecordingPlacementTests.cpp"
Cohesion: 0.21
Nodes (11): FrameCount, path, Sample, shared_ptr, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 150 - "MidiRecorder"
Cohesion: 0.14
Nodes (14): CapturedMessage, FramePosition, MidiBuffer, atomic, queueCapacity, size_t, uint64_t, MidiRecorder (+6 more)

### Community 151 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 152 - "ChildResult"
Cohesion: 0.17
Nodes (10): End, ChildResult, code, end, output, path, string, vector (+2 more)

### Community 153 - "DiskStreamer"
Cohesion: 0.18
Nodes (11): shared_ptr, DiskStreamer, add, mutex_, running_, streams_, thread_, atomic (+3 more)

### Community 154 - "KernelTable"
Cohesion: 0.21
Nodes (9): SampleRate, vector, KernelTable, phases, weights_, width, resample(), sinc() (+1 more)

### Community 155 - "Track"
Cohesion: 0.17
Nodes (12): findTrack, Track, colour, height, id, muted, name, outputMixerNode (+4 more)

### Community 156 - "Fixture"
Cohesion: 0.20
Nodes (10): Tick, vector, Fixture, channel, pattern, project, trackA, trackB (+2 more)

### Community 157 - "RealtimeSafetyTests.cpp"
Cohesion: 0.25
Nodes (7): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister(), allocationSize(), size_t

### Community 158 - "INCDAW — Project Format"
Cohesion: 0.18
Nodes (10): 1. Shape: a package directory, not a single file, 2. Versioning and migration, 3. Text vs binary, 4. Media: referenced or embedded, 5. Autosave, backup and recovery, 6. Archiving, 7. Determinism, 8. Tests (Phase 4 gate) (+2 more)

### Community 159 - "BuiltinEffect"
Cohesion: 0.16
Nodes (10): BuiltinEffect, values_, atomic, size_t, vector, AnalyzerEffect, maxChannels, atomic (+2 more)

### Community 160 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 161 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 162 - "SharedLibrary"
Cohesion: 0.29
Nodes (7): path, string, SharedLibrary, close, handle_, open, symbol

### Community 163 - "BlobReader"
Cohesion: 0.15
Nodes (15): BlobReader, cursor, data, size, blobWrite(), BlobWriter, out, overflowed (+7 more)

### Community 164 - "PluginNode"
Cohesion: 0.18
Nodes (5): FrameCount, ParameterSink, StateIO, PluginNode, instance_

### Community 165 - "AudioAsset"
Cohesion: 0.18
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 166 - "RecordingSink"
Cohesion: 0.40
Nodes (5): pair, ParameterSink, uint32_t, RecordingSink, received

### Community 167 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 168 - "build"
Cohesion: 0.27
Nodes (9): bucketize(), Bucket, FrameCount, path, Result, Sample, vector, sizeBuckets() (+1 more)

### Community 169 - "TimestampedMidiMessage"
Cohesion: 0.22
Nodes (9): midiMessageReceived, sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos (+1 more)

### Community 170 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 171 - "capturePluginState"
Cohesion: 0.51
Nodes (9): capturePluginState(), path, string, uint8_t, vector, readBlobFile(), restorePluginState(), stateFileNameFor() (+1 more)

### Community 172 - "PluginFolder"
Cohesion: 0.24
Nodes (7): path, PluginFolder, crash, dir, gain, ScratchDir, path

### Community 173 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 174 - "setParameter"
Cohesion: 0.22
Nodes (3): FilterMode, uint32_t, setParameter

### Community 175 - "SetAutomationPointsCommand"
Cohesion: 0.31
Nodes (6): AutomationPoint, vector, SetAutomationPointsCommand, laneId_, points_, previous_

### Community 176 - "RemoveAutomationLaneCommand"
Cohesion: 0.22
Nodes (6): RemoveAutomationLaneCommand, execute, index_, lane_, laneId_, undo

### Community 177 - "MidiMapNode"
Cohesion: 0.25
Nodes (5): Binding, size_t, vector, MidiMapNode, bindings_

### Community 178 - "exportArrangement"
Cohesion: 0.25
Nodes (7): size_t, notes_, path, Result, uint64_t, exportArrangement(), importAsPattern()

### Community 180 - "MidiMapping"
Cohesion: 0.22
Nodes (8): MidiMapping, controller, id, maxValue, midiChannel, minValue, parameterKey, targetEntity

### Community 181 - "AutomationFixture"
Cohesion: 0.29
Nodes (5): AutomationFixture, channel, pattern, project, tempo

### Community 182 - "create"
Cohesion: 0.25
Nodes (8): clap_event_param_value_t, create, array, string, unique_ptr, PendingParamEvents, count, events

### Community 183 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 184 - "MidiDeviceInfo"
Cohesion: 0.25
Nodes (6): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback

### Community 186 - "MidiImportResult"
Cohesion: 0.25
Nodes (7): string, vector, MidiImportResult, error, newChannels, pattern, succeeded

### Community 187 - "ProjectMetadata"
Cohesion: 0.25
Nodes (8): ProjectMetadata, artist, comment, created, createdWith, lastSavedWith, modified, title

### Community 188 - "ScratchDir"
Cohesion: 0.50
Nodes (3): path, ScratchDir, path

### Community 189 - "renderArrangement"
Cohesion: 0.39
Nodes (7): FrameCount, Sample, size_t, vector, makeAudio(), renderArrangement(), tone()

### Community 190 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped()

### Community 191 - "Node"
Cohesion: 0.12
Nodes (11): Node, process, ParameterSink, StateIO, function, InsertFactory, unique_ptr, ScriptedFactory (+3 more)

### Community 192 - "RenderTests.cpp"
Cohesion: 0.25
Nodes (5): path, string, makeArrangedProject(), ScratchDirectory, path

### Community 193 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 194 - "INCDAW — Release Guide"
Cohesion: 0.29
Nodes (6): 1. What a release is, 2. Cutting a release, 3. Installing (first launch on another Mac), 4. Updating, 5. Release notes — 0.9.0 (2026-08-16), INCDAW — Release Guide

### Community 195 - "collectForBlock"
Cohesion: 0.29
Nodes (6): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock, resetCounters

### Community 196 - "SequencedNote"
Cohesion: 0.29
Nodes (6): SequencedNote, channel, key, lengthTicks, startTick, velocity

### Community 197 - "BlockSegment"
Cohesion: 0.29
Nodes (6): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount

### Community 198 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 199 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 200 - "StressTests.cpp"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 201 - "renderClickFrames"
Cohesion: 0.33
Nodes (6): FrameCount, FramePosition, size_t, vector, plan(), renderClickFrames()

### Community 203 - "process"
Cohesion: 0.40
Nodes (5): FrameCount, SampleRate, prepare, process, triggerClick

### Community 204 - "collectForRange"
Cohesion: 0.33
Nodes (4): FrameCount, FramePosition, MidiBuffer, collectForRange

### Community 205 - "renderProject"
Cohesion: 0.17
Nodes (12): clipLengthTicks(), clipStartTicks(), Tick, findChannel, arrangementEndFrames(), FrameCount, path, uint64_t (+4 more)

### Community 206 - "INCDAWPianoRollView"
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

### Community 213 - "PatternTests.cpp"
Cohesion: 0.53
Nodes (5): Tick, vector, note(), shapeOf(), startsOf()

### Community 214 - "bench/main.cpp"
Cohesion: 0.47
Nodes (5): time_point, vector, main(), median(), millisecondsSince()

### Community 215 - "StateIO"
Cohesion: 0.40
Nodes (3): StateIO, loadState, saveState

### Community 220 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 224 - "ScratchDir"
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
- **1348 isolated node(s):** `execute`, `id`, `name`, `undo`, `index_` (+1343 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **9 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `EntityId` connect `EntityId` to `Command`, `AddAutomationLaneCommand`, `NoteCommands.cpp`, `ChannelCommands.cpp`, `PatternCommands.cpp`, `RemoveTrackCommand`, `PianoRollModel`, `ChannelSamplerZone`, `SamplerWiringTests.cpp`, `DuplicateClipsCommand`, `write`, `AutomationCommands.cpp`, `AddInsertCommand`, `Channel`, `Pattern`, `RenderOptions`, `InstrumentNode`, `MixerTests.cpp`, `ConnectMixerCommand`, `Track`, `Fixture`, `AudioFileData`, `AudioAsset`, `EditAssetRegionCommand`, `AddMidiMappingCommand`, `SetAutomationPointsCommand`, `RemoveAutomationLaneCommand`, `Clip`, `CompiledProjectGraph`, `GraphCompileOptions`, `MidiMapping`, `AutomationFixture`, `MidiImportResult`, `load`, `allocate`, `InsertRecordedTakeCommand`, `Model.h`, `MoveClipsCommand`, `LoadSampleCommand`, `PluginStateTests.cpp`, `renderProject`, `CommandRegistry`, `MixerNode`, `PluginInsertTests.cpp`, `PlaylistModel.cpp`, `AddPatternClipCommand`, `PlaylistModel`, `PluginIdentifier`, `Project`, `AutomationWriteSession`, `EditFixture`, `WriteAutomationCommand`?**
  _High betweenness centrality (0.123) - this node is a cross-community bridge._