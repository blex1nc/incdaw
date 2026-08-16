# Graph Report - fl-studio-2026-features-96da39  (2026-08-16)

## Corpus Check
- 312 files · ~235,357 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 4946 nodes · 8726 edges · 261 communities (256 shown, 5 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 429 edges (avg confidence: 0.81)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `831f0e7b`
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
- EntityId
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
- ArpeggiateNotesCommand
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
- PluginPersistenceTests.cpp
- RealtimeGuard.cpp
- AutomationWriteSession
- Region
- BasicMidiBuffer
- SystemInfo
- EditFixture
- ParsedHeader
- SamplerZone
- AudioClipNode
- MixerCommands.cpp
- AudioBufferView
- Channel
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
- AddMarkerCommand
- GainNode
- SineOscillatorNode
- PluginInsertTests.cpp
- 4. Specialised tests
- TimingProbeInstrument
- PluginInstanceManager
- INCDAW — Plugin Host
- AddChannelCommand
- INCDAW — Architecture
- ProcessContext
- DuplicateClipsCommand
- MoveClipsCommand
- PlaylistView.mm
- EffectParameter
- Instrument
- INCDAW — Audio Engine
- renderNode
- Project
- CoreMidiDevice.cpp
- Options
- ParameterRegistry
- ChannelRackView.mm
- NudgeChordCommand
- vector
- AutomationPoint
- AutomationNode
- AddPatternClipCommand
- MidiEvent
- LoudnessMeterEffect
- BlobReader
- RenderOptions
- main.mm
- FlangerEffect
- NoteCommands.cpp
- ToggleStepCommand
- INCDAW — Performance Strategy
- INCDAW — Project Format
- RealtimeSafetyTests.cpp
- AudioEngine.h
- MidiTests.cpp
- string
- AutomationProbe
- CountingCommand
- PluginFolder
- FuzzTests.cpp
- BuiltinEffectInfo
- MidiDevice
- CommandRegistry.cpp
- PatternListView.mm
- allocate
- Smoother
- DelayLineNode
- SharedLibrary
- write
- SamplerStreamingTests.cpp
- ChildResult
- RemoveClipsCommand
- make-dmg.sh
- renderClickFrames
- MusicTheory.cpp
- AudioFileData
- Json.cpp
- ClipCommands.cpp
- ClapInstance
- ConstantNode
- ResizeClipsCommand
- Command
- Parser
- SampleCache
- RecordedEvent
- PluginNode
- RemoveMixerNodeCommand
- capturePluginState
- SendFixture
- Version
- TimestampedMidiMessage
- BuiltinEffect
- makeTestSignal
- ClapLibrary
- SidechainFixture
- TimeSignatureEvent
- ScratchDirectory
- MidiMapNode
- INCDAWMixerView
- Node
- ConnectMixerCommand
- ParameterSink
- v1.4/Fixture.incdaw/manifest.json
- ClapLibrary.cpp
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- MidiDeviceInfo
- TrackCommands.cpp
- AnalyzerEffect
- INCDAWPlaylistView
- ModulationEffects.cpp
- check
- renderArrangement
- AudioCaptureSink
- ChannelCommands.cpp
- BuiltinEffectTests.cpp
- PluginStateTests.cpp
- SetSendGainCommand
- timeStretch
- setParameter
- humanizeNoteStarts
- Fixture
- AutomationFixture
- read
- SimpleSynth.cpp
- ClipIds
- collectForBlock
- create
- RemoveChannelCommand
- INCDAW — Release Guide
- Fixture
- RoutingConnection
- v1.2/Fixture.incdaw/manifest.json
- v1.3/Fixture.incdaw/manifest.json
- RenderResult
- processThrough
- Fixture
- ClapDescriptor
- Fixture
- SplitFixture
- SetClipMutedCommand
- StressTests.cpp
- SplitClipCommand
- exportArrangement
- StretchClipsCommand
- string
- SetChannelStepKeyCommand
- Fixture
- SetChannelOutputCommand
- MidiImportResult
- RemoveTrackCommand
- RenderTests.cpp
- SequencedNote
- ScratchDirectory
- INCDAWPianoRollView
- trimToMaximumDepth
- SetChannelMutedCommand
- v1.5/Fixture.incdaw/manifest.json
- renderProject
- renderBlock
- ScratchDirectory
- SetChannelSoloedCommand
- SetMixerPolarityCommand
- INCDAW — FL Studio 2026 Gap Analysis
- AddTrackCommand
- KernelTable
- SetTrackMutedCommand
- INCDAWAudioEditorView
- SetTrackSoloedCommand
- sampleWaveform
- openEditor
- RenameChannelCommand
- emptyOutTryPush
- setParameter
- collectForRange
- PatternTests.cpp
- RecordingSink
- ScratchDirectory
- arrangementEndFrames
- ScratchDir
- .size

## God Nodes (most connected - your core abstractions)
1. `Project` - 260 edges
2. `EntityId` - 230 edges
3. `Command` - 135 edges
4. `TempoMap` - 75 edges
5. `AudioEngine` - 68 edges
6. `Sampler` - 61 edges
7. `CoreAudioDevice` - 59 edges
8. `AudioFileData` - 57 edges
9. `Node` - 54 edges
10. `AudioBufferPool` - 53 edges

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

## Communities (261 total, 5 thin omitted)

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
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, uint32_t (+17 more)

### Community 4 - "WavStreamWriter"
Cohesion: 0.05
Nodes (59): ofstream, appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample (+51 more)

### Community 5 - "PluginRegistry"
Cohesion: 0.15
Nodes (21): Library, Located, int64_t, path, size_t, string, uint64_t, vector (+13 more)

### Community 6 - "AudioBufferPool"
Cohesion: 0.11
Nodes (16): AudioBufferPool, channelPointers_, reset, samples_, FrameCount, Sample, size_t, unique_ptr (+8 more)

### Community 7 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 8 - "PlaylistModel"
Cohesion: 0.10
Nodes (30): Rect, size_t, Tick, vector, size_t, Tick, vector, Viewport (+22 more)

### Community 9 - "SimpleSynth"
Cohesion: 0.08
Nodes (18): array, atomic, maxVoices, ParameterSink, SampleRate, uint64_t, Voice, SimpleSynth (+10 more)

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
Cohesion: 0.13
Nodes (11): CommandRegistry, actions_, project_, redo, redoStack_, undo, undoStack_, CommandPtr (+3 more)

### Community 14 - "AudioEngine"
Cohesion: 0.07
Nodes (50): RetiredGraph, AudioEngine, active_, anchor_, anchorVersion_, audioDeviceAboutToStart, audioDeviceStopped, availableDevices (+42 more)

### Community 15 - "TestLatencyPlugin.cpp"
Cohesion: 0.11
Nodes (25): clap_host_t, clap_plugin_descriptor_t, clap_plugin_factory_t, clap_plugin_t, clap_process_status, clap_process_t, vector, factoryCreatePlugin() (+17 more)

### Community 16 - "EditAssetRegionCommand"
Cohesion: 0.06
Nodes (31): AudioEditOp, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_, minted_ (+23 more)

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
Nodes (26): clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate, Segment (+18 more)

### Community 21 - "InstrumentNode"
Cohesion: 0.11
Nodes (16): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, ParameterSink, unique_ptr, InstrumentNode (+8 more)

### Community 22 - "EntityId"
Cohesion: 0.10
Nodes (31): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, size_t, vector (+23 more)

### Community 23 - "WavStreamReader"
Cohesion: 0.07
Nodes (29): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+21 more)

