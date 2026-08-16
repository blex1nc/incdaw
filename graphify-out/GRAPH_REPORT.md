# Graph Report - continue-after-analysis-0b7236  (2026-08-16)

## Corpus Check
- 317 files · ~240,786 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 5032 nodes · 8907 edges · 255 communities (250 shown, 5 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 442 edges (avg confidence: 0.81)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `bb47fb50`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- PatternCommands.cpp
- Command
- Transport
- WavStreamWriter
- PluginRegistry
- AudioBufferPool
- CoreAudioDevice
- PlaylistModel.cpp
- SimpleSynth
- AddInsertCommand
- Json
- Node
- CommandRegistry
- AudioEngine
- TestLatencyPlugin.cpp
- EditAssetRegionCommand
- ChannelRackModel
- AudioLogger
- RecordingSession
- TempoMap.cpp
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
- Model.h
- compileArrangement
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
- INCDAWAppDelegate
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
- ChannelCommands.cpp
- INCDAW — Architecture
- ProcessContext
- DuplicateClipsCommand
- MoveClipsCommand
- PlaylistView.mm
- BuiltinEffect
- Browser
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
- TempoMap
- AddPatternClipCommand
- SliceAssetCommand
- LoudnessMeterEffect
- ClapLibrary.cpp
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
- Pattern
- AutomationProbe
- PlaylistModel
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
- BlockSegment
- MusicTheory.cpp
- AudioFileData
- Json.cpp
- ClipCommands.cpp
- ClapInstance
- ConstantNode
- ResizeClipsCommand
- friend
- Parser
- SampleCache
- RecordedEvent
- PluginNode
- process
- capturePluginState
- SendFixture
- Version
- TimestampedMidiMessage
- BuiltinEffect.cpp
- makeTestSignal
- ClapLibrary
- SidechainFixture
- TimeSignatureEvent
- ScratchDirectory
- MidiMapNode
- INCDAWMixerView
- ImpulseNode
- ConnectMixerCommand
- ParameterSink
- v1.4/Fixture.incdaw/manifest.json
- AddAutomationLaneCommand
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- MidiDeviceInfo
- INCDAWBrowserNode
- AnalyzerEffect
- ChannelSamplerZone
- ImpulseNode
- check
- renderArrangement
- AudioCaptureSink
- AutomationCommands.cpp
- BuiltinEffectTests.cpp
- PluginStateTests.cpp
- MixerStripNode.cpp
- timeStretch
- MixerNode
- MidiEvent
- Fixture
- AutomationFixture
- read
- DiskStreamer
- ClipIds
- collectForBlock
- PendingParamEvents
- AudioAsset
- INCDAW — Release Guide
- Fixture
- Track
- v1.2/Fixture.incdaw/manifest.json
- v1.3/Fixture.incdaw/manifest.json
- RenderResult
- ModulationEffectTests.cpp
- Fixture
- ClapDescriptor
- Fixture
- SplitFixture
- SetClipMutedCommand
- build
- SplitClipCommand
- exportArrangement
- StretchClipsCommand
- string
- PluginParameterInfo
- Fixture
- SetAutomationPointsCommand
- MidiImportResult
- RemoveAutomationLaneCommand
- ScratchDirectory
- SequencedNote
- ScratchDirectory
- INCDAWPianoRollView
- trimToMaximumDepth
- create
- v1.5/Fixture.incdaw/manifest.json
- Channel
- InputMonitorNode
- ScratchDirectory
- TimelineMarker
- INCDAWBrowserView
- INCDAW — FL Studio 2026 Gap Analysis
- ScriptedFactory
- KernelTable
- AudioEditorView.mm
- INCDAWAudioEditorView
- ScratchDirectory
- InsertFixture
- bench/main.cpp
- WavStreamReader.cpp
- ScratchDir
- snapTick
- ScratchDirectory

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

## Communities (255 total, 5 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "PatternCommands.cpp"
Cohesion: 0.04
Nodes (43): AddPatternCommand, execute, index_, minted_, pattern_, undo, DuplicatePatternCommand, execute (+35 more)

### Community 2 - "Command"
Cohesion: 0.04
Nodes (45): Command, execute, id, name, undo, AddTrackCommand, execute, index_ (+37 more)

### Community 3 - "Transport"
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, uint32_t (+17 more)

### Community 4 - "WavStreamWriter"
Cohesion: 0.05
Nodes (59): ofstream, appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample (+51 more)

### Community 5 - "PluginRegistry"
Cohesion: 0.16
Nodes (21): Library, Located, int64_t, path, size_t, string, uint64_t, vector (+13 more)

### Community 6 - "AudioBufferPool"
Cohesion: 0.09
Nodes (15): AudioBufferPool, channelPointers_, reset, samples_, FrameCount, Sample, size_t, unique_ptr (+7 more)

### Community 7 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 8 - "PlaylistModel.cpp"
Cohesion: 0.20
Nodes (18): Rect, size_t, vector, addToSelection, clipAtPoint, clipRect, clipsInRectangle, collectVisibleClips (+10 more)

### Community 9 - "SimpleSynth"
Cohesion: 0.06
Nodes (39): FrameCount, SampleRate, size_t, uint32_t, Voice, Waveform, frequencyForKey(), array (+31 more)

### Community 10 - "AddInsertCommand"
Cohesion: 0.08
Nodes (24): AddInsertCommand, execute, minted_, mixerNode_, plugin_, slot_, undo, findNode() (+16 more)

### Community 11 - "Json"
Cohesion: 0.10
Nodes (14): nullptr_t, int64_t, pair, string, vector, Json, asDouble, boolean_ (+6 more)

### Community 12 - "Node"
Cohesion: 0.07
Nodes (29): Node, process, ParameterSink, StateIO, CompiledProjectGraph, automation, channels, channelStripFor (+21 more)

### Community 13 - "CommandRegistry"
Cohesion: 0.13
Nodes (12): CommandRegistry, actions_, clearHistory, project_, redo, redoStack_, undo, undoStack_ (+4 more)

### Community 14 - "AudioEngine"
Cohesion: 0.07
Nodes (49): RetiredGraph, AudioEngine, active_, anchor_, anchorVersion_, audioDeviceAboutToStart, audioDeviceStopped, availableDevices (+41 more)

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

### Community 20 - "TempoMap.cpp"
Cohesion: 0.20
Nodes (16): clampTempo(), FramePosition, SampleRate, Tick, vector, musicalPositionForTick, nextTempoChangeAfter, rebuild (+8 more)

### Community 21 - "InstrumentNode"
Cohesion: 0.07
Nodes (25): MidiBuffer, ParameterSink, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare (+17 more)

### Community 22 - "EntityId"
Cohesion: 0.08
Nodes (40): size_t, Tick, vector, noteAtStep(), execute, undo, EntityId, invalidValue (+32 more)

### Community 23 - "WavStreamReader"
Cohesion: 0.11
Nodes (16): ifstream, FrameCount, path, SampleRate, size_t, uint16_t, uint64_t, uint8_t (+8 more)

### Community 24 - "Sampler"
Cohesion: 0.05
Nodes (30): FilterMode, uint32_t, array, atomic, maxVoices, ParameterSink, SampleRate, size_t (+22 more)

### Community 25 - "MixerStripNode"
Cohesion: 0.12
Nodes (11): atomic, Sample, MixerStripNode, left_, meter_, muted_, polarityInverted_, right_ (+3 more)

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
Cohesion: 0.10
Nodes (24): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+16 more)

### Community 30 - "InsertRecordedTakeCommand"
Cohesion: 0.10
Nodes (16): Placement, size_t, string, vector, InsertRecordedTakeCommand, asset_, assetIndex_, clipIndices_ (+8 more)

### Community 31 - "CoreAudioDevice.cpp"
Cohesion: 0.29
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 32 - "NoteSequence"
Cohesion: 0.12
Nodes (17): FrameCount, FramePosition, MidiBuffer, Tick, vector, Tick, uint32_t, vector (+9 more)

### Community 33 - "Clip"
Cohesion: 0.08
Nodes (25): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+17 more)

### Community 34 - "atomic"
Cohesion: 0.20
Nodes (3): array, atomic, MidiBuffer

### Community 35 - "MidiMessage"
Cohesion: 0.10
Nodes (11): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+3 more)

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
Cohesion: 0.06
Nodes (46): Direction, map, ArpeggiateNotesCommand, channel_, direction_, execute, indices_, pattern_ (+38 more)

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
Cohesion: 0.16
Nodes (12): Tick, friend, int64_t, MusicalPosition, bar, beat, tick, musicalPositionToTicks() (+4 more)

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
Cohesion: 0.12
Nodes (15): WriteAutomationCommand, clip_, clipIndex_, key_, laneAfter_, laneCreated_, laneId_, laneIndex_ (+7 more)

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
Cohesion: 0.19
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
Cohesion: 0.13
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 61 - "SystemInfo"
Cohesion: 0.14
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
Cohesion: 0.14
Nodes (13): AudioClipNode, addClip, clips_, fetchScratch_, prepare, process, FrameCount, PlacedClip (+5 more)

### Community 66 - "MixerCommands.cpp"
Cohesion: 0.02
Nodes (77): RemovedRouting, AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo (+69 more)

### Community 67 - "AudioBufferView"
Cohesion: 0.08
Nodes (25): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t, allocate (+17 more)

### Community 68 - "Model.h"
Cohesion: 0.10
Nodes (19): AutomationLane, id, parameterKey, points, targetEntity, string, PluginSlot, bypassed (+11 more)

### Community 69 - "compileArrangement"
Cohesion: 0.28
Nodes (15): Emit, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto(), Tick, uint64_t (+7 more)

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
Cohesion: 0.08
Nodes (35): NoteList, size_t, Tick, vector, size_t, Tick, vector, Viewport (+27 more)

### Community 77 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 78 - "SampleRingBuffer"
Cohesion: 0.19
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 79 - "[0.9.0] — 2026-08-16 — the core is complete"
Cohesion: 0.04
Nodes (44): [0.9.0] — 2026-08-16 — the core is complete, INCDAW — Changelog, Phase 0 — Research and architecture — 2026-08-14, Phase 10 — Mixer, routing and delay compensation — 2026-08-14, Phase 11a — Automation: the generic subsystem — 2026-08-15, Phase 11b — Automation placement and recording — 2026-08-15, Phase 12 (part 1) — WAV codec — 2026-08-15, Phase 12 (part 2) — Input capture and recording — 2026-08-15 (+36 more)

### Community 80 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 81 - "MixerTests.cpp"
Cohesion: 0.15
Nodes (12): FrameCount, Sample, SampleRate, size_t, vector, LatentProcessorNode, delay_, latency_ (+4 more)

### Community 82 - "ioProcTrampoline"
Cohesion: 0.21
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 83 - "INCDAWAppDelegate"
Cohesion: 0.11
Nodes (16): NSApplicationDelegate, NSSegmentedControl, NSTextField, NSWindow, NSView, INCDAWChannelRackView, -initWithFrameprojectregistry, INCDAWAppDelegate (+8 more)

### Community 84 - "INCDAW — Roadmap"
Cohesion: 0.08
Nodes (23): Deliberately out of scope, INCDAW — Roadmap, Phase 0 — Research and architecture ✅ COMPLETE, Phase 10 — Mixer and routing, Phase 11 — Automation, Phase 12 — Recording and audio editor, Phase 13 — Plugin hosting, Phase 14 — Sampler (+15 more)

### Community 85 - "AddMidiMappingCommand"
Cohesion: 0.10
Nodes (17): AddMidiMappingCommand, controller_, execute, mapping_, midiChannel_, minted_, parameterKey_, target_ (+9 more)

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
Nodes (34): dbToGain(), sumInputsInto(), coefficientFor(), process, size_t, process, process, linkedPeakAt() (+26 more)

### Community 97 - "DuplicateClipsCommand"
Cohesion: 0.22
Nodes (8): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_, undo

### Community 98 - "MoveClipsCommand"
Cohesion: 0.18
Nodes (10): MovedAudioClip, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, canMergeWith, clips_, mergeWith, movedAudio_ (+2 more)

### Community 99 - "PlaylistView.mm"
Cohesion: 0.14
Nodes (21): -acceptsFirstResponder, -addTrackRect, -drawAutomationCurveForinRect, -drawBarLinesInLaneAtheight, -drawClips, -drawPlayhead, -drawRect, -drawRuler (+13 more)

### Community 100 - "BuiltinEffect"
Cohesion: 0.08
Nodes (22): BuiltinEffect, values_, EffectParameter, defaultValue, id, maxValue, minValue, name (+14 more)

### Community 101 - "Browser"
Cohesion: 0.07
Nodes (62): directory_entry, Browser, addDefaultRoots, addRoot, canDecodeAudio, classify, clear, defaultSearchLimit (+54 more)

### Community 102 - "INCDAW — Audio Engine"
Cohesion: 0.15
Nodes (12): 10. Audio correctness requirements, 11. Performance budget, 1. The prime directive, 2. Device layer, 3. Realtime thread scheduling, 4. Realtime safety enforcement, 5. Signal flow, 6. Block processing and sample-accurate events (+4 more)

### Community 103 - "renderNode"
Cohesion: 0.21
Nodes (11): FrameCount, path, Sample, shared_ptr, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 104 - "Project"
Cohesion: 0.07
Nodes (33): Project, audioAssets_, automation_, channels_, clips_, findMixerNode, ids_, markers_ (+25 more)

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

### Community 109 - "NudgeChordCommand"
Cohesion: 0.09
Nodes (26): vector, findEvents(), Scale, size_t, string, vector, InsertNotesCommand, channel_ (+18 more)

### Community 110 - "vector"
Cohesion: 0.06
Nodes (20): vector, string, unordered_map, mutex, StateIO, loadState, saveState, path (+12 more)

### Community 111 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 112 - "TempoMap"
Cohesion: 0.05
Nodes (31): AutomationNode, bindings_, tempoMap_, Binding, size_t, vector, FramePosition, SampleRate (+23 more)

### Community 113 - "AddPatternClipCommand"
Cohesion: 0.15
Nodes (11): AddPatternClipCommand, clip_, execute, index_, length_, minted_, pattern_, start_ (+3 more)

### Community 114 - "SliceAssetCommand"
Cohesion: 0.12
Nodes (14): FrameCount, size_t, string, vector, SliceAssetCommand, asset_, channel_, channelIndex_ (+6 more)

### Community 115 - "LoudnessMeterEffect"
Cohesion: 0.06
Nodes (31): Biquad, FrameCount, SampleRate, FrameCount, SampleRate, uint64_t, LoudnessMeterEffect, completeHop (+23 more)

### Community 116 - "ClapLibrary.cpp"
Cohesion: 0.09
Nodes (32): clap_event_header_t, blobRead(), BlobReader, cursor, data, size, blobWrite(), BlobWriter (+24 more)

### Community 117 - "RenderOptions"
Cohesion: 0.11
Nodes (19): BitDepth, FramePosition, SampleRate, uint64_t, RenderOptions, bitDepth, blockSize, dither (+11 more)

### Community 118 - "main.mm"
Cohesion: 0.22
Nodes (13): -applicationDidFinishLaunching, -editorChanged, +listWidth, -openAudioAssetInEditor, -selectPattern, -showAudioEditor, -showEditorAtSegment, -showMixer (+5 more)

### Community 119 - "FlangerEffect"
Cohesion: 0.06
Nodes (37): ChorusEffect, centreMs, line_, mask_, maxChannels, maxDepthMs, phase_, prepare (+29 more)

### Community 120 - "NoteCommands.cpp"
Cohesion: 0.04
Nodes (66): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, NoteIndices (+58 more)

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
Cohesion: 0.15
Nodes (8): AudioCaptureSink, FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 126 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped()

### Community 127 - "Pattern"
Cohesion: 0.09
Nodes (21): AutomationCurve, AutomationPoint, curve, tension, tick, value, Tick, vector (+13 more)

### Community 128 - "AutomationProbe"
Cohesion: 0.29
Nodes (6): AutomationProbe, calls, registry, written, FramePosition, vector

### Community 129 - "PlaylistModel"
Cohesion: 0.16
Nodes (10): size_t, Tick, vector, Viewport, PlaylistModel, noClip, noTrack, resizeHandleWidth (+2 more)

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
Cohesion: 0.35
Nodes (10): findAction, invoke, redoName, registerAction, search, undoName, Entry, string (+2 more)

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
Cohesion: 0.14
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
Cohesion: 0.20
Nodes (9): string, RemovedClip, vector, RemoveClipsCommand, clips_, execute, name, removed_ (+1 more)

### Community 144 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 145 - "BlockSegment"
Cohesion: 0.20
Nodes (9): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, size_t (+1 more)

### Community 146 - "MusicTheory.cpp"
Cohesion: 0.11
Nodes (32): ChordDetection, bassKey, display, inverted, matched, rootPitchClass, type, ChordType (+24 more)

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
Nodes (22): clap_plugin_gui_t, clap_plugin_state_t, ParamEvent, ClapInstance, closeEditor, editorOpen_, gui_, host_ (+14 more)

### Community 151 - "ConstantNode"
Cohesion: 0.10
Nodes (14): ConstantNode, latency_, value_, FrameCount, Sample, size_t, vector, OrderRecordingNode (+6 more)

### Community 152 - "ResizeClipsCommand"
Cohesion: 0.20
Nodes (9): FrameCount, ResizeClipsCommand, canMergeWith, clips_, lengthDelta_, mergeWith, previousFrameLengths_, previousLengths_ (+1 more)

### Community 153 - "friend"
Cohesion: 0.12
Nodes (9): friend, MidiMapping, controller, id, maxValue, midiChannel, minValue, parameterKey (+1 more)

### Community 154 - "Parser"
Cohesion: 0.30
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 155 - "SampleCache"
Cohesion: 0.15
Nodes (16): int64_t, path, shared_ptr, size_t, string, uintmax_t, Entry, mutex (+8 more)

### Community 156 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 157 - "PluginNode"
Cohesion: 0.18
Nodes (5): FrameCount, ParameterSink, StateIO, PluginNode, instance_

### Community 158 - "process"
Cohesion: 0.17
Nodes (12): framesToSeconds(), FrameCount, SampleRate, Tick, secondsToFrames(), secondsToTicks(), ticksToSeconds(), FrameCount (+4 more)

### Community 159 - "capturePluginState"
Cohesion: 0.51
Nodes (9): capturePluginState(), path, string, uint8_t, vector, readBlobFile(), restorePluginState(), stateFileNameFor() (+1 more)

### Community 160 - "SendFixture"
Cohesion: 0.33
Nodes (5): SendFixture, project, registry, strip, tempo

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
Cohesion: 0.13
Nodes (15): clap_plugin_entry_t, ClapLibrary, close, create, descriptors, entry_, factory_, library_ (+7 more)

### Community 166 - "SidechainFixture"
Cohesion: 0.14
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

### Community 171 - "ImpulseNode"
Cohesion: 0.25
Nodes (6): FrameCount, Sample, vector, ImpulseNode, at_, render()

### Community 172 - "ConnectMixerCommand"
Cohesion: 0.06
Nodes (28): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+20 more)

### Community 174 - "v1.4/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 175 - "AddAutomationLaneCommand"
Cohesion: 0.15
Nodes (8): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, size_t, string

### Community 176 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 177 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 178 - "MidiDeviceInfo"
Cohesion: 0.25
Nodes (6): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback

### Community 179 - "INCDAWBrowserNode"
Cohesion: 0.16
Nodes (13): app, GroupKind, NSMutableArray, NSOutlineView, NSScrollView, NSSearchField, NSTableColumn, INCDAWBrowserNode (+5 more)

### Community 180 - "AnalyzerEffect"
Cohesion: 0.38
Nodes (4): AnalyzerEffect, maxChannels, atomic, size_t

### Community 181 - "ChannelSamplerZone"
Cohesion: 0.14
Nodes (14): ChannelSamplerZone, asset, end, gain, keyHigh, keyLow, loopCrossfade, loopEnd (+6 more)

### Community 182 - "ImpulseNode"
Cohesion: 0.15
Nodes (9): FramePosition, ImpulseNode, latency_, position_, MixerFixture, channel, pattern, project (+1 more)

### Community 183 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 184 - "renderArrangement"
Cohesion: 0.23
Nodes (10): FrameCount, path, Sample, size_t, vector, makeAudio(), renderArrangement(), ScratchDir (+2 more)

### Community 186 - "AutomationCommands.cpp"
Cohesion: 0.19
Nodes (12): execute, undo, AutomationPoint, vector, findLane(), canMergeWith, execute, mergeWith (+4 more)

### Community 187 - "BuiltinEffectTests.cpp"
Cohesion: 0.19
Nodes (14): FrameCount, Sample, size_t, vector, RefAllpass, index, line, RefComb (+6 more)

### Community 188 - "PluginStateTests.cpp"
Cohesion: 0.12
Nodes (17): anyNonZero(), compileLoaded(), InsertFactory, path, Sample, uint8_t, vector, factoryFor() (+9 more)

### Community 190 - "MixerStripNode.cpp"
Cohesion: 0.24
Nodes (12): FrameCount, Sample, SampleRate, panGains, prepare, process, refreshTargets, setGain (+4 more)

### Community 191 - "timeStretch"
Cohesion: 0.29
Nodes (9): size_t, vector, detectOnsets(), monoMixOf(), similarityAt(), StretchOptions, pitchSemitones, ratio (+1 more)

### Community 192 - "MixerNode"
Cohesion: 0.15
Nodes (13): MixerNodeType, MixerNode, colour, id, inserts, muted, name, pan (+5 more)

### Community 193 - "MidiEvent"
Cohesion: 0.10
Nodes (25): execute, undo, appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector (+17 more)

### Community 194 - "Fixture"
Cohesion: 0.20
Nodes (10): Tick, vector, Fixture, channel, pattern, project, trackA, trackB (+2 more)

### Community 195 - "AutomationFixture"
Cohesion: 0.20
Nodes (9): AutomationFixture, channel, pattern, project, tempo, AutomationPoint, Tick, enginePoint() (+1 more)

### Community 196 - "read"
Cohesion: 0.23
Nodes (21): assetFilePath(), FrameCount, Sample, string, vector, execute, name, undo (+13 more)

### Community 197 - "DiskStreamer"
Cohesion: 0.20
Nodes (10): DiskStreamer, mutex_, running_, serviceOnce, streams_, thread_, atomic, mutex (+2 more)

### Community 199 - "collectForBlock"
Cohesion: 0.29
Nodes (6): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock, resetCounters

### Community 200 - "PendingParamEvents"
Cohesion: 0.40
Nodes (5): clap_event_param_value_t, array, PendingParamEvents, count, events

### Community 201 - "AudioAsset"
Cohesion: 0.17
Nodes (11): AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id, relativePath (+3 more)

### Community 202 - "INCDAW — Release Guide"
Cohesion: 0.29
Nodes (6): 1. What a release is, 2. Cutting a release, 3. Installing (first launch on another Mac), 4. Updating, 5. Release notes — 0.9.0 (2026-08-16), INCDAW — Release Guide

### Community 203 - "Fixture"
Cohesion: 0.25
Nodes (6): Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 204 - "Track"
Cohesion: 0.17
Nodes (12): findTrack, Track, colour, height, id, muted, name, outputMixerNode (+4 more)

### Community 205 - "v1.2/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 206 - "v1.3/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 207 - "RenderResult"
Cohesion: 0.20
Nodes (9): FrameCount, string, vector, RenderResult, arrangementFrames, audio, error, succeeded (+1 more)

### Community 208 - "ModulationEffectTests.cpp"
Cohesion: 0.39
Nodes (8): FrameCount, Sample, size_t, vector, requireBitExact(), requireFiniteAndBounded(), rmsOf(), testSignal()

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

### Community 216 - "build"
Cohesion: 0.27
Nodes (9): bucketize(), Bucket, FrameCount, path, Result, Sample, vector, sizeBuckets() (+1 more)

### Community 217 - "SplitClipCommand"
Cohesion: 0.22
Nodes (7): SplitClipCommand, clip_, minted_, previous_, right_, splitTick_, undo

### Community 218 - "exportArrangement"
Cohesion: 0.25
Nodes (7): size_t, notes_, path, Result, uint64_t, exportArrangement(), importAsPattern()

### Community 219 - "StretchClipsCommand"
Cohesion: 0.22
Nodes (8): Snapshot, StretchClipsCommand, canMergeWith, clips_, lengthDelta_, mergeWith, previous_, undo

### Community 221 - "PluginParameterInfo"
Cohesion: 0.20
Nodes (9): string, uint32_t, PluginParameterInfo, defaultValue, id, maxValue, minValue, name (+1 more)

### Community 222 - "Fixture"
Cohesion: 0.22
Nodes (7): Tick, Fixture, channel, pattern, project, registry, note()

### Community 223 - "SetAutomationPointsCommand"
Cohesion: 0.31
Nodes (6): AutomationPoint, vector, SetAutomationPointsCommand, laneId_, points_, previous_

### Community 224 - "MidiImportResult"
Cohesion: 0.25
Nodes (7): string, vector, MidiImportResult, error, newChannels, pattern, succeeded

### Community 225 - "RemoveAutomationLaneCommand"
Cohesion: 0.22
Nodes (6): RemoveAutomationLaneCommand, execute, index_, lane_, laneId_, undo

### Community 227 - "ScratchDirectory"
Cohesion: 0.33
Nodes (4): path, string, ScratchDirectory, path

### Community 229 - "SequencedNote"
Cohesion: 0.21
Nodes (11): SequencedNote, channel, key, lengthTicks, startTick, velocity, Tick, vector (+3 more)

### Community 231 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 232 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 233 - "trimToMaximumDepth"
Cohesion: 0.40
Nodes (6): execute, executeMerging, setMaximumDepth, trimToMaximumDepth, CommandPtr, size_t

### Community 234 - "create"
Cohesion: 0.22
Nodes (9): FrameCount, Sample, size_t, readAt, FrameCount, path, shared_ptr, string (+1 more)

### Community 235 - "v1.5/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 236 - "Channel"
Cohesion: 0.14
Nodes (14): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+6 more)

### Community 237 - "InputMonitorNode"
Cohesion: 0.12
Nodes (14): FrameCount, Sample, SampleRate, size_t, vector, InputMonitorNode, channelCount_, ring_ (+6 more)

### Community 238 - "ScratchDirectory"
Cohesion: 0.33
Nodes (4): path, string, ScratchDirectory, path

### Community 239 - "TimelineMarker"
Cohesion: 0.22
Nodes (8): uint32_t, findMarker, TimelineMarker, colour, id, length, name, tick

### Community 240 - "INCDAWBrowserView"
Cohesion: 0.25
Nodes (7): NSMenuDelegate, NSOutlineViewDataSource, NSOutlineViewDelegate, NSView, INCDAWBrowserView, -initWithFramebrowser, -reload

### Community 241 - "INCDAW — FL Studio 2026 Gap Analysis"
Cohesion: 0.40
Nodes (4): 1. FL Studio 2026 headline features vs INCDAW 0.9.0, 2. Baseline FL Studio capabilities INCDAW still lacks, 3. Closure plan — this branch, INCDAW — FL Studio 2026 Gap Analysis

### Community 242 - "ScriptedFactory"
Cohesion: 0.25
Nodes (7): function, InsertFactory, unique_ptr, ScriptedFactory, fail, makers, requests

### Community 243 - "KernelTable"
Cohesion: 0.21
Nodes (9): SampleRate, vector, KernelTable, phases, weights_, width, resample(), sinc() (+1 more)

### Community 244 - "AudioEditorView.mm"
Cohesion: 0.29
Nodes (6): -acceptsFirstResponder, -hasSelection, -initWithFrameprojectregistry, -isFlipped, -selectionFrom, -selectionTo

### Community 245 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 246 - "ScratchDirectory"
Cohesion: 0.33
Nodes (4): path, string, ScratchDirectory, path

### Community 248 - "InsertFixture"
Cohesion: 0.33
Nodes (4): InsertFixture, pattern, project, tempo

### Community 249 - "bench/main.cpp"
Cohesion: 0.47
Nodes (5): time_point, vector, main(), median(), millisecondsSince()

### Community 250 - "WavStreamReader.cpp"
Cohesion: 0.50
Nodes (4): path, Result, close, open

### Community 252 - "ScratchDir"
Cohesion: 0.50
Nodes (3): path, ScratchDir, path

### Community 257 - "ScratchDirectory"
Cohesion: 0.33
Nodes (4): path, string, ScratchDirectory, path

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
- **1532 isolated node(s):** `streams_`, `enabled_`, `path`, `name`, `kind` (+1527 more)
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
- **Why does `EntityId` connect `EntityId` to `PatternCommands.cpp`, `Command`, `PlaylistModel`, `PlaylistModel.cpp`, `PluginParameterTests.cpp`, `AddInsertCommand`, `Node`, `EditAssetRegionCommand`, `ClipCommands.cpp`, `friend`, `InsertRecordedTakeCommand`, `SendFixture`, `Clip`, `SidechainFixture`, `ArpeggiateNotesCommand`, `GraphCompileOptions`, `LoadSampleCommand`, `ConnectMixerCommand`, `AddAutomationLaneCommand`, `WriteAutomationCommand`, `ChannelSamplerZone`, `load`, `ImpulseNode`, `AutomationWriteSession`, `AutomationCommands.cpp`, `PluginStateTests.cpp`, `EditFixture`, `MixerNode`, `MixerCommands.cpp`, `AutomationFixture`, `read`, `Model.h`, `ClipIds`, `compileArrangement`, `Fixture`, `AudioAsset`, `Fixture`, `Track`, `Fixture`, `AddMidiMappingCommand`, `AddMarkerCommand`, `SplitFixture`, `SplitClipCommand`, `ChannelCommands.cpp`, `SetAutomationPointsCommand`, `MidiImportResult`, `RemoveAutomationLaneCommand`, `Fixture`, `InsertFixture`, `Project`, `Channel`, `NudgeChordCommand`, `vector`, `TimelineMarker`, `TempoMap`, `AddPatternClipCommand`, `SliceAssetCommand`, `RenderOptions`, `NoteCommands.cpp`, `Pattern`?**
  _High betweenness centrality (0.134) - this node is a cross-community bridge._