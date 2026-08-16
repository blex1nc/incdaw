# Graph Report - onay-devam-6548af  (2026-08-16)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 4482 nodes · 7852 edges · 230 communities (226 shown, 4 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 381 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `c47506db`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- MixerCommands.cpp
- INCDAW
- Channel
- Project
- Command
- NoteCommands.cpp
- Track
- ChannelCommands.cpp
- WavStreamWriter
- TestGainPlugin.cpp
- PianoRollModel
- Transport
- Pattern
- read
- CoreAudioDevice
- [0.9.0] — 2026-08-16 — the core is complete
- vector
- RenderOptions
- PlaylistModel
- AudioStream
- MidiEvent
- AddInsertCommand
- atomic
- ProcessContext
- INCDAW — Decision Log
- AddMidiMappingCommand
- Sampler
- GraphBuilder
- WavStreamReader
- MixerTests.cpp
- ChannelRackModel
- TempoMap
- SimpleSynth
- EqEffect
- 2. INCDAW functional scope
- MetronomeNode
- DelayEffect
- AudioFileData
- AudioLogger
- TestLatencyPlugin.cpp
- ClapInstance
- PluginInstanceManager
- Json
- MusicalPosition
- AudioDeviceConfig
- Json.cpp
- RecordingSession
- CoreAudioDevice.cpp
- INCDAWMixerView
- MixerStripNode
- EditAssetRegionCommand
- Clip
- GraphCompileOptions
- ConnectMixerCommand
- MidiMessage
- CompiledProjectGraph
- PlaylistView.mm
- AudioRecorder
- INCDAW — Roadmap
- AudioEngine
- .buffer
- Sampler.cpp
- ConstantNode
- read
- AudioBufferView
- CallbackProfiler
- CompressorEffect
- AudioDevice
- ClapLibrary.cpp
- InsertRecordedTakeCommand
- BuiltinEffect
- ParameterRegistry
- InstrumentNode
- PluginIdentifier
- PluginInsertTests.cpp
- PluginStateTests.cpp
- SamplerStreamingTests.cpp
- AutomationTests.cpp
- CommandRegistry.cpp
- LoadSampleCommand
- WaveformOverview
- AutomationNode
- RealtimeGuard.cpp
- 4. Specialised tests
- SamplerZoneStream
- AudioEngine.cpp
- DelayLineNode
- CompiledGraph
- SamplerZone
- BasicMidiBuffer
- MidiInput
- load
- FuzzTests.cpp
- ClapLibrary
- write
- CoreMidiDevice
- main.mm
- AudioBufferPool
- Node
- SimpleSynth.cpp
- SystemInfo
- CountingCommand
- SamplerTests.cpp
- TimingProbeInstrument
- ioProcTrampoline
- MidiRecorder
- INCDAW — Plugin Host
- AudioClipNode
- SampleCache
- main
- SampleRingBuffer
- LevelMeter
- Smoother
- PluginParameterInfo
- GainNode
- SineOscillatorNode
- NoteSequence
- EditFixture
- LoopbackResult
- BuiltinEffectTests.cpp
- INCDAW — Architecture
- PluginRegistry
- AutomationWriteSession
- CommandRegistry
- WriteAutomationCommand
- allocate
- EffectParameter
- INCDAW — Performance Strategy
- AddAutomationLaneCommand
- BuiltinEffectInfo
- Instrument
- INCDAWAppDelegate
- InputMonitorNode
- SamplerWiringTests.cpp
- Options
- INCDAW — Audio Engine
- AudioEditCommands.cpp
- ChannelRackView.mm
- RecordingPlacementTests.cpp
- CoreMidiDevice.cpp
- KernelTable
- Fixture
- INCDAW — Project Format
- ChildResult
- AutomationCommands.cpp
- AddPatternClipCommand
- captureAudioBlock
- TimelineAnchor
- TimeSignatureEvent
- MidiDevice
- SharedLibrary
- BlobReader
- PluginNode
- humanizeNoteStarts
- AutomationProbe
- PatternListView.mm
- SetAutomationPointsCommand
- ClipCommands.cpp
- build
- TimestampedMidiMessage
- RecordedEvent
- capturePluginState
- Harness
- PluginFolder
- make-dmg.sh
- setParameter
- MoveClipsCommand
- RemoveAutomationLaneCommand
- RemoveClipsCommand
- DuplicateClipsCommand
- ResizeClipsCommand
- MidiMapNode
- exportArrangement
- AutomationFixture
- create
- ClipIds
- Denormals.h
- SetClipMutedCommand
- updatedRecents
- Version
- MidiImportResult
- Fixture
- renderArrangement
- MidiTests.cpp
- ScriptedFactory
- RenderTests.cpp
- makeTestSignal
- INCDAW — Release Guide
- AnalyzerEffect
- collectForBlock
- SequencedNote
- scanDirectory
- ScanOutcome
- ScratchDirectory
- ScratchDirectory
- StressTests.cpp
- INCDAWPianoRollView
- string
- collectForRange
- ClapDescriptor
- DitherSource
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
- MidiDeviceInfo
- scanOutOfProcess
- INCDAWAudioEditorView
- INCDAWPlaylistView
- Fixture
- check
- AudioCaptureSink
- ParameterSink
- ScratchDir
- ScratchDir
- ScratchDir
- gainBlob

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

## Communities (230 total, 4 thin omitted)

### Community 0 - "MixerCommands.cpp"
Cohesion: 0.03
Nodes (71): RemovedRouting, AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo (+63 more)

### Community 1 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 2 - "Channel"
Cohesion: 0.03
Nodes (72): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+64 more)

### Community 3 - "Project"
Cohesion: 0.06
Nodes (57): SetMixerMutedCommand, execute, muted_, nodeId_, undo, Tick, EntityId, invalidValue (+49 more)

### Community 4 - "Command"
Cohesion: 0.04
Nodes (48): Command, execute, id, name, undo, AddPatternCommand, execute, index_ (+40 more)

### Community 5 - "NoteCommands.cpp"
Cohesion: 0.04
Nodes (58): undo, NoteIndices, size_t, string, vector, DeleteNotesCommand, channel_, execute (+50 more)

### Community 6 - "Track"
Cohesion: 0.04
Nodes (52): AddTrackCommand, execute, index_, minted_, track_, undo, RemovedClip, size_t (+44 more)

### Community 7 - "ChannelCommands.cpp"
Cohesion: 0.04
Nodes (46): RemovedContent, AddChannelCommand, channel_, execute, index_, minted_, undo, size_t (+38 more)

### Community 8 - "WavStreamWriter"
Cohesion: 0.05
Nodes (59): ofstream, appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample (+51 more)

### Community 9 - "TestGainPlugin.cpp"
Cohesion: 0.06
Nodes (57): clap_gui_resize_hints_t, clap_id, clap_param_info_t, clap_window_t, applyParamEvents(), clap_host_t, clap_input_events_t, clap_istream_t (+49 more)

### Community 10 - "PianoRollModel"
Cohesion: 0.07
Nodes (39): NoteList, size_t, Tick, vector, size_t, Tick, vector, Viewport (+31 more)

### Community 11 - "Transport"
Cohesion: 0.06
Nodes (34): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FramePosition, size_t (+26 more)

### Community 12 - "Pattern"
Cohesion: 0.07
Nodes (43): AutomationCurve, Emit, size_t, Tick, vector, noteAtStep(), execute, undo (+35 more)

### Community 13 - "read"
Cohesion: 0.09
Nodes (39): appendBigU16(), appendBigU32(), appendChunk(), appendVlq(), path, Result, size_t, Tick (+31 more)

### Community 14 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 15 - "[0.9.0] — 2026-08-16 — the core is complete"
Cohesion: 0.05
Nodes (43): [0.9.0] — 2026-08-16 — the core is complete, INCDAW — Changelog, Phase 0 — Research and architecture — 2026-08-14, Phase 10 — Mixer, routing and delay compensation — 2026-08-14, Phase 11a — Automation: the generic subsystem — 2026-08-15, Phase 11b — Automation placement and recording — 2026-08-15, Phase 12 (part 1) — WAV codec — 2026-08-15, Phase 12 (part 2) — Input capture and recording — 2026-08-15 (+35 more)

### Community 16 - "vector"
Cohesion: 0.09
Nodes (4): vector, string, unordered_map, mutex

### Community 17 - "RenderOptions"
Cohesion: 0.06
Nodes (37): BitDepth, clipLengthTicks(), clipStartTicks(), Tick, findChannel, arrangementEndFrames(), FrameCount, path (+29 more)

### Community 18 - "PlaylistModel"
Cohesion: 0.10
Nodes (30): Rect, size_t, Tick, vector, size_t, Tick, vector, Viewport (+22 more)

### Community 19 - "AudioStream"
Cohesion: 0.08
Nodes (33): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+25 more)

### Community 20 - "MidiEvent"
Cohesion: 0.06
Nodes (28): AddNoteCommand, channel_, execute, index_, note_, pattern_, size_t, size_t (+20 more)

### Community 21 - "AddInsertCommand"
Cohesion: 0.08
Nodes (24): AddInsertCommand, execute, minted_, mixerNode_, plugin_, slot_, undo, findNode() (+16 more)

### Community 22 - "atomic"
Cohesion: 0.08
Nodes (16): atomic, MidiBuffer, array, array, atomic, Capacity, size_t, T (+8 more)

### Community 23 - "ProcessContext"
Cohesion: 0.10
Nodes (26): dbToGain(), size_t, sumInputsInto(), coefficientFor(), process, size_t, process, process (+18 more)

### Community 24 - "INCDAW — Decision Log"
Cohesion: 0.06
Nodes (34): D-001 — Core implementation language: C++20, D-002 — Build system: CMake + Ninja, D-003 — Audio I/O: CoreAudio HAL directly, no wrapper framework, D-004 — Realtime thread scheduling: os_workgroup / Audio Workgroups, D-005 — Platform strategy: macOS first, Windows later, Linux not precluded, D-006 — UI: AppKit shell + INCDAW-owned Metal-rendered widget layer, D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded), D-008 — Licensing: INCDAW is closed-source (+26 more)

### Community 25 - "AddMidiMappingCommand"
Cohesion: 0.07
Nodes (25): AddMidiMappingCommand, controller_, execute, mapping_, midiChannel_, minted_, parameterKey_, target_ (+17 more)

### Community 26 - "Sampler"
Cohesion: 0.06
Nodes (27): array, atomic, maxVoices, ParameterSink, SampleRate, size_t, uint64_t, vector (+19 more)

### Community 27 - "GraphBuilder"
Cohesion: 0.09
Nodes (28): Connection, NodeIndex, SampleRate, size_t, unique_ptr, GraphBuilder, addNode, analyse (+20 more)

### Community 28 - "WavStreamReader"
Cohesion: 0.08
Nodes (29): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+21 more)

### Community 29 - "MixerTests.cpp"
Cohesion: 0.09
Nodes (21): FrameCount, FramePosition, Sample, SampleRate, size_t, vector, ImpulseNode, latency_ (+13 more)

### Community 30 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 31 - "TempoMap"
Cohesion: 0.11
Nodes (28): execute, execute, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition (+20 more)

### Community 32 - "SimpleSynth"
Cohesion: 0.08
Nodes (22): uint32_t, array, atomic, maxVoices, ParameterSink, Sample, SampleRate, uint64_t (+14 more)

### Community 33 - "EqEffect"
Cohesion: 0.09
Nodes (22): Coefficients, FrameCount, SampleRate, EqEffect, bandCount, cached_, coefficients_, maxChannels (+14 more)

### Community 34 - "2. INCDAW functional scope"
Cohesion: 0.07
Nodes (29): 1.1 Findings that changed INCDAW's architecture, 1.2 Supported plugin formats (official), 1.3 Other FL Studio 2026 features, recorded for completeness, 1. Functional reference: FL Studio 2026, 2. INCDAW functional scope, 3. Non-functional requirements, Audio editor, Audio engine (+21 more)

### Community 35 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 36 - "DelayEffect"
Cohesion: 0.08
Nodes (24): Allpass, Comb, FrameCount, SampleRate, DelayEffect, capacity_, lines_, maxChannels (+16 more)

### Community 37 - "AudioFileData"
Cohesion: 0.15
Nodes (25): applyGain(), applyRamp(), clampedRegion(), Sample, fadeIn(), fadeOut(), FrameCount, normalize() (+17 more)

### Community 38 - "AudioLogger"
Cohesion: 0.09
Nodes (22): AudioLogger, capacityFrames_, circle_, enabled_, grab, log, prepare, ready_ (+14 more)

### Community 39 - "TestLatencyPlugin.cpp"
Cohesion: 0.11
Nodes (25): clap_host_t, clap_plugin_descriptor_t, clap_plugin_factory_t, clap_plugin_t, clap_process_status, clap_process_t, vector, factoryCreatePlugin() (+17 more)

### Community 40 - "ClapInstance"
Cohesion: 0.08
Nodes (23): clap_plugin_gui_t, clap_plugin_state_t, ParamEvent, ClapInstance, closeEditor, editorOpen_, gui_, host_ (+15 more)

### Community 41 - "PluginInstanceManager"
Cohesion: 0.11
Nodes (26): Held, size_t, string, uint32_t, uint64_t, unique_ptr, vector, mutex (+18 more)

### Community 42 - "Json"
Cohesion: 0.08
Nodes (18): nullptr_t, int64_t, int64_t, pair, string, vector, Json, append (+10 more)

### Community 43 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 44 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 45 - "Json.cpp"
Cohesion: 0.19
Nodes (22): size_t, string, escapeInto(), formatDouble(), asString, contains, dump, dumpTo (+14 more)

### Community 46 - "RecordingSession"
Cohesion: 0.09
Nodes (20): path, Placement, string, vector, FrameCount, FramePosition, uint32_t, uint64_t (+12 more)

### Community 47 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 48 - "INCDAWMixerView"
Cohesion: 0.15
Nodes (25): incdaw, NSArray, NSDictionary, NSView, INCDAWMixerView, -acceptsFirstResponder, -addStripRect, -drawRect (+17 more)

### Community 49 - "MixerStripNode"
Cohesion: 0.13
Nodes (19): FrameCount, Sample, SampleRate, atomic, Sample, MixerStripNode, left_, meter_ (+11 more)

### Community 50 - "EditAssetRegionCommand"
Cohesion: 0.09
Nodes (21): AudioEditOp, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_, minted_ (+13 more)

### Community 51 - "Clip"
Cohesion: 0.08
Nodes (25): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+17 more)

### Community 52 - "GraphCompileOptions"
Cohesion: 0.08
Nodes (25): PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, insertFactory, instrumentFactory, masterGain, maxBlockSize (+17 more)

### Community 53 - "ConnectMixerCommand"
Cohesion: 0.08
Nodes (20): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+12 more)

### Community 54 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 55 - "CompiledProjectGraph"
Cohesion: 0.10
Nodes (24): CompiledProjectGraph, automation, channels, channelStripFor, channelStrips, error, graph, insertSlots (+16 more)

### Community 56 - "PlaylistView.mm"
Cohesion: 0.13
Nodes (24): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+16 more)

### Community 57 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

### Community 58 - "INCDAW — Roadmap"
Cohesion: 0.08
Nodes (23): Deliberately out of scope, INCDAW — Roadmap, Phase 0 — Research and architecture ✅ COMPLETE, Phase 10 — Mixer and routing, Phase 11 — Automation, Phase 12 — Recording and audio editor, Phase 13 — Plugin hosting, Phase 14 — Sampler (+15 more)

### Community 59 - "AudioEngine"
Cohesion: 0.09
Nodes (20): RetiredGraph, AudioCaptureSink, AudioEngine, active_, anchor_, anchorVersion_, blockCounter_, blockMidi_ (+12 more)

### Community 60 - ".buffer"
Cohesion: 0.12
Nodes (18): anyNonZero(), pair, ParameterSink, path, Sample, uint32_t, vector, ParameterFixture (+10 more)

### Community 61 - "Sampler.cpp"
Cohesion: 0.16
Nodes (22): FrameCount, Sample, SampleRate, vector, Voice, interpolate(), activeVoiceCount, allNotesOff (+14 more)

### Community 62 - "ConstantNode"
Cohesion: 0.10
Nodes (14): ConstantNode, latency_, value_, FrameCount, Sample, size_t, vector, OrderRecordingNode (+6 more)

### Community 63 - "read"
Cohesion: 0.16
Nodes (22): Format, path, Result, size_t, uint16_t, uint32_t, uint8_t, vector (+14 more)

### Community 64 - "AudioBufferView"
Cohesion: 0.16
Nodes (11): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t, process (+3 more)

### Community 65 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 66 - "CompressorEffect"
Cohesion: 0.11
Nodes (18): CompressorEffect, envelope_, prepare, reduction_, sampleRate_, FrameCount, SampleRate, GateEffect (+10 more)

### Community 67 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 68 - "ClapLibrary.cpp"
Cohesion: 0.13
Nodes (21): clap_event_header_t, blobRead(), size, blobWrite(), hasEditor, openEditor, process, setParameter (+13 more)

### Community 69 - "InsertRecordedTakeCommand"
Cohesion: 0.10
Nodes (16): Placement, size_t, string, vector, InsertRecordedTakeCommand, asset_, assetIndex_, clipIndices_ (+8 more)

### Community 70 - "BuiltinEffect"
Cohesion: 0.19
Nodes (18): appendF64(), appendU32(), BuiltinEffect, BuiltinEffect::BuiltinEffect(), loadState, saveState, setParameter, value (+10 more)

### Community 71 - "ParameterRegistry"
Cohesion: 0.18
Nodes (19): Applier, convertParameters(), Entry, size_t, string, uint32_t, vector, Entry (+11 more)

### Community 72 - "InstrumentNode"
Cohesion: 0.11
Nodes (16): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, ParameterSink, unique_ptr, InstrumentNode (+8 more)

### Community 73 - "PluginIdentifier"
Cohesion: 0.13
Nodes (15): builtinSampler(), builtinSimpleSynth(), Format, string, formatName(), Format, friend, string (+7 more)

### Community 74 - "PluginInsertTests.cpp"
Cohesion: 0.18
Nodes (13): anyNonZero(), ClipInsert, threshold_, FrameCount, Sample, vector, GainInsert, factor_ (+5 more)

### Community 75 - "PluginStateTests.cpp"
Cohesion: 0.12
Nodes (17): anyNonZero(), compileLoaded(), InsertFactory, path, Sample, uint8_t, vector, factoryFor() (+9 more)

### Community 76 - "SamplerStreamingTests.cpp"
Cohesion: 0.13
Nodes (18): FrameCount, MidiBuffer, path, Sample, shared_ptr, size_t, string, vector (+10 more)

### Community 77 - "AutomationTests.cpp"
Cohesion: 0.13
Nodes (14): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+6 more)

### Community 78 - "CommandRegistry.cpp"
Cohesion: 0.17
Nodes (19): clearHistory, execute, executeMerging, findAction, invoke, redo, redoName, registerAction (+11 more)

### Community 79 - "LoadSampleCommand"
Cohesion: 0.12
Nodes (15): size_t, string, vector, LoadSampleCommand, asset_, assetIndex_, channelId_, created_ (+7 more)

### Community 80 - "WaveformOverview"
Cohesion: 0.11
Nodes (17): Bucket, FrameCount, SampleRate, size_t, vector, WaveformOverview, channelCount, channels (+9 more)

### Community 81 - "AutomationNode"
Cohesion: 0.11
Nodes (11): AutomationNode, bindings_, tempoMap_, Binding, size_t, vector, controlChange(), path (+3 more)

### Community 82 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 83 - "4. Specialised tests"
Cohesion: 0.11
Nodes (18): 1. Principle, 2. Framework, 3. Test levels, 4. Specialised tests, 5. What is not tested automatically, End-to-end, Fuzzing (from Phase 4), Golden-file audio (from Phase 7) (+10 more)

### Community 84 - "SamplerZoneStream"
Cohesion: 0.14
Nodes (16): Slot, uint64_t, array, FrameCount, shared_ptr, size_t, SamplerZoneStream, claimSlot (+8 more)

### Community 85 - "AudioEngine.cpp"
Cohesion: 0.18
Nodes (17): audioDeviceAboutToStart, audioDeviceStopped, bufferSize, captureAudioBlock, collectRetiredGraphs, inputChannels, isRunning, maxServiceableBlockSize (+9 more)

### Community 86 - "DelayLineNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_, prepare (+6 more)

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

### Community 93 - "ClapLibrary"
Cohesion: 0.14
Nodes (12): clap_plugin_entry_t, ClapLibrary, close, descriptors, entry_, factory_, library_, open (+4 more)

### Community 94 - "write"
Cohesion: 0.19
Nodes (16): int32_t, AiffFile, write, appendBigU16(), appendBigU32(), appendExtended(), appendId(), Format (+8 more)

### Community 95 - "CoreMidiDevice"
Cohesion: 0.14
Nodes (15): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+7 more)

### Community 96 - "main.mm"
Cohesion: 0.17
Nodes (17): NSAlert, NSScrollView, NSSplitView, -applicationDidFinishLaunching, -editorChanged, NSView, -openAudioAssetInEditor, -selectChannel (+9 more)

### Community 97 - "AudioBufferPool"
Cohesion: 0.14
Nodes (11): AudioBufferPool, channelPointers_, reset, samples_, FrameCount, Sample, size_t, unique_ptr (+3 more)

### Community 98 - "Node"
Cohesion: 0.14
Nodes (9): FrameCount, SampleRate, Node, process, ParameterSink, StateIO, FrameCount, ImpulseNode (+1 more)

### Community 99 - "SimpleSynth.cpp"
Cohesion: 0.15
Nodes (17): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), polyBlep(), activeVoiceCount (+9 more)

### Community 100 - "SystemInfo"
Cohesion: 0.14
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 101 - "CountingCommand"
Cohesion: 0.12
Nodes (8): CountingCommand, counter_, delta_, string, Tick, makeProjectWithNotes(), NoOpCommand, note()

### Community 102 - "SamplerTests.cpp"
Cohesion: 0.18
Nodes (17): constantSample(), FrameCount, MidiBuffer, Sample, shared_ptr, vector, makeEnvelopeTransparent(), nyquistSample() (+9 more)

### Community 103 - "TimingProbeInstrument"
Cohesion: 0.14
Nodes (10): Applied, FrameCount, MidiBuffer, Sample, SampleRate, vector, renderSynth(), TimingProbeInstrument (+2 more)

### Community 104 - "ioProcTrampoline"
Cohesion: 0.21
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 105 - "MidiRecorder"
Cohesion: 0.14
Nodes (14): CapturedMessage, FramePosition, MidiBuffer, atomic, queueCapacity, size_t, uint64_t, MidiRecorder (+6 more)

### Community 106 - "INCDAW — Plugin Host"
Cohesion: 0.12
Nodes (16): 10. Testing, 1. Supported formats, 2. Prime directive, 3. Pipeline, 4. Isolation strategy, 5. Parameter system, 6. State, 7. Editor / UI bridge (+8 more)

### Community 107 - "AudioClipNode"
Cohesion: 0.13
Nodes (13): AudioClipNode, addClip, clips_, fetchScratch_, prepare, process, FrameCount, PlacedClip (+5 more)

### Community 108 - "SampleCache"
Cohesion: 0.15
Nodes (16): int64_t, path, shared_ptr, size_t, string, Entry, mutex, string (+8 more)

### Community 109 - "main"
Cohesion: 0.15
Nodes (16): availableDevices, deviceName, midiInput_, profiler_, sampleRate, setGraph, start, transport_ (+8 more)

### Community 110 - "SampleRingBuffer"
Cohesion: 0.21
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 111 - "LevelMeter"
Cohesion: 0.15
Nodes (12): atomic, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond, rmsWindowSeconds (+4 more)

### Community 112 - "Smoother"
Cohesion: 0.18
Nodes (9): atomic, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds, sampleRate_ (+1 more)

### Community 113 - "PluginParameterInfo"
Cohesion: 0.13
Nodes (11): ParameterSink, StateIO, string, uint32_t, PluginParameterInfo, defaultValue, id, maxValue (+3 more)

### Community 114 - "GainNode"
Cohesion: 0.15
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 115 - "SineOscillatorNode"
Cohesion: 0.13
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 116 - "NoteSequence"
Cohesion: 0.17
Nodes (13): Tick, vector, Tick, uint32_t, vector, NoteSequence, byEnd_, clear (+5 more)

### Community 117 - "EditFixture"
Cohesion: 0.15
Nodes (13): FrameCount, path, Sample, size_t, EditFixture, assetId, file, project (+5 more)

### Community 118 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 119 - "BuiltinEffectTests.cpp"
Cohesion: 0.19
Nodes (15): FrameCount, Sample, size_t, vector, processThrough(), RefAllpass, index, line (+7 more)

### Community 120 - "INCDAW — Architecture"
Cohesion: 0.12
Nodes (15): 1. Guiding principle, 2. Layer model, 3. Proposed repository structure, 4. Threading model, 5. Data model, 6. Command architecture, 7. Engine boundary, 8. Plugin isolation (+7 more)

### Community 121 - "PluginRegistry"
Cohesion: 0.22
Nodes (14): Library, Located, string, vector, field(), vector, PluginRegistry, clearBlacklist (+6 more)

### Community 122 - "AutomationWriteSession"
Cohesion: 0.15
Nodes (12): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, string, Tick (+4 more)

### Community 123 - "CommandRegistry"
Cohesion: 0.15
Nodes (9): CommandRegistry, actions_, project_, redoStack_, undoStack_, CommandPtr, Entry, size_t (+1 more)

### Community 124 - "WriteAutomationCommand"
Cohesion: 0.12
Nodes (15): WriteAutomationCommand, clip_, clipIndex_, key_, laneAfter_, laneCreated_, laneId_, laneIndex_ (+7 more)

### Community 125 - "allocate"
Cohesion: 0.14
Nodes (13): allocate, FrameCount, size_t, Sample, vector, render(), Sample, renderedPeak() (+5 more)

### Community 126 - "EffectParameter"
Cohesion: 0.12
Nodes (14): EffectParameter, defaultValue, id, maxValue, minValue, name, stepped, uint32_t (+6 more)

### Community 127 - "INCDAW — Performance Strategy"
Cohesion: 0.13
Nodes (14): 1. Reference machine, 2. Targets, 3. Instrumentation, 4. Method, 5. Known design-level performance decisions, 6. Profiling tooling, 7. Phase 18 — measured baseline and optimisations, Audio (+6 more)

### Community 128 - "AddAutomationLaneCommand"
Cohesion: 0.15
Nodes (8): AddAutomationLaneCommand, execute, index_, key_, lane_, minted_, target_, string

### Community 129 - "BuiltinEffectInfo"
Cohesion: 0.17
Nodes (14): BuiltinEffectInfo, displayName, parameterCount, parameters, uid, CatalogueEntry, info, make (+6 more)

### Community 130 - "Instrument"
Cohesion: 0.14
Nodes (10): MidiBuffer, ParameterSink, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare (+2 more)

### Community 131 - "INCDAWAppDelegate"
Cohesion: 0.14
Nodes (12): NSApplicationDelegate, NSObject, NSSegmentedControl, NSTextField, NSWindow, NSView, INCDAWChannelRackView, -initWithFrameprojectregistry (+4 more)

### Community 132 - "InputMonitorNode"
Cohesion: 0.15
Nodes (9): FrameCount, Sample, SampleRate, size_t, vector, InputMonitorNode, channelCount_, ring_ (+1 more)

### Community 133 - "SamplerWiringTests.cpp"
Cohesion: 0.19
Nodes (10): path, string, noteAtZero(), SamplerProject, asset, channel, project, ScratchDirectory (+2 more)

### Community 134 - "Options"
Cohesion: 0.14
Nodes (14): int64_t, Options, amplitude, buffer, device, frequency, input, listOnly (+6 more)

### Community 135 - "INCDAW — Audio Engine"
Cohesion: 0.15
Nodes (12): 10. Audio correctness requirements, 11. Performance budget, 1. The prime directive, 2. Device layer, 3. Realtime thread scheduling, 4. Realtime safety enforcement, 5. Signal flow, 6. Block processing and sample-accurate events (+4 more)

### Community 136 - "AudioEditCommands.cpp"
Cohesion: 0.32
Nodes (12): assetFilePath(), Sample, string, vector, execute, name, undo, findAsset() (+4 more)

### Community 137 - "ChannelRackView.mm"
Cohesion: 0.26
Nodes (12): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+4 more)

### Community 138 - "RecordingPlacementTests.cpp"
Cohesion: 0.21
Nodes (11): FrameCount, path, Sample, shared_ptr, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 139 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 140 - "KernelTable"
Cohesion: 0.21
Nodes (9): SampleRate, vector, KernelTable, phases, weights_, width, resample(), sinc() (+1 more)

### Community 141 - "Fixture"
Cohesion: 0.20
Nodes (10): Tick, vector, Fixture, channel, pattern, project, trackA, trackB (+2 more)

### Community 142 - "INCDAW — Project Format"
Cohesion: 0.18
Nodes (10): 1. Shape: a package directory, not a single file, 2. Versioning and migration, 3. Text vs binary, 4. Media: referenced or embedded, 5. Autosave, backup and recovery, 6. Archiving, 7. Determinism, 8. Tests (Phase 4 gate) (+2 more)

### Community 143 - "ChildResult"
Cohesion: 0.18
Nodes (10): End, ChildResult, code, end, output, path, string, vector (+2 more)

### Community 144 - "AutomationCommands.cpp"
Cohesion: 0.24
Nodes (10): undo, AutomationPoint, vector, findLane(), canMergeWith, execute, undo, sortPoints() (+2 more)

### Community 145 - "AddPatternClipCommand"
Cohesion: 0.18
Nodes (9): AddPatternClipCommand, clip_, index_, length_, minted_, pattern_, start_, track_ (+1 more)

### Community 146 - "captureAudioBlock"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 147 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 148 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 149 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 150 - "SharedLibrary"
Cohesion: 0.25
Nodes (7): path, string, SharedLibrary, close, handle_, open, symbol

### Community 151 - "BlobReader"
Cohesion: 0.22
Nodes (11): BlobReader, cursor, data, BlobWriter, out, overflowed, loadState, saveState (+3 more)

### Community 152 - "PluginNode"
Cohesion: 0.18
Nodes (5): FrameCount, ParameterSink, StateIO, PluginNode, instance_

### Community 153 - "humanizeNoteStarts"
Cohesion: 0.29
Nodes (10): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+2 more)

### Community 154 - "AutomationProbe"
Cohesion: 0.22
Nodes (8): tempoMap_, AutomationProbe, calls, registry, written, FramePosition, vector, makeProject()

### Community 155 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 156 - "SetAutomationPointsCommand"
Cohesion: 0.27
Nodes (7): AutomationPoint, vector, SetAutomationPointsCommand, laneId_, mergeWith, points_, previous_

### Community 157 - "ClipCommands.cpp"
Cohesion: 0.27
Nodes (8): execute, undo, execute, canMergeWith, mergeWith, undo, canMergeWith, trackAtOffset()

### Community 158 - "build"
Cohesion: 0.27
Nodes (9): bucketize(), Bucket, FrameCount, path, Result, Sample, vector, sizeBuckets() (+1 more)

### Community 159 - "TimestampedMidiMessage"
Cohesion: 0.22
Nodes (9): midiMessageReceived, sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos (+1 more)

### Community 160 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 161 - "capturePluginState"
Cohesion: 0.51
Nodes (9): capturePluginState(), path, string, uint8_t, vector, readBlobFile(), restorePluginState(), stateFileNameFor() (+1 more)

### Community 162 - "Harness"
Cohesion: 0.20
Nodes (7): path, string, Harness, folder, registry, ScratchDir, path

### Community 163 - "PluginFolder"
Cohesion: 0.24
Nodes (7): path, PluginFolder, crash, dir, gain, ScratchDir, path

### Community 164 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 165 - "setParameter"
Cohesion: 0.22
Nodes (3): FilterMode, uint32_t, setParameter

### Community 166 - "MoveClipsCommand"
Cohesion: 0.22
Nodes (8): MovedAudioClip, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, movedAudio_, tickDelta_, trackDelta_

### Community 167 - "RemoveAutomationLaneCommand"
Cohesion: 0.22
Nodes (7): size_t, RemoveAutomationLaneCommand, execute, index_, lane_, laneId_, undo

### Community 168 - "RemoveClipsCommand"
Cohesion: 0.20
Nodes (9): string, RemovedClip, vector, RemoveClipsCommand, clips_, execute, name, removed_ (+1 more)

### Community 169 - "DuplicateClipsCommand"
Cohesion: 0.22
Nodes (8): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_, undo

### Community 170 - "ResizeClipsCommand"
Cohesion: 0.22
Nodes (8): FrameCount, ResizeClipsCommand, clips_, lengthDelta_, mergeWith, previousFrameLengths_, previousLengths_, undo

### Community 171 - "MidiMapNode"
Cohesion: 0.25
Nodes (5): Binding, size_t, vector, MidiMapNode, bindings_

### Community 172 - "exportArrangement"
Cohesion: 0.25
Nodes (7): size_t, notes_, path, Result, uint64_t, exportArrangement(), importAsPattern()

### Community 173 - "AutomationFixture"
Cohesion: 0.22
Nodes (6): AutomationFixture, channel, pattern, project, tempo, FramePosition

### Community 174 - "create"
Cohesion: 0.25
Nodes (8): clap_event_param_value_t, create, array, string, unique_ptr, PendingParamEvents, count, events

### Community 176 - "Denormals.h"
Cohesion: 0.39
Nodes (5): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister()

### Community 177 - "SetClipMutedCommand"
Cohesion: 0.29
Nodes (6): SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 178 - "updatedRecents"
Cohesion: 0.32
Nodes (7): autosaveIsNewer(), autosavePathFor(), path, size_t, string, vector, updatedRecents()

### Community 179 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 180 - "MidiImportResult"
Cohesion: 0.25
Nodes (7): string, vector, MidiImportResult, error, newChannels, pattern, succeeded

### Community 181 - "Fixture"
Cohesion: 0.25
Nodes (6): Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 182 - "renderArrangement"
Cohesion: 0.39
Nodes (7): FrameCount, Sample, size_t, vector, makeAudio(), renderArrangement(), tone()

### Community 183 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped()

### Community 184 - "ScriptedFactory"
Cohesion: 0.25
Nodes (7): function, InsertFactory, unique_ptr, ScriptedFactory, fail, makers, requests

### Community 185 - "RenderTests.cpp"
Cohesion: 0.25
Nodes (5): path, string, makeArrangedProject(), ScratchDirectory, path

### Community 186 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 187 - "INCDAW — Release Guide"
Cohesion: 0.29
Nodes (6): 1. What a release is, 2. Cutting a release, 3. Installing (first launch on another Mac), 4. Updating, 5. Release notes — 0.9.0 (2026-08-16), INCDAW — Release Guide

### Community 188 - "AnalyzerEffect"
Cohesion: 0.38
Nodes (4): AnalyzerEffect, maxChannels, atomic, size_t

### Community 189 - "collectForBlock"
Cohesion: 0.29
Nodes (6): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock, resetCounters

### Community 190 - "SequencedNote"
Cohesion: 0.29
Nodes (6): SequencedNote, channel, key, lengthTicks, startTick, velocity

### Community 191 - "scanDirectory"
Cohesion: 0.38
Nodes (7): int64_t, path, size_t, uint64_t, mtimeSecondsOf(), scanDirectory, sizeOf()

### Community 192 - "ScanOutcome"
Cohesion: 0.29
Nodes (7): string, vector, ScanOutcome, detail, plugins, status, Status

### Community 193 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 194 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 195 - "StressTests.cpp"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 196 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 198 - "collectForRange"
Cohesion: 0.33
Nodes (4): FrameCount, FramePosition, MidiBuffer, collectForRange

### Community 199 - "ClapDescriptor"
Cohesion: 0.33
Nodes (6): ClapDescriptor, id, name, vendor, version, string

### Community 200 - "DitherSource"
Cohesion: 0.47
Nodes (3): uint64_t, DitherSource, state_

### Community 201 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 202 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 203 - "v1.2/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 204 - "v1.3/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 205 - "v1.4/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 206 - "renderBlock"
Cohesion: 0.47
Nodes (5): FrameCount, Sample, vector, renderBlock(), tone()

### Community 207 - "PatternTests.cpp"
Cohesion: 0.53
Nodes (5): Tick, vector, note(), shapeOf(), startsOf()

### Community 208 - "InsertFixture"
Cohesion: 0.33
Nodes (4): InsertFixture, pattern, project, tempo

### Community 209 - "SessionFixture"
Cohesion: 0.40
Nodes (4): path, string, SessionFixture, root

### Community 210 - "StateIO"
Cohesion: 0.40
Nodes (3): StateIO, loadState, saveState

### Community 211 - "MidiDeviceInfo"
Cohesion: 0.40
Nodes (5): string, MidiDeviceInfo, identifier, isInput, name

### Community 212 - "scanOutOfProcess"
Cohesion: 0.60
Nodes (4): path, string, parseLine(), scanOutOfProcess()

### Community 213 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 214 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 215 - "Fixture"
Cohesion: 0.40
Nodes (3): Fixture, project, registry

### Community 216 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 220 - "ScratchDir"
Cohesion: 0.50
Nodes (3): path, ScratchDir, path

### Community 221 - "ScratchDir"
Cohesion: 0.50
Nodes (3): path, ScratchDir, path

### Community 222 - "ScratchDir"
Cohesion: 0.50
Nodes (3): path, ScratchDir, path

### Community 223 - "gainBlob"
Cohesion: 0.67
Nodes (3): uint8_t, vector, gainBlob()

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
- **1338 isolated node(s):** `index_`, `minted_`, `node_`, `type_`, `connection_` (+1333 more)
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
- **Why does `EntityId` connect `Project` to `AddAutomationLaneCommand`, `MixerCommands.cpp`, `Channel`, `Command`, `NoteCommands.cpp`, `Track`, `ChannelCommands.cpp`, `AudioEditCommands.cpp`, `SamplerWiringTests.cpp`, `PianoRollModel`, `Pattern`, `Fixture`, `AutomationCommands.cpp`, `AddPatternClipCommand`, `PlaylistModel`, `RenderOptions`, `MidiEvent`, `AddInsertCommand`, `AddMidiMappingCommand`, `SetAutomationPointsCommand`, `ClipCommands.cpp`, `MixerTests.cpp`, `AudioFileData`, `RemoveAutomationLaneCommand`, `AutomationFixture`, `ClipIds`, `EditAssetRegionCommand`, `Clip`, `MidiImportResult`, `ConnectMixerCommand`, `GraphCompileOptions`, `CompiledProjectGraph`, `Fixture`, `.buffer`, `InsertRecordedTakeCommand`, `InstrumentNode`, `PluginIdentifier`, `PluginStateTests.cpp`, `LoadSampleCommand`, `InsertFixture`, `load`, `CountingCommand`, `EditFixture`, `AutomationWriteSession`, `WriteAutomationCommand`?**
  _High betweenness centrality (0.099) - this node is a cross-community bridge._