### Community 24 - "Sampler"
Cohesion: 0.06
Nodes (27): array, atomic, maxVoices, ParameterSink, SampleRate, size_t, uint64_t, vector (+19 more)

### Community 25 - "MixerStripNode"
Cohesion: 0.10
Nodes (23): FrameCount, Sample, SampleRate, atomic, Sample, MixerStripNode, left_, meter_ (+15 more)

### Community 26 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 27 - "CoreMidiDevice"
Cohesion: 0.15
Nodes (14): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+6 more)

### Community 28 - "DelayEffect"
Cohesion: 0.08
Nodes (24): Allpass, Comb, FrameCount, SampleRate, DelayEffect, capacity_, lines_, maxChannels (+16 more)

### Community 29 - "AudioStream"
Cohesion: 0.07
Nodes (34): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+26 more)

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
Cohesion: 0.04
Nodes (50): ClipType, AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id (+42 more)

### Community 34 - "atomic"
Cohesion: 0.20
Nodes (3): array, atomic, MidiBuffer

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

### Community 39 - "ArpeggiateNotesCommand"
Cohesion: 0.07
Nodes (31): Direction, ArpeggiateNotesCommand, channel_, direction_, indices_, pattern_, previousEvents_, step_ (+23 more)

### Community 40 - "TestGainPlugin.cpp"
Cohesion: 0.06
Nodes (57): clap_gui_resize_hints_t, clap_id, clap_param_info_t, clap_window_t, applyParamEvents(), clap_host_t, clap_input_events_t, clap_istream_t (+49 more)

### Community 41 - "GraphCompileOptions"
Cohesion: 0.08
Nodes (25): PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, insertFactory, instrumentFactory, masterGain, maxBlockSize (+17 more)

### Community 42 - "LoadSampleCommand"
Cohesion: 0.11
Nodes (15): size_t, string, vector, LoadSampleCommand, asset_, assetIndex_, channelId_, created_ (+7 more)

### Community 43 - "CallbackProfiler"
Cohesion: 0.12
Nodes (14): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+6 more)

### Community 44 - "MusicalPosition"
Cohesion: 0.11
Nodes (19): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+11 more)

### Community 45 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 46 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 47 - "WaveformOverview"
Cohesion: 0.08
Nodes (26): bucketize(), Bucket, FrameCount, path, Result, Sample, vector, Bucket (+18 more)

### Community 48 - "WriteAutomationCommand"
Cohesion: 0.05
Nodes (47): AddAutomationLaneCommand, execute, index_, key_, lane_, minted_, target_, undo (+39 more)

### Community 49 - "Sampler.cpp"
Cohesion: 0.16
Nodes (22): FrameCount, Sample, SampleRate, vector, Voice, interpolate(), activeVoiceCount, allNotesOff (+14 more)

### Community 50 - "CompressorEffect"
Cohesion: 0.07
Nodes (26): CompressorEffect, envelope_, noKeyInput, prepare, reduction_, sampleRate_, FrameCount, SampleRate (+18 more)

### Community 51 - "SamplerZoneStream"
Cohesion: 0.14
Nodes (16): Slot, uint64_t, array, FrameCount, shared_ptr, size_t, SamplerZoneStream, claimSlot (+8 more)

### Community 52 - "GraphBuilder"
Cohesion: 0.09
Nodes (29): Connection, process, FrameCount, FramePosition, MidiBuffer, NodeIndex, SampleRate, size_t (+21 more)

### Community 53 - "LevelMeter"
Cohesion: 0.15
Nodes (12): atomic, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond, rmsWindowSeconds (+4 more)

### Community 54 - "load"
Cohesion: 0.20
Nodes (21): append, automationPointFrom(), bindUnassignedContent(), AutomationPoint, path, Result, string, idFrom() (+13 more)

### Community 55 - "CompiledGraph"
Cohesion: 0.13
Nodes (13): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+5 more)

### Community 56 - "PluginPersistenceTests.cpp"
Cohesion: 0.17
Nodes (10): path, uint8_t, vector, gainBlob(), Harness, folder, registry, processOnes() (+2 more)

### Community 57 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 58 - "AutomationWriteSession"
Cohesion: 0.14
Nodes (12): AutomationWriteSession, capture, enabled_, finish, streams_, AutomationPoint, string, Tick (+4 more)

### Community 59 - "Region"
Cohesion: 0.27
Nodes (15): applyGain(), applyRamp(), clampedRegion(), Sample, fadeIn(), fadeOut(), FrameCount, normalize() (+7 more)

### Community 60 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 61 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 62 - "EditFixture"
Cohesion: 0.15
Nodes (13): FrameCount, path, Sample, size_t, EditFixture, assetId, file, project (+5 more)

### Community 63 - "ParsedHeader"
Cohesion: 0.18
Nodes (17): Result, size_t, uint16_t, uint32_t, uint8_t, vector, fillMetadata(), loadAndParse() (+9 more)

### Community 64 - "SamplerZone"
Cohesion: 0.11
Nodes (18): FrameCount, shared_ptr, handleMessage, SamplerZone, end, gain, keyHigh, keyLow (+10 more)

### Community 65 - "AudioClipNode"
Cohesion: 0.13
Nodes (13): AudioClipNode, addClip, clips_, fetchScratch_, prepare, process, FrameCount, PlacedClip (+5 more)

### Community 66 - "MixerCommands.cpp"
Cohesion: 0.07
Nodes (26): SetMixerMutedCommand, execute, muted_, nodeId_, undo, SetMixerSoloedCommand, execute, nodeId_ (+18 more)

### Community 67 - "AudioBufferView"
Cohesion: 0.19
Nodes (8): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t, runKeyed()

### Community 68 - "Channel"
Cohesion: 0.03
Nodes (73): AutomationCurve, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint, curve (+65 more)

### Community 69 - "Pattern"
Cohesion: 0.09
Nodes (38): Emit, size_t, Tick, vector, noteAtStep(), execute, undo, Tick (+30 more)

### Community 70 - "PluginIdentifier"
Cohesion: 0.13
Nodes (14): builtinSampler(), builtinSimpleSynth(), Format, string, formatName(), Format, friend, string (+6 more)

### Community 71 - "read"
Cohesion: 0.09
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
Cohesion: 0.09
Nodes (19): FrameCount, Sample, SampleRate, size_t, vector, InputMonitorNode, channelCount_, ring_ (+11 more)

### Community 79 - "[0.9.0] — 2026-08-16 — the core is complete"
Cohesion: 0.04
Nodes (44): [0.9.0] — 2026-08-16 — the core is complete, INCDAW — Changelog, Phase 0 — Research and architecture — 2026-08-14, Phase 10 — Mixer, routing and delay compensation — 2026-08-14, Phase 11a — Automation: the generic subsystem — 2026-08-15, Phase 11b — Automation placement and recording — 2026-08-15, Phase 12 (part 1) — WAV codec — 2026-08-15, Phase 12 (part 2) — Input capture and recording — 2026-08-15 (+36 more)

### Community 80 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 81 - "MixerTests.cpp"
Cohesion: 0.09
Nodes (21): FrameCount, FramePosition, Sample, SampleRate, size_t, vector, ImpulseNode, latency_ (+13 more)

### Community 82 - "ioProcTrampoline"
Cohesion: 0.23
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

### Community 86 - "AddMarkerCommand"
Cohesion: 0.07
Nodes (25): AddMarkerCommand, execute, index_, length_, marker_, minted_, tick_, undo (+17 more)

### Community 87 - "GainNode"
Cohesion: 0.15
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 88 - "SineOscillatorNode"
Cohesion: 0.13
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 89 - "PluginInsertTests.cpp"
Cohesion: 0.08
Nodes (27): anyNonZero(), ClipInsert, threshold_, FrameCount, function, InsertFactory, path, Sample (+19 more)

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

### Community 94 - "AddChannelCommand"
Cohesion: 0.15
Nodes (7): AddChannelCommand, channel_, index_, minted_, undo, size_t, string

### Community 95 - "INCDAW — Architecture"
Cohesion: 0.12
Nodes (15): 1. Guiding principle, 2. Layer model, 3. Proposed repository structure, 4. Threading model, 5. Data model, 6. Command architecture, 7. Engine boundary, 8. Plugin isolation (+7 more)

### Community 96 - "ProcessContext"
Cohesion: 0.14
Nodes (29): dbToGain(), sumInputsInto(), coefficientFor(), process, size_t, process, process, linkedPeakAt() (+21 more)

### Community 97 - "DuplicateClipsCommand"
Cohesion: 0.22
Nodes (8): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_, undo

### Community 98 - "MoveClipsCommand"
Cohesion: 0.18
Nodes (10): MovedAudioClip, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, canMergeWith, clips_, mergeWith, movedAudio_ (+2 more)

### Community 99 - "PlaylistView.mm"
Cohesion: 0.14
Nodes (21): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+13 more)

### Community 100 - "EffectParameter"
Cohesion: 0.12
Nodes (14): EffectParameter, defaultValue, id, maxValue, minValue, name, stepped, uint32_t (+6 more)

### Community 101 - "Instrument"
Cohesion: 0.14
Nodes (10): MidiBuffer, ParameterSink, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare (+2 more)

### Community 102 - "INCDAW — Audio Engine"
Cohesion: 0.15
Nodes (12): 10. Audio correctness requirements, 11. Performance budget, 1. The prime directive, 2. Device layer, 3. Realtime thread scheduling, 4. Realtime safety enforcement, 5. Signal flow, 6. Block processing and sample-accurate events (+4 more)

### Community 103 - "renderNode"
Cohesion: 0.21
Nodes (11): FrameCount, path, Sample, shared_ptr, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 104 - "Project"
Cohesion: 0.09
Nodes (39): map, execute, undo, chordGroups(), NoteIndices, Tick, vector, findEvents() (+31 more)

### Community 105 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 106 - "Options"
Cohesion: 0.12
Nodes (17): int64_t, string, Options, amplitude, buffer, device, frequency, input (+9 more)

### Community 107 - "ParameterRegistry"
Cohesion: 0.21
Nodes (18): Applier, convertParameters(), Entry, size_t, string, uint32_t, vector, Entry (+10 more)

### Community 108 - "ChannelRackView.mm"
Cohesion: 0.26
Nodes (12): -acceptsFirstResponder, -channelCount, -currentPattern, -drawRect, -hitForEvent, -initWithFrameprojectregistry, -isFlipped, -mouseDown (+4 more)

### Community 109 - "NudgeChordCommand"
Cohesion: 0.09
Nodes (26): vector, findEvents(), Scale, size_t, string, vector, InsertNotesCommand, channel_ (+18 more)

### Community 110 - "vector"
Cohesion: 0.08
Nodes (18): vector, string, unordered_map, mutex, ParameterSink, StateIO, StateIO, loadState (+10 more)

### Community 111 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 112 - "AutomationNode"
Cohesion: 0.11
Nodes (11): AutomationNode, bindings_, tempoMap_, Binding, size_t, vector, controlChange(), path (+3 more)

### Community 113 - "AddPatternClipCommand"
Cohesion: 0.15
Nodes (11): AddPatternClipCommand, clip_, execute, index_, length_, minted_, pattern_, start_ (+3 more)

### Community 114 - "MidiEvent"
Cohesion: 0.05
Nodes (36): AddNoteCommand, channel_, execute, index_, note_, pattern_, size_t, FrameCount (+28 more)

### Community 115 - "LoudnessMeterEffect"
Cohesion: 0.06
Nodes (31): Biquad, FrameCount, SampleRate, FrameCount, SampleRate, uint64_t, LoudnessMeterEffect, completeHop (+23 more)

### Community 116 - "BlobReader"
Cohesion: 0.22
Nodes (11): BlobReader, cursor, data, BlobWriter, out, overflowed, loadState, saveState (+3 more)

### Community 117 - "RenderOptions"
Cohesion: 0.11
Nodes (19): BitDepth, FramePosition, SampleRate, uint64_t, RenderOptions, bitDepth, blockSize, dither (+11 more)

### Community 118 - "main.mm"
Cohesion: 0.29
Nodes (10): -editorChanged, -openAudioAssetInEditor, -showAudioEditor, -showEditorAtSegment, -showMixer, -showPianoRoll, -showPlaylist, -togglePlayback (+2 more)

### Community 119 - "FlangerEffect"
Cohesion: 0.07
Nodes (30): ChorusEffect, centreMs, line_, mask_, maxChannels, maxDepthMs, phase_, sampleRate_ (+22 more)

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

### Community 125 - "AudioEngine.h"
Cohesion: 0.18
Nodes (7): AudioCaptureSink, FramePosition, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 126 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped()

### Community 127 - "string"
Cohesion: 0.09
Nodes (14): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType (+6 more)

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
Cohesion: 0.16
Nodes (14): BuiltinEffectInfo, displayName, parameterCount, parameters, uid, CatalogueEntry, info, make (+6 more)

### Community 133 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 134 - "CommandRegistry.cpp"
Cohesion: 0.30
Nodes (11): clearHistory, findAction, invoke, redoName, registerAction, search, undoName, Entry (+3 more)

### Community 135 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 136 - "allocate"
Cohesion: 0.11
Nodes (21): allocate, FrameCount, size_t, FramePosition, Sample, vector, render(), anyNonZero() (+13 more)

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
Cohesion: 0.14
Nodes (16): FrameCount, path, Sample, shared_ptr, size_t, string, vector, openZoneStream() (+8 more)

### Community 142 - "ChildResult"
Cohesion: 0.17
Nodes (10): End, ChildResult, code, end, output, path, string, vector (+2 more)

### Community 143 - "RemoveClipsCommand"
Cohesion: 0.20
Nodes (9): string, RemovedClip, vector, RemoveClipsCommand, clips_, execute, name, removed_ (+1 more)

### Community 144 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 145 - "renderClickFrames"
Cohesion: 0.15
Nodes (12): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, FramePosition (+4 more)

### Community 146 - "MusicTheory.cpp"
Cohesion: 0.12
Nodes (31): ChordDetection, bassKey, display, inverted, matched, rootPitchClass, type, ChordType (+23 more)

### Community 147 - "AudioFileData"
Cohesion: 0.09
Nodes (21): FrameCount, vector, detectOnsets(), AudioFileData, channelCount, channels, frameCount, sampleRate (+13 more)

### Community 148 - "Json.cpp"
Cohesion: 0.22
Nodes (13): int64_t, size_t, string, escapeInto(), formatDouble(), asBool, asInt, asString (+5 more)

### Community 149 - "ClipCommands.cpp"
Cohesion: 0.38
Nodes (8): execute, execute, undo, execute, execute, execute, trackAtOffset(), frameForTick

### Community 150 - "ClapInstance"
Cohesion: 0.09
Nodes (21): clap_plugin_gui_t, clap_plugin_state_t, ParamEvent, ClapInstance, editorOpen_, gui_, host_, latency_ (+13 more)

### Community 151 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 152 - "ResizeClipsCommand"
Cohesion: 0.20
Nodes (9): FrameCount, ResizeClipsCommand, canMergeWith, clips_, lengthDelta_, mergeWith, previousFrameLengths_, previousLengths_ (+1 more)

### Community 153 - "Command"
Cohesion: 0.12
Nodes (13): Command, execute, id, name, undo, SetMixerPanCommand, canMergeWith, execute (+5 more)

### Community 154 - "Parser"
Cohesion: 0.30
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 155 - "SampleCache"
Cohesion: 0.08
Nodes (26): int64_t, path, shared_ptr, size_t, string, Entry, mutex, string (+18 more)

### Community 156 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 157 - "PluginNode"
Cohesion: 0.18
Nodes (5): FrameCount, ParameterSink, StateIO, PluginNode, instance_

### Community 158 - "RemoveMixerNodeCommand"
Cohesion: 0.09
Nodes (17): RemovedRouting, DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t (+9 more)

### Community 159 - "capturePluginState"
Cohesion: 0.51
Nodes (9): capturePluginState(), path, string, uint8_t, vector, readBlobFile(), restorePluginState(), stateFileNameFor() (+1 more)

### Community 160 - "SendFixture"
Cohesion: 0.14
Nodes (11): ConstantSourceInsert, level_, FrameCount, pair, feedSine(), SendFixture, project, registry (+3 more)

### Community 161 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 162 - "TimestampedMidiMessage"
Cohesion: 0.22
Nodes (9): midiMessageReceived, sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos (+1 more)

### Community 163 - "BuiltinEffect"
Cohesion: 0.18
Nodes (19): appendF64(), appendU32(), BuiltinEffect, BuiltinEffect::BuiltinEffect(), loadState, saveState, setParameter, value (+11 more)

### Community 164 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 165 - "ClapLibrary"
Cohesion: 0.13
Nodes (11): clap_plugin_entry_t, ClapLibrary, descriptors, entry_, factory_, library_, clap_plugin_factory_t, path (+3 more)

### Community 166 - "SidechainFixture"
Cohesion: 0.13
Nodes (9): ConstantSourceInsert, level_, SidechainFixture, bassStrip, compressorSlot, kickStrip, project, registry (+1 more)

### Community 167 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 168 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 169 - "MidiMapNode"
Cohesion: 0.25
Nodes (5): Binding, size_t, vector, MidiMapNode, bindings_

### Community 170 - "INCDAWMixerView"
Cohesion: 0.14
Nodes (25): incdaw, NSArray, NSDictionary, NSView, INCDAWMixerView, -acceptsFirstResponder, -addStripRect, -drawRect (+17 more)

### Community 171 - "Node"
Cohesion: 0.10
Nodes (13): FrameCount, SampleRate, Node, process, ParameterSink, StateIO, FrameCount, ImpulseNode (+5 more)

### Community 172 - "ConnectMixerCommand"
Cohesion: 0.13
Nodes (12): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+4 more)

### Community 174 - "v1.4/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 175 - "ClapLibrary.cpp"
Cohesion: 0.21
Nodes (12): blobRead(), closeEditor, close, open, clap_host_t, clap_istream_t, path, hostGetExtension() (+4 more)

### Community 176 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 177 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 178 - "MidiDeviceInfo"
Cohesion: 0.25
Nodes (6): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback

### Community 179 - "TrackCommands.cpp"
Cohesion: 0.18
Nodes (11): execute, execute, undo, SetTrackHeightCommand, canMergeWith, execute, height_, mergeWith (+3 more)

### Community 180 - "AnalyzerEffect"
Cohesion: 0.38
Nodes (4): AnalyzerEffect, maxChannels, atomic, size_t

### Community 181 - "INCDAWPlaylistView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry, -invalidateWaveformCache

### Community 182 - "ModulationEffects.cpp"
Cohesion: 0.22
Nodes (10): prepare, FrameCount, Sample, SampleRate, size_t, vector, prepare, prepare (+2 more)

### Community 183 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 184 - "renderArrangement"
Cohesion: 0.23
Nodes (10): FrameCount, path, Sample, size_t, vector, makeAudio(), renderArrangement(), ScratchDir (+2 more)

### Community 186 - "ChannelCommands.cpp"
Cohesion: 0.19
Nodes (10): execute, execute, SetChannelVolumeCommand, canMergeWith, channelId_, execute, mergeWith, previousVolume_ (+2 more)

### Community 187 - "BuiltinEffectTests.cpp"
Cohesion: 0.19
Nodes (15): FrameCount, Sample, size_t, vector, processThrough(), RefAllpass, index, line (+7 more)

### Community 188 - "PluginStateTests.cpp"
Cohesion: 0.12
Nodes (17): anyNonZero(), compileLoaded(), InsertFactory, path, Sample, uint8_t, vector, factoryFor() (+9 more)

### Community 190 - "SetSendGainCommand"
Cohesion: 0.18
Nodes (8): SetSendGainCommand, canMergeWith, connectionId_, execute, gain_, mergeWith, previous_, undo

### Community 191 - "timeStretch"
Cohesion: 0.29
Nodes (9): size_t, vector, detectOnsets(), monoMixOf(), similarityAt(), StretchOptions, pitchSemitones, ratio (+1 more)

### Community 192 - "setParameter"
Cohesion: 0.22
Nodes (3): FilterMode, uint32_t, setParameter

### Community 193 - "humanizeNoteStarts"
Cohesion: 0.29
Nodes (10): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+2 more)

### Community 194 - "Fixture"
Cohesion: 0.20
Nodes (10): Tick, vector, Fixture, channel, pattern, project, trackA, trackB (+2 more)

### Community 195 - "AutomationFixture"
Cohesion: 0.20
Nodes (9): AutomationFixture, channel, pattern, project, tempo, AutomationPoint, Tick, enginePoint() (+1 more)

### Community 196 - "read"
Cohesion: 0.23
Nodes (21): assetFilePath(), FrameCount, Sample, string, vector, execute, name, undo (+13 more)

### Community 197 - "SimpleSynth.cpp"
Cohesion: 0.27
Nodes (9): size_t, frequencyForKey(), activeVoiceCount, allNotesOff, findVoiceToSteal, handleMessage, releaseVoicesForKey, startVoice (+1 more)

### Community 199 - "collectForBlock"
Cohesion: 0.29
Nodes (6): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock, resetCounters

### Community 200 - "create"
Cohesion: 0.25
Nodes (8): clap_event_param_value_t, create, array, string, unique_ptr, PendingParamEvents, count, events

### Community 201 - "RemoveChannelCommand"
Cohesion: 0.20
Nodes (8): RemovedContent, vector, RemoveChannelCommand, channel_, channelId_, content_, index_, undo

### Community 202 - "INCDAW — Release Guide"
Cohesion: 0.29
Nodes (6): 1. What a release is, 2. Cutting a release, 3. Installing (first launch on another Mac), 4. Updating, 5. Release notes — 0.9.0 (2026-08-16), INCDAW — Release Guide

### Community 203 - "Fixture"
Cohesion: 0.25
Nodes (6): Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 204 - "RoutingConnection"
Cohesion: 0.20
Nodes (9): findRouting, RoutingConnection, destination, gain, id, isSend, preFader, sidechain (+1 more)

### Community 205 - "v1.2/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 206 - "v1.3/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 207 - "RenderResult"
Cohesion: 0.20
Nodes (9): FrameCount, string, vector, RenderResult, arrangementFrames, audio, error, succeeded (+1 more)

### Community 208 - "processThrough"
Cohesion: 0.38
Nodes (9): FrameCount, Sample, size_t, vector, processThrough(), requireBitExact(), requireFiniteAndBounded(), rmsOf() (+1 more)

### Community 209 - "Fixture"
Cohesion: 0.40
Nodes (3): Fixture, project, registry

### Community 211 - "ClapDescriptor"
Cohesion: 0.12
Nodes (17): ClapDescriptor, id, name, vendor, version, string, path, string (+9 more)

### Community 212 - "Fixture"
Cohesion: 0.18
Nodes (8): Tick, vector, Fixture, channel, pattern, project, registry, note()

### Community 214 - "SplitFixture"
Cohesion: 0.20
Nodes (9): Tick, note(), SplitFixture, channel, clip, pattern, project, registry (+1 more)

### Community 215 - "SetClipMutedCommand"
Cohesion: 0.25
Nodes (6): SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 216 - "StressTests.cpp"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 217 - "SplitClipCommand"
Cohesion: 0.22
Nodes (7): SplitClipCommand, clip_, minted_, previous_, right_, splitTick_, undo

### Community 218 - "exportArrangement"
Cohesion: 0.25
Nodes (7): size_t, notes_, path, Result, uint64_t, exportArrangement(), importAsPattern()

### Community 219 - "StretchClipsCommand"
Cohesion: 0.22
Nodes (8): Snapshot, StretchClipsCommand, canMergeWith, clips_, lengthDelta_, mergeWith, previous_, undo

### Community 221 - "SetChannelStepKeyCommand"
Cohesion: 0.22
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 222 - "Fixture"
Cohesion: 0.22
Nodes (7): Tick, Fixture, channel, pattern, project, registry, note()

### Community 223 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 224 - "MidiImportResult"
Cohesion: 0.25
Nodes (7): string, vector, MidiImportResult, error, newChannels, pattern, succeeded

### Community 225 - "RemoveTrackCommand"
Cohesion: 0.22
Nodes (7): RemovedClip, vector, RemoveTrackCommand, clips_, index_, track_, trackId_

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

### Community 233 - "trimToMaximumDepth"
Cohesion: 0.40
Nodes (6): execute, executeMerging, setMaximumDepth, trimToMaximumDepth, CommandPtr, size_t

### Community 234 - "SetChannelMutedCommand"
Cohesion: 0.25
Nodes (5): SetChannelMutedCommand, channelId_, execute, muted_, undo

### Community 235 - "v1.5/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 236 - "renderProject"
Cohesion: 0.25
Nodes (7): findChannel, path, uint64_t, DitherSource, state_, renderProject(), renderProjectToFile()

### Community 237 - "renderBlock"
Cohesion: 0.47
Nodes (5): FrameCount, Sample, vector, renderBlock(), tone()

### Community 238 - "ScratchDirectory"
Cohesion: 0.33
Nodes (4): path, string, ScratchDirectory, path

### Community 239 - "SetChannelSoloedCommand"
Cohesion: 0.25
Nodes (5): SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 240 - "SetMixerPolarityCommand"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 241 - "INCDAW — FL Studio 2026 Gap Analysis"
Cohesion: 0.40
Nodes (4): 1. FL Studio 2026 headline features vs INCDAW 0.9.0, 2. Baseline FL Studio capabilities INCDAW still lacks, 3. Closure plan — this branch, INCDAW — FL Studio 2026 Gap Analysis

### Community 242 - "AddTrackCommand"
Cohesion: 0.25
Nodes (6): AddTrackCommand, index_, minted_, track_, undo, size_t

### Community 243 - "KernelTable"
Cohesion: 0.21
Nodes (9): SampleRate, vector, KernelTable, phases, weights_, width, resample(), sinc() (+1 more)

### Community 244 - "SetTrackMutedCommand"
Cohesion: 0.25
Nodes (5): SetTrackMutedCommand, execute, muted_, trackId_, undo

### Community 245 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 246 - "SetTrackSoloedCommand"
Cohesion: 0.25
Nodes (5): SetTrackSoloedCommand, execute, soloed_, trackId_, undo

### Community 248 - "sampleWaveform"
Cohesion: 0.25
Nodes (8): FrameCount, SampleRate, Voice, Waveform, polyBlep(), prepare, renderRange, sampleWaveform

### Community 249 - "openEditor"
Cohesion: 0.25
Nodes (8): size, blobWrite(), hasEditor, openEditor, process, setParameter, clap_ostream_t, uint32_t

### Community 250 - "RenameChannelCommand"
Cohesion: 0.29
Nodes (5): RenameChannelCommand, channelId_, execute, previousName_, undo

### Community 252 - "emptyOutTryPush"
Cohesion: 0.33
Nodes (6): clap_event_header_t, clap_input_events_t, clap_output_events_t, emptyOutTryPush(), pendingInGet(), pendingInSize()

### Community 253 - "setParameter"
Cohesion: 0.33
Nodes (4): uint32_t, Sample, Waveform, setParameter

### Community 254 - "collectForRange"
Cohesion: 0.33
Nodes (4): FrameCount, FramePosition, MidiBuffer, collectForRange

### Community 255 - "PatternTests.cpp"
Cohesion: 0.53
Nodes (5): Tick, vector, note(), shapeOf(), startsOf()

### Community 256 - "RecordingSink"
Cohesion: 0.40
Nodes (5): pair, ParameterSink, uint32_t, RecordingSink, received

### Community 257 - "ScratchDirectory"
Cohesion: 0.33
Nodes (4): path, string, ScratchDirectory, path

### Community 258 - "arrangementEndFrames"
Cohesion: 0.50
Nodes (5): clipLengthTicks(), clipStartTicks(), Tick, arrangementEndFrames(), FrameCount

### Community 259 - "ScratchDir"
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
- **1513 isolated node(s):** `streams_`, `enabled_`, `noRow`, `layout_`, `id` (+1508 more)
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
- **Why does `EntityId` connect `EntityId` to `PatternCommands.cpp`, `RenameTrackCommand`, `CountingCommand`, `PlaylistModel`, `allocate`, `AddInsertCommand`, `CompiledProjectGraph`, `EditAssetRegionCommand`, `ClipCommands.cpp`, `InstrumentNode`, `Command`, `SampleCache`, `RemoveMixerNodeCommand`, `InsertRecordedTakeCommand`, `SendFixture`, `Clip`, `SidechainFixture`, `ArpeggiateNotesCommand`, `GraphCompileOptions`, `LoadSampleCommand`, `ConnectMixerCommand`, `WriteAutomationCommand`, `TrackCommands.cpp`, `load`, `ChannelCommands.cpp`, `AutomationWriteSession`, `PluginStateTests.cpp`, `SetSendGainCommand`, `EditFixture`, `MixerCommands.cpp`, `AutomationFixture`, `read`, `Channel`, `ClipIds`, `Pattern`, `Fixture`, `RemoveChannelCommand`, `Fixture`, `RoutingConnection`, `PianoRollModel`, `MixerTests.cpp`, `Fixture`, `AddMidiMappingCommand`, `AddMarkerCommand`, `SplitFixture`, `SplitClipCommand`, `PluginInsertTests.cpp`, `SetChannelStepKeyCommand`, `AddChannelCommand`, `SetChannelOutputCommand`, `MidiImportResult`, `RemoveTrackCommand`, `Fixture`, `Project`, `SetChannelMutedCommand`, `renderProject`, `NudgeChordCommand`, `SetChannelSoloedCommand`, `SetMixerPolarityCommand`, `AddPatternClipCommand`, `MidiEvent`, `AddTrackCommand`, `SetTrackMutedCommand`, `RenderOptions`, `SetTrackSoloedCommand`, `NoteCommands.cpp`, `RenameChannelCommand`, `string`?**
  _High betweenness centrality (0.124) - this node is a cross-community bridge._