# Graph Report - timbre  (2026-08-23)

## Corpus Check
- 437 files · ~405,693 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 6935 nodes · 12474 edges · 336 communities (326 shown, 10 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 751 edges (avg confidence: 0.81)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `e5b00275`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- MixerCommands.cpp
- Browser
- SetVelocityCommand
- WavStreamWriter
- WriteAutomationCommand
- ApplyInsertPresetCommand
- LoudnessMeterEffect
- PatternCommands.cpp
- TestGainPlugin.cpp
- PianoRollModel
- AudioEngine
- Command
- PluginInstanceManager
- ProcessContext
- MultibandCompressorEffect
- Transport
- AddInsertCommand
- CoreAudioDevice
- Sampler
- AddMarkerCommand
- BiquadCoefficients
- SetNoteLabelCommand
- parseReleaseFeed
- AuditionPlayer
- AudioStream
- LookaheadLimiterEffect
- PluginInsertTests.cpp
- GraphBuilder
- StretchAssetCommand
- PresetBar.mm
- AudioRecorder
- CommandRegistry
- NudgeChordCommand
- atomic
- InstrumentNode
- TestLatencyPlugin.cpp
- MixerTests.cpp
- INCDAW
- ChannelRackModel
- WavStreamReader
- TempoMap
- FlangerEffect
- AudioUnitInstance
- CompiledProjectGraph
- MusicTheory.cpp
- EqEffect
- INCDAWMixerView
- RemoveMixerNodeCommand
- ClapInstance
- MetronomeNode
- AudioLogger
- read
- AddSamplerZoneCommand
- SetTempoCommand
- WavetableSynth
- Wavetable
- PluginRegistry
- ImportAudioClipCommand
- The work, in order
- AudioDeviceConfig
- Json.cpp
- RecordingSession
- PlaylistView.mm
- CoreAudioDevice.cpp
- GraphCompileOptions
- detectOnsets
- -initWithFramebrowser
- Clip
- INCDAWAppDelegate
- AppSettings
- read
- AddMidiMappingCommand
- MusicalPosition
- MidiMessage
- HostedPlugin
- INCDAWCommandPaletteView
- ChannelSamplerZone
- CallbackProfiler
- Sampler.cpp
- SimpleSynth
- SetSamplerZoneCommand
- WaveformOverview
- ParameterFixture
- MidiInput
- AudioDevice
- ControlBarView.mm
- DrumMachine
- AutomationWriteSession
- InsertRecordedTakeCommand
- ParameterRegistry
- RenderOptions
- project::compileProjectGraph
- FmSynth
- AudioBufferPool
- WaveshaperEffect
- InputMonitorNode
- PluginIdentifier
- ThemePalette.cpp
- RealtimeGuard.cpp
- INCDAW macOS app bundle target
- Audio Correctness Requirements
- FL Studio 2026 Functional Reference
- SamplerZoneStream
- PluginPickerModel
- PlaylistModel
- PianoRollHeaderModel
- AudioBufferView
- ImpulseNode
- SamplerZone
- DelayLineNode
- StereoImagerEffect
- load
- FuzzTests.cpp
- incdaw_tests suite target
- write
- CoreMidiDevice
- SliceAssetCommand
- AudioUnitParameterDescription
- SampleRingBuffer
- MixerStripNode
- AudioAssetImport
- MidiMapNode
- SystemInfo
- BasicMidiBuffer
- PianoInstrument
- EntityId
- allocate
- SplitFixture
- ConstantNode
- SamplerTests.cpp
- Options
- TimingProbeInstrument
- ioProcTrampoline
- MidiRecorder
- CoreMidiDevice.cpp
- app::CommandRegistry
- ParametricEqEffect
- AudioClipNode
- SamplerWiringTests.cpp
- LevelMeter
- LockFreeQueue
- Channel
- Project
- GainNode
- SineOscillatorNode
- PresetLibrary.cpp
- Json
- INCDAWShaperView
- EditFixture
- LoopbackResult
- BuiltinEffectTests.cpp
- INCDAWEqCurveView
- MultibandTests.cpp
- MidiEvent
- AutomationPoint
- plugins::ClapInstance
- .incdaw Package Directory
- Permanent Version Fixtures (tests/fixtures)
- PianoInstrument.cpp
- Fft
- ChannelRackView.mm
- PluginPersistenceTests.cpp
- SidechainFixture
- Plugin Misbehaviour Matrix
- ChannelCommands.cpp
- ClapLibrary.cpp
- AudioFileData
- Realtime Safety Guard (debug builds)
- RenderGraph (compiled immutable graph)
- INCDAW Testing Strategy
- ConvolutionReverbEffect
- SplitClipCommand
- RemoveTrackCommand
- PluginParameterInfo
- AnalyzerEffect
- VocoderEffect
- INCDAWConvolverPanel
- renderNode
- Node
- Phase 18 Measured Baseline (2026-08-16)
- D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded)
- Phase 1 — Foundation and Build System
- UI Build-Out (post-phase increments)
- BeatGateEffect
- KernelTable
- ClapDescriptor
- Pattern
- incdaw_engine layer library
- AutomationNode
- CompiledGraph
- renderArrangement
- Fixture
- capturePluginState
- ChildResult
- SettingsWindow.mm
- INCDAWBeatGateView
- FactoryPresetTable
- TimelineAnchor
- Release
- timeStretch
- SharedLibrary
- humanizeNoteStarts
- UtilityEffects.cpp
- PatternListView.mm
- AddPatternClipCommand
- TonePanel.mm
- ParsedHeader
- Fixture
- RecordedEvent
- processThrough
- CountingCommand
- PluginFolder
- SamplerStreamingTests.cpp
- make-dmg.sh
- Atomic Graph Pointer Swap
- D-028 — Hosted plugins reach the graph through an injected factory
- MidiDevice
- PianoModelSpec
- MidiDeviceInfo
- ToggleStepCommand
- SendFixture
- StereoImagerTests.cpp
- RemoveSamplerZoneCommand
- ScratchDirectory
- BuiltinEffect
- exportArrangement
- Smoother
- main.mm
- ConnectMixerCommand
- Denormals.h
- AudioDevice (platform device interface)
- INCDAWSettingsWindow
- ConvolutionReverb.cpp
- Fixture
- Preset
- renderClickFrames
- Version
- TimestampedMidiMessage
- LoadSampleCommand
- NoteCommands.cpp
- Fixture
- InsertAudioCommand
- RenderTests.cpp
- makeTestSignal
- project::compileProjectGraph
- BlobReader
- EditAssetRegionCommand
- INCDAWPianoRollView
- ScratchDir
- processThrough
- ScratchDirectory
- StoppedTransportTests.cpp
- StressTests.cpp
- engine/audio/AudioRecorder
- AppSettingsTests.cpp
- ClipCommands.cpp
- AddNoteCommand
- NoteSequence
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- v1.2/Fixture.incdaw/manifest.json
- v1.3/Fixture.incdaw/manifest.json
- v1.4/Fixture.incdaw/manifest.json
- v1.5/Fixture.incdaw/manifest.json
- v1.6/Fixture.incdaw/manifest.json
- ThemePaletteTests.cpp
- QuantizeNotesCommand
- setParameter
- main
- prepare
- .analysisSampleRate
- ScratchDirectory
- MoveNotesCommand
- collectForBlock
- StrumNotesCommand
- nanosForFrame
- check
- AudioCaptureSink
- ParameterSink
- ProjectSession.cpp
- SessionFixture
- Performance Targets (audio, UI, project)
- create
- D-020 — A mixer strip is one node
- StateIO
- CommandRegistry
- Out-of-Process-Ready Plugin Host ABI
- D-009 — Distribution: ad-hoc signed, un-notarized DMG
- D-010 — Version control: git, initialised at Phase 0
- PluginStateTests.cpp
- ClapInstance
- MintedAsset
- MidiImportResult
- ParameterTable
- AutomationFixture
- collectForRange
- ArpeggiateNotesCommand
- DeleteNotesCommand
- INCDAWAudioEditorView
- INCDAWTonePanel
- SequencedNote
- UpdateCheckTests.cpp
- DiskStreamer
- SampleCache
- Legal / IP Boundary
- emptyOutTryPush
- build
- RenderResult
- processThrough
- BuiltinInstrumentInfo
- .incdaw versioned project format
- ResizeNotesCommand
- uint32_t
- processThrough
- AutomationProbe
- ScriptedFactory
- INCDAWControlBarView
- load
- BuiltinEffectInfo
- InsertFixture
- OrderRecordingNode
- prepare
- transformImpulse
- ScratchDir
- loopWithHits
- .latencyFrames

## God Nodes (most connected - your core abstractions)
1. `Project` - 296 edges
2. `EntityId` - 263 edges
3. `Command` - 161 edges
4. `TempoMap` - 90 edges
5. `AudioEngine` - 70 edges
6. `AudioBufferPool` - 69 edges
7. `BuiltinEffect` - 68 edges
8. `AudioFileData` - 65 edges
9. `Node` - 65 edges
10. `ProcessContext` - 63 edges

## Surprising Connections (you probably didn't know these)
- `INCDAW` --semantically_similar_to--> `INCDAW project mission`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Clip / Project Data Model` --semantically_similar_to--> `Core data model (no collapsed entities)`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Performance Strategy` --semantically_similar_to--> `Measure-before-optimise performance strategy`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `main()` --calls--> `close`  [INFERRED]
  tools/audiocheck/main.cpp → src/platform/MidiDevice.h
- `Development Phases (0-20)` --semantically_similar_to--> `Phase 0-20 feature roadmap`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Approval-Gated Development Protocol** — claude_absolute_user_control_rule, claude_graphify_mandate, claude_feature_workflow, claude_scope_control, claude_dependency_policy [EXTRACTED 1.00]
- **FL Studio 2026 Gap Closure Plan (P1-P10)** — docs_fl2026_gap_gap_analysis, docs_fl2026_gap_p1_chord_toolkit, docs_fl2026_gap_p2_note_tools, docs_fl2026_gap_p3_split_and_markers, docs_fl2026_gap_p4_sidechain, docs_fl2026_gap_p5_loudness_and_sends, docs_fl2026_gap_p6_modulation_and_transient, docs_fl2026_gap_p7_timestretch, docs_fl2026_gap_p8_slicer, docs_fl2026_gap_p9_browser, docs_fl2026_gap_p10_au_hosting [EXTRACTED 1.00]
- **Hostile-Plugin Survival Contract** — docs_plugin_host_hostile_input_prime_directive, docs_plugin_host_isolation_strategy, docs_plugin_host_crash_policy, docs_plugin_host_plugin_blacklist, docs_plugin_host_plugin_validation, docs_plugin_host_missing_plugin_placeholder, docs_testing_plugin_misbehaviour_matrix [EXTRACTED 1.00]
- **INCDAW six-library layer stack (ui -> app -> project -> engine -> platform, plugins beside engine)** — src_cmakelists_incdaw_platform, src_cmakelists_incdaw_engine, src_cmakelists_incdaw_plugins, src_cmakelists_incdaw_project, src_cmakelists_incdaw_app, src_cmakelists_incdaw, src_cmakelists_layer_dependency_rule [EXTRACTED 1.00]
- **Plugin Host Pipeline: scan → registry → instance → graph node** — docs_plugin_host_pluginscanner, docs_plugin_host_pluginregistry, docs_plugin_host_plugininstance, docs_plugin_host_plugininstancemanager, docs_plugin_host_pluginnode [EXTRACTED 1.00]
- **Project Format Durability Guarantees** — docs_project_format_determinism, docs_project_format_version_fixtures, docs_project_format_migration_chain, docs_project_format_missing_media_handling, docs_testing_fuzzing, docs_project_format_format_tests [EXTRACTED 1.00]
- **Stopped-transport rendering contract (the idle-hum fix)** — changelog_idle_hum_defect, changelog_processcontext_playing, changelog_instrumentnode, changelog_audioclipnode, changelog_automationnode [EXTRACTED 1.00]
- **Stopped-Transport Silence Contract** — docs_audio_engine_transport, docs_audio_engine_processcontext_playing, docs_audio_engine_instrumentnode, docs_audio_engine_audioclipnode, docs_audio_engine_automationnode [EXTRACTED 1.00]
- **Plugin hosting pipeline: scan, catalogue, instantiate, render, persist** — changelog_scanoutofprocess, changelog_pluginregistry, changelog_plugininstancemanager, changelog_clapinstance, changelog_audiounitinstance, changelog_hostedplugin, changelog_pluginnode, changelog_stateio, changelog_parametersink [INFERRED 0.85]
- **Mechanically Enforced Realtime and Layering Safety** — docs_audio_engine_prime_directive, docs_audio_engine_realtime_guard, docs_audio_engine_denormal_handling, docs_architecture_threading_model, docs_architecture_layering_test, docs_architecture_reaper_queue [INFERRED 0.85]

## Communities (336 total, 10 thin omitted)

### Community 0 - "MixerCommands.cpp"
Cohesion: 0.03
Nodes (67): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType (+59 more)

### Community 1 - "Browser"
Cohesion: 0.07
Nodes (63): directory_entry, Browser, addDefaultRoots, addRoot, canDecodeAudio, classify, clear, defaultSearchLimit (+55 more)

### Community 2 - "SetVelocityCommand"
Cohesion: 0.13
Nodes (9): string, SetVelocityCommand, canMergeWith, channel_, indices_, mergeWith, pattern_, previousVelocities_ (+1 more)

### Community 3 - "WavStreamWriter"
Cohesion: 0.05
Nodes (59): ofstream, appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample (+51 more)

### Community 4 - "WriteAutomationCommand"
Cohesion: 0.05
Nodes (42): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, AutomationPoint, size_t (+34 more)

### Community 5 - "ApplyInsertPresetCommand"
Cohesion: 0.11
Nodes (23): ApplyInsertPresetCommand, after_, before_, execute, presetName_, undo, write, writer_ (+15 more)

### Community 6 - "LoudnessMeterEffect"
Cohesion: 0.08
Nodes (22): Biquad, FrameCount, LoudnessMeterEffect, highpass_, histogramBins, histogramCount_, histogramPower_, hopCount_ (+14 more)

### Community 7 - "PatternCommands.cpp"
Cohesion: 0.04
Nodes (43): AddPatternCommand, execute, index_, minted_, pattern_, undo, DuplicatePatternCommand, execute (+35 more)

### Community 8 - "TestGainPlugin.cpp"
Cohesion: 0.06
Nodes (57): clap_gui_resize_hints_t, clap_id, clap_param_info_t, clap_window_t, applyParamEvents(), clap_host_t, clap_input_events_t, clap_istream_t (+49 more)

### Community 9 - "PianoRollModel"
Cohesion: 0.05
Nodes (58): NoteList, size_t, Tick, vector, Viewport, size_t, Tick, vector (+50 more)

### Community 10 - "AudioEngine"
Cohesion: 0.07
Nodes (51): RetiredGraph, AudioCaptureSink, AudioEngine, active_, anchor_, anchorVersion_, audioDeviceAboutToStart, audioDeviceStopped (+43 more)

### Community 11 - "Command"
Cohesion: 0.05
Nodes (17): string, vector, Command, execute, id, name, undo, CommandRegistry (+9 more)

### Community 12 - "PluginInstanceManager"
Cohesion: 0.06
Nodes (39): clap_plugin_entry_t, Held, ClapLibrary, entry_, factory_, library_, clap_plugin_factory_t, size_t (+31 more)

### Community 13 - "ProcessContext"
Cohesion: 0.13
Nodes (37): dbToGain(), sumInputsInto(), process, coefficientFor(), process, size_t, process, process (+29 more)

### Community 14 - "MultibandCompressorEffect"
Cohesion: 0.06
Nodes (33): DeEsserEffect, cachedHz_, channels_, envelope_, highpass_, lowpass_, maxChannels, reduction_ (+25 more)

### Community 15 - "Transport"
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, uint32_t (+17 more)

### Community 16 - "AddInsertCommand"
Cohesion: 0.06
Nodes (33): AddInsertCommand, append, execute, index_, minted_, mixerNode_, plugin_, slot_ (+25 more)

### Community 17 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 18 - "Sampler"
Cohesion: 0.06
Nodes (27): array, atomic, maxVoices, ParameterSink, SampleRate, size_t, uint64_t, vector (+19 more)

### Community 19 - "AddMarkerCommand"
Cohesion: 0.06
Nodes (32): AddMarkerCommand, execute, index_, length_, marker_, minted_, tick_, undo (+24 more)

### Community 20 - "BiquadCoefficients"
Cohesion: 0.08
Nodes (26): complex, BiquadSection, z1, z2, butterworthHighpass(), butterworthLowpass(), SampleRate, LinkwitzRileyHalf (+18 more)

### Community 21 - "SetNoteLabelCommand"
Cohesion: 0.12
Nodes (14): NoteIndices, string, vector, LegatoNotesCommand, channel_, indices_, pattern_, previousDurations_ (+6 more)

### Community 22 - "parseReleaseFeed"
Cohesion: 0.15
Nodes (19): automaticCheckIsDue(), int64_t, optional, size_t, string, vector, evaluateFeed(), newestRelease() (+11 more)

### Community 23 - "AuditionPlayer"
Cohesion: 0.05
Nodes (35): Retired, AuditionPlayer, collect, current_, gain_, generation_, play, playing_ (+27 more)

### Community 24 - "AudioStream"
Cohesion: 0.10
Nodes (24): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+16 more)

### Community 25 - "LookaheadLimiterEffect"
Cohesion: 0.05
Nodes (42): CompressorEffect, envelope_, prepare, reduction_, sampleRate_, FrameCount, SampleRate, GateEffect (+34 more)

### Community 26 - "PluginInsertTests.cpp"
Cohesion: 0.18
Nodes (13): anyNonZero(), ClipInsert, threshold_, FrameCount, Sample, vector, GainInsert, factor_ (+5 more)

### Community 27 - "GraphBuilder"
Cohesion: 0.10
Nodes (25): Connection, NodeIndex, SampleRate, size_t, unique_ptr, GraphBuilder, addNode, analyse (+17 more)

### Community 28 - "StretchAssetCommand"
Cohesion: 0.06
Nodes (27): DeleteAudioRegionCommand, applied_, asset_, minted_, region_, removed_, FrameCount, Sample (+19 more)

### Community 29 - "PresetBar.mm"
Cohesion: 0.10
Nodes (22): NSMenuItem, NSString, NSView, INCDAWPresetBar, +attachToWindow, +barInWindow, -refreshAppearance, +refreshAppearanceInWindow (+14 more)

### Community 30 - "AudioRecorder"
Cohesion: 0.07
Nodes (29): AudioCaptureSink, AudioRecorder, captureAudioBlock, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_ (+21 more)

### Community 31 - "CommandRegistry"
Cohesion: 0.10
Nodes (28): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+20 more)

### Community 32 - "NudgeChordCommand"
Cohesion: 0.09
Nodes (26): vector, findEvents(), Scale, size_t, string, vector, InsertNotesCommand, channel_ (+18 more)

### Community 33 - "atomic"
Cohesion: 0.06
Nodes (20): OperatorOffset, array, atomic, MidiBuffer, forPad(), PadOffset, uint32_t, forOperator() (+12 more)

### Community 34 - "InstrumentNode"
Cohesion: 0.11
Nodes (16): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, ParameterSink, unique_ptr, InstrumentNode (+8 more)

### Community 35 - "TestLatencyPlugin.cpp"
Cohesion: 0.10
Nodes (31): clap_host_t, clap_istream_t, clap_ostream_t, clap_plugin_descriptor_t, clap_plugin_factory_t, clap_plugin_t, clap_process_status, clap_process_t (+23 more)

### Community 36 - "MixerTests.cpp"
Cohesion: 0.09
Nodes (21): FrameCount, FramePosition, Sample, SampleRate, size_t, vector, ImpulseNode, latency_ (+13 more)

### Community 37 - "INCDAW"
Cohesion: 0.14
Nodes (34): Audio Editor, Audio Engine, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts, Content / Sound Library, Controller Linking (+26 more)

### Community 38 - "ChannelRackModel"
Cohesion: 0.13
Nodes (32): Hit, centred(), ChannelRackModel, contentHeight, contentLeft, hitTest, knobDragTravel, knobForDrag (+24 more)

### Community 39 - "WavStreamReader"
Cohesion: 0.08
Nodes (29): ifstream, FrameCount, path, Result, Sample, size_t, FrameCount, path (+21 more)

### Community 40 - "TempoMap"
Cohesion: 0.09
Nodes (33): execute, execute, execute, clampTempo(), FramePosition, SampleRate, Tick, vector (+25 more)

### Community 41 - "FlangerEffect"
Cohesion: 0.06
Nodes (40): ChorusEffect, centreMs, line_, mask_, maxChannels, maxDepthMs, phase_, prepare (+32 more)

### Community 42 - "AudioUnitInstance"
Cohesion: 0.08
Nodes (30): AudioUnitHandle, closeEditor, hasEditor, latencyFrames, open, openEditor, process, restoreState (+22 more)

### Community 43 - "CompiledProjectGraph"
Cohesion: 0.07
Nodes (33): CompiledProjectGraph, automation, builtInserts, builtSlots, channels, channelStripFor, channelStrips, error (+25 more)

### Community 44 - "MusicTheory.cpp"
Cohesion: 0.12
Nodes (32): ChordDetection, bassKey, display, inverted, matched, rootPitchClass, type, ChordType (+24 more)

### Community 45 - "EqEffect"
Cohesion: 0.09
Nodes (22): FrameCount, SampleRate, designEqBands(), EqEffect, bandCount, cached_, coefficients_, maxChannels (+14 more)

### Community 46 - "INCDAWMixerView"
Cohesion: 0.09
Nodes (53): incdaw, NSDraggingDestination, NSArray, NSDictionary, NSView, INCDAWMixerView, -acceptsFirstResponder, -addStripRect (+45 more)

### Community 47 - "RemoveMixerNodeCommand"
Cohesion: 0.09
Nodes (17): RemovedRouting, DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t (+9 more)

### Community 48 - "ClapInstance"
Cohesion: 0.07
Nodes (26): clap_plugin_gui_t, clap_plugin_latency_t, clap_plugin_params_t, clap_plugin_state_t, ParamEvent, ClapInstance, editorOpen_, gui_ (+18 more)

### Community 49 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 50 - "AudioLogger"
Cohesion: 0.09
Nodes (22): AudioLogger, capacityFrames_, circle_, enabled_, grab, log, prepare, ready_ (+14 more)

### Community 51 - "read"
Cohesion: 0.08
Nodes (39): appendBigU16(), appendBigU32(), appendChunk(), appendVlq(), path, Result, size_t, Tick (+31 more)

### Community 52 - "AddSamplerZoneCommand"
Cohesion: 0.13
Nodes (10): AddSamplerZoneCommand, asset_, assetId_, assetIndex_, channelId_, created_, minted_, path_ (+2 more)

### Community 53 - "SetTempoCommand"
Cohesion: 0.07
Nodes (28): string, vector, SetTempoCommand, canMergeWith, captured_, clampTempo, execute, maximumTempo (+20 more)

### Community 54 - "WavetableSynth"
Cohesion: 0.06
Nodes (53): Envelope, LfoShape, FrameCount, SampleRate, Settings, size_t, uint32_t, WavetableParam (+45 more)

### Community 55 - "Wavetable"
Cohesion: 0.05
Nodes (52): Allpass, Comb, FrameCount, SampleRate, DelayEffect, capacity_, lines_, maxChannels (+44 more)

### Community 56 - "PluginRegistry"
Cohesion: 0.13
Nodes (23): Library, Located, int64_t, path, size_t, string, uint64_t, vector (+15 more)

### Community 57 - "ImportAudioClipCommand"
Cohesion: 0.09
Nodes (17): size_t, string, Tick, ImportAudioClipCommand, clip_, clipIndex_, import_, minted_ (+9 more)

### Community 58 - "The work, in order"
Cohesion: 0.11
Nodes (17): A10 — parametric EQ, more bands and a draggable curve, A11 — convolution reverb, A12/A13 — vocoder, and time/volume gating, A1 — wavetable synth, A2 — FM synth, A4 — drum machine / pad instrument, A5 — the preset system (do this first), A6 — multiband compressor (+9 more)

### Community 59 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 60 - "Json.cpp"
Cohesion: 0.19
Nodes (22): size_t, string, escapeInto(), formatDouble(), asString, contains, dump, dumpTo (+14 more)

### Community 61 - "RecordingSession"
Cohesion: 0.09
Nodes (20): path, Placement, string, vector, FrameCount, FramePosition, uint32_t, uint64_t (+12 more)

### Community 62 - "PlaylistView.mm"
Cohesion: 0.11
Nodes (26): -acceptsFirstResponder, -addTrackRect, -draggingEntered, -draggingExited, -draggingUpdated, -drawAutomationCurveForinBody, -drawBarLinesInLaneAtheight, -drawClips (+18 more)

### Community 63 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 64 - "GraphCompileOptions"
Cohesion: 0.08
Nodes (26): PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, insertFactory, instrumentFactory, masterGain, maxBlockSize (+18 more)

### Community 65 - "detectOnsets"
Cohesion: 0.50
Nodes (3): FrameCount, vector, detectOnsets()

### Community 66 - "-initWithFramebrowser"
Cohesion: 0.13
Nodes (17): app, GroupKind, NSMutableArray, NSOutlineView, NSTableColumn, NSTableRowView, INCDAWBrowserNode, INCDAWBrowserRowView (+9 more)

### Community 67 - "Clip"
Cohesion: 0.05
Nodes (39): ClipType, AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id (+31 more)

### Community 68 - "INCDAWAppDelegate"
Cohesion: 0.08
Nodes (21): NSApplicationDelegate, NSMenuDelegate, NSOutlineViewDataSource, NSOutlineViewDelegate, NSView, INCDAWBrowserView, -initWithFramebrowser, -reload (+13 more)

### Community 69 - "AppSettings"
Cohesion: 0.12
Nodes (22): AppSettings, appearance, audio, currentVersion, fromJson, load, midiInputIdentifiers, openInputAtLaunch (+14 more)

### Community 70 - "read"
Cohesion: 0.23
Nodes (24): assetFilePath(), FrameCount, Sample, string, vector, execute, undo, execute (+16 more)

### Community 71 - "AddMidiMappingCommand"
Cohesion: 0.08
Nodes (21): AddMidiMappingCommand, controller_, mapping_, midiChannel_, minted_, parameterKey_, target_, size_t (+13 more)

### Community 72 - "MusicalPosition"
Cohesion: 0.12
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 73 - "MidiMessage"
Cohesion: 0.10
Nodes (10): FrameCount, friend, uint8_t, MidiMessage, data1, data2, frameOffset, status (+2 more)

### Community 74 - "HostedPlugin"
Cohesion: 0.09
Nodes (14): ParameterSink, StateIO, uint32_t, HostedPlugin, closeEditor, hasEditor, latencyFrames, openEditor (+6 more)

### Community 75 - "INCDAWCommandPaletteView"
Cohesion: 0.14
Nodes (22): NSPanel, NSObject, NSString, INCDAWCommandEntry, +entryWithTitlecategoryshortcutrun, INCDAWCommandPalette, -close, -dealloc (+14 more)

### Community 76 - "ChannelSamplerZone"
Cohesion: 0.14
Nodes (14): ChannelSamplerZone, asset, end, gain, keyHigh, keyLow, loopCrossfade, loopEnd (+6 more)

### Community 77 - "CallbackProfiler"
Cohesion: 0.11
Nodes (14): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+6 more)

### Community 78 - "Sampler.cpp"
Cohesion: 0.15
Nodes (22): FrameCount, Sample, SampleRate, vector, Voice, interpolate(), activeVoiceCount, allNotesOff (+14 more)

### Community 79 - "SimpleSynth"
Cohesion: 0.06
Nodes (38): FrameCount, SampleRate, size_t, uint32_t, Voice, Waveform, frequencyForKey(), array (+30 more)

### Community 80 - "SetSamplerZoneCommand"
Cohesion: 0.16
Nodes (14): execute, undo, string, ensureAssetForFile(), undo, SetSamplerZoneCommand, canMergeWith, channelId_ (+6 more)

### Community 81 - "WaveformOverview"
Cohesion: 0.11
Nodes (17): Bucket, FrameCount, SampleRate, size_t, vector, WaveformOverview, channelCount, channels (+9 more)

### Community 82 - "ParameterFixture"
Cohesion: 0.14
Nodes (15): anyNonZero(), pair, ParameterSink, Sample, uint32_t, vector, ParameterFixture, channel (+7 more)

### Community 83 - "MidiInput"
Cohesion: 0.14
Nodes (13): atomic, queueCapacity, size_t, uint64_t, MidiInput, dropped_, hasPending_, lastControl_ (+5 more)

### Community 84 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 85 - "ControlBarView.mm"
Cohesion: 0.11
Nodes (21): NSTextField, -beginTypingTempo, -controlTextDidEndEditing, -controltextViewdoCommandBySelector, -drawDisplay, -drawRect, -endTypingTempoCommitting, -initWithFrame (+13 more)

### Community 86 - "DrumMachine"
Cohesion: 0.06
Nodes (47): Engine, FrameCount, SampleRate, Settings, size_t, uint32_t, Voice, decayAt() (+39 more)

### Community 87 - "AutomationWriteSession"
Cohesion: 0.13
Nodes (16): AutomationWriteSession, capture, closedSegments_, enabled_, finish, gestureEnded, streams_, AutomationPoint (+8 more)

### Community 88 - "InsertRecordedTakeCommand"
Cohesion: 0.10
Nodes (16): Placement, size_t, string, vector, InsertRecordedTakeCommand, asset_, assetIndex_, clipIndices_ (+8 more)

### Community 89 - "ParameterRegistry"
Cohesion: 0.18
Nodes (19): Applier, convertParameters(), Entry, size_t, string, uint32_t, vector, Entry (+11 more)

### Community 90 - "RenderOptions"
Cohesion: 0.10
Nodes (21): BitDepth, FramePosition, function, SampleRate, uint64_t, RenderOptions, bitDepth, blockSize (+13 more)

### Community 91 - "project::compileProjectGraph"
Cohesion: 0.11
Nodes (22): engine/audio/AudioClipNode, engine/audio/AudioStream + DiskStreamer (D-025), Export Audio options dialog, engine::InstrumentNode, EBU R128 loudness (incdaw.loudness), engine/dsp/MixerStripNode, MoveInsertCommand (insert reordering), project::renderProject / OfflineRender (+14 more)

### Community 92 - "FmSynth"
Cohesion: 0.06
Nodes (45): FrameCount, SampleRate, Settings, size_t, uint32_t, fastSine(), fmParameterCount(), FmSynth (+37 more)

### Community 93 - "AudioBufferPool"
Cohesion: 0.10
Nodes (19): AudioBufferPool, channelPointers_, reset, samples_, FrameCount, Sample, size_t, unique_ptr (+11 more)

### Community 94 - "WaveshaperEffect"
Cohesion: 0.08
Nodes (30): shaperPointCount, array, FrameCount, halfbandTaps, SampleRate, designHalfband(), array, halfbandTaps (+22 more)

### Community 95 - "InputMonitorNode"
Cohesion: 0.15
Nodes (9): FrameCount, Sample, SampleRate, size_t, vector, InputMonitorNode, channelCount_, ring_ (+1 more)

### Community 96 - "PluginIdentifier"
Cohesion: 0.10
Nodes (18): builtinPiano(), builtinSampler(), builtinSimpleSynth(), Format, string, formatName(), Format, friend (+10 more)

### Community 97 - "ThemePalette.cpp"
Cohesion: 0.06
Nodes (66): members_, Entry, path, string, string_view, vector, path, ThemeLibrary (+58 more)

### Community 98 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 99 - "INCDAW macOS app bundle target"
Cohesion: 0.13
Nodes (19): app::AppSettings, Audio and MIDI Settings window (Cmd+,), engine::AuditionPlayer (browser preview), app::Browser + INCDAWBrowserView, Piano Roll chord toolkit (app::music), Command Search palette (Cmd+K), ui/macos/ControlBarView, MIDI hardware link (MidiDevice -> MidiInput) (+11 more)

### Community 100 - "Audio Correctness Requirements"
Cohesion: 0.12
Nodes (19): Absolute User Control Rule, Audio Correctness Requirements, Decision Log, Definition of Done, Dependency Policy, Development Phases (0-20), Required Documentation Set, Required Feature Workflow (Steps A-E) (+11 more)

### Community 101 - "FL Studio 2026 Functional Reference"
Cohesion: 0.13
Nodes (19): Insert Chain Compilation, Pre-Fader Inserts (D-028), INCDAW Project Format v1.x, macOS Audio Workgroups API Finding, Clip Gain / Pan / Normalize as Clip Properties, FL Studio 2026 Functional Reference, Multi-Output Plugin Nodes Requirement, Extensible Per-Note Property Slot (+11 more)

### Community 102 - "SamplerZoneStream"
Cohesion: 0.12
Nodes (19): Slot, uint64_t, array, FrameCount, shared_ptr, size_t, SamplerZoneStream, claimSlot (+11 more)

### Community 103 - "PluginPickerModel"
Cohesion: 0.05
Nodes (60): NSDraggingItem, NSDraggingSource, NSImage, NSPasteboardItem, NSSearchFieldDelegate, categoryFor(), size_t, string (+52 more)

### Community 104 - "PlaylistModel"
Cohesion: 0.07
Nodes (42): Rect, size_t, Tick, vector, size_t, Tick, vector, Viewport (+34 more)

### Community 105 - "PianoRollHeaderModel"
Cohesion: 0.07
Nodes (31): Snap, Layout, Rect, Scale, size_t, Tick, PianoRollHeaderModel, layout_ (+23 more)

### Community 106 - "AudioBufferView"
Cohesion: 0.16
Nodes (11): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t, Sample (+3 more)

### Community 107 - "ImpulseNode"
Cohesion: 0.40
Nodes (3): FrameCount, ImpulseNode, at_

### Community 108 - "SamplerZone"
Cohesion: 0.11
Nodes (18): FrameCount, shared_ptr, handleMessage, SamplerZone, end, gain, keyHigh, keyLow (+10 more)

### Community 109 - "DelayLineNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_, prepare (+6 more)

### Community 110 - "StereoImagerEffect"
Cohesion: 0.08
Nodes (23): FrameCount, SampleRate, array, atomic, ChannelState, SampleRate, size_t, StereoImagerEffect (+15 more)

### Community 111 - "load"
Cohesion: 0.21
Nodes (20): automationPointFrom(), bindUnassignedContent(), AutomationPoint, path, Result, string, idFrom(), midiEventFrom() (+12 more)

### Community 112 - "FuzzTests.cpp"
Cohesion: 0.16
Nodes (13): corrupt(), path, size_t, string, uint64_t, uint8_t, vector, Random (+5 more)

### Community 113 - "incdaw_tests suite target"
Cohesion: 0.18
Nodes (14): plugins::AudioUnitInstance / platform::AudioUnitHost, plugins::HostedPlugin interface, plugins::PluginRegistry (catalogue + blacklist), plugins::scanOutOfProcess, incdaw-pluginscan out-of-process scanner, Plugin host architecture pipeline, Scanner rides inside INCDAW.app, CLAP header inclusion discipline (+6 more)

### Community 114 - "write"
Cohesion: 0.19
Nodes (16): int32_t, AiffFile, write, appendBigU16(), appendBigU32(), appendExtended(), appendId(), Format (+8 more)

### Community 115 - "CoreMidiDevice"
Cohesion: 0.14
Nodes (15): MIDIClientRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_, client_, close, handlePackets (+7 more)

### Community 116 - "SliceAssetCommand"
Cohesion: 0.12
Nodes (14): FrameCount, size_t, string, vector, SliceAssetCommand, asset_, channel_, channelIndex_ (+6 more)

### Community 117 - "AudioUnitParameterDescription"
Cohesion: 0.15
Nodes (13): AudioUnitDescription, isInstrument, manufacturer, name, uid, AudioUnitParameterDescription, defaultValue, id (+5 more)

### Community 118 - "SampleRingBuffer"
Cohesion: 0.21
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 119 - "MixerStripNode"
Cohesion: 0.10
Nodes (23): FrameCount, Sample, SampleRate, atomic, Sample, MixerStripNode, left_, meter_ (+15 more)

### Community 120 - "AudioAssetImport"
Cohesion: 0.15
Nodes (16): AudioAssetImport, asset, created, id, index, string, size_t, importAudioAsset() (+8 more)

### Community 121 - "MidiMapNode"
Cohesion: 0.25
Nodes (5): Binding, size_t, vector, MidiMapNode, bindings_

### Community 122 - "SystemInfo"
Cohesion: 0.14
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 123 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 124 - "PianoInstrument"
Cohesion: 0.06
Nodes (27): PianoModel, uint32_t, array, atomic, maxVoices, ParameterSink, SampleRate, uint64_t (+19 more)

### Community 125 - "EntityId"
Cohesion: 0.07
Nodes (41): NoteIndices, EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, colourForIndex() (+33 more)

### Community 126 - "allocate"
Cohesion: 0.08
Nodes (26): allocate, FrameCount, size_t, FramePosition, FrameCount, Sample, vector, renderBlock() (+18 more)

### Community 127 - "SplitFixture"
Cohesion: 0.12
Nodes (14): CommandRegistry, path, string, Tick, note(), ScratchDirectory, path, SplitFixture (+6 more)

### Community 128 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 129 - "SamplerTests.cpp"
Cohesion: 0.18
Nodes (17): constantSample(), FrameCount, MidiBuffer, Sample, shared_ptr, vector, makeEnvelopeTransparent(), nyquistSample() (+9 more)

### Community 130 - "Options"
Cohesion: 0.12
Nodes (17): int64_t, string, Options, amplitude, buffer, device, frequency, input (+9 more)

### Community 131 - "TimingProbeInstrument"
Cohesion: 0.14
Nodes (10): Applied, FrameCount, MidiBuffer, Sample, SampleRate, vector, renderSynth(), TimingProbeInstrument (+2 more)

### Community 132 - "ioProcTrampoline"
Cohesion: 0.21
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 133 - "MidiRecorder"
Cohesion: 0.14
Nodes (14): CapturedMessage, FramePosition, MidiBuffer, atomic, queueCapacity, size_t, uint64_t, MidiRecorder (+6 more)

### Community 134 - "CoreMidiDevice.cpp"
Cohesion: 0.35
Nodes (11): CFStringRef, MIDIEndpointRef, MIDIObjectRef, enumerateInputs, enumerateOutputs, open, string, vector (+3 more)

### Community 135 - "app::CommandRegistry"
Cohesion: 0.13
Nodes (17): app::ChannelRackModel, Command Architecture (every action is a command object), Command Palette, app::CommandRegistry, INCDAW Project Data Model, app::PianoRollModel, app::registerStandardActions, Stable 64-bit Entity Identity (+9 more)

### Community 136 - "ParametricEqEffect"
Cohesion: 0.06
Nodes (40): parametricBandCount, bandIndex(), array, bandCount, BandOffset, FrameCount, ParametricBandType, SampleRate (+32 more)

### Community 137 - "AudioClipNode"
Cohesion: 0.13
Nodes (13): AudioClipNode, addClip, clips_, fetchScratch_, prepare, process, FrameCount, PlacedClip (+5 more)

### Community 138 - "SamplerWiringTests.cpp"
Cohesion: 0.19
Nodes (10): path, string, noteAtZero(), SamplerProject, asset, channel, project, ScratchDirectory (+2 more)

### Community 139 - "LevelMeter"
Cohesion: 0.15
Nodes (12): atomic, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond, rmsWindowSeconds (+4 more)

### Community 140 - "LockFreeQueue"
Cohesion: 0.12
Nodes (13): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+5 more)

### Community 141 - "Channel"
Cohesion: 0.04
Nodes (46): Channel, colour, id, instrument, instrumentParameters, instrumentStateFile, muted, name (+38 more)

### Community 142 - "Project"
Cohesion: 0.06
Nodes (48): execute, undo, AutomationPoint, vector, findLane(), execute, undo, sortPoints() (+40 more)

### Community 143 - "GainNode"
Cohesion: 0.15
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 144 - "SineOscillatorNode"
Cohesion: 0.13
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 145 - "PresetLibrary.cpp"
Cohesion: 0.23
Nodes (29): Entry, optional, path, string, string_view, vector, defaultPresetValues(), factoryPresetsFor() (+21 more)

### Community 146 - "Json"
Cohesion: 0.11
Nodes (15): nullptr_t, int64_t, int64_t, pair, string, vector, Json, asBool (+7 more)

### Community 147 - "INCDAWShaperView"
Cohesion: 0.15
Nodes (20): NSObject, INCDAWShaperPanel, +makePanelWithTitlerowsonWrite, +refreshAppearance, +refreshWindowvalues, -sliderMoved, INCDAWShaperView, -drawRect (+12 more)

### Community 148 - "EditFixture"
Cohesion: 0.15
Nodes (13): FrameCount, path, Sample, size_t, EditFixture, assetId, file, project (+5 more)

### Community 149 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 150 - "BuiltinEffectTests.cpp"
Cohesion: 0.19
Nodes (15): FrameCount, Sample, size_t, vector, processThrough(), RefAllpass, index, line (+7 more)

### Community 151 - "INCDAWEqCurveView"
Cohesion: 0.16
Nodes (20): NSObject, INCDAWEqCurvePanel, +makePanelWithTitlerowssampleRateonWrite, +refreshAppearance, +refreshWindowvalues, INCDAWEqCurveView, -bandNear, -drawRect (+12 more)

### Community 152 - "MultibandTests.cpp"
Cohesion: 0.50
Nodes (8): Sample, size_t, vector, mix(), peakOf(), processThrough(), rmsOf(), sine()

### Community 153 - "MidiEvent"
Cohesion: 0.07
Nodes (41): map, execute, undo, chordGroups(), NoteIndices, Tick, vector, findEvents() (+33 more)

### Community 154 - "AutomationPoint"
Cohesion: 0.17
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 155 - "plugins::ClapInstance"
Cohesion: 0.22
Nodes (15): engine::AutomationNode, app/AutomationWriteSession, engine::BuiltinEffect (Node + ParameterSink + StateIO), plugins::ClapInstance, Plugin delay compensation (PDC), ui/macos/InsertParameterPanel, Limiter (Lookahead) builtin effect, engine::MidiMapNode (+7 more)

### Community 156 - ".incdaw Package Directory"
Cohesion: 0.16
Nodes (15): Prime Directive: Third-Party Plugins Are Hostile Input, Missing-Plugin Placeholder Retains State, Plugin Blacklist, Plugin State System, Plugin Validation Checks, PluginRegistry, PluginScanner, project/PluginStateFiles (+7 more)

### Community 157 - "Permanent Version Fixtures (tests/fixtures)"
Cohesion: 0.17
Nodes (15): Save Determinism (byte-identical re-save), Phase 4 Format Test Gate, INCDAW's Own JSON Writer, manifest.json, Migration Chain (v1.0 → v1.1 → v2.0), ProjectFile::migrate, Permanent Version Fixtures (tests/fixtures), Format Version History (1.0–1.5) (+7 more)

### Community 158 - "PianoInstrument.cpp"
Cohesion: 0.17
Nodes (20): array, FrameCount, SampleRate, size_t, Voice, decayCoefficient(), frequencyForKey(), makeSineTable() (+12 more)

### Community 159 - "Fft"
Cohesion: 0.09
Nodes (31): size_t, Fft, forward, reversed_, setSize, twiddleCos_, twiddleSin_, size_t (+23 more)

### Community 160 - "ChannelRackView.mm"
Cohesion: 0.29
Nodes (11): -acceptsFirstResponder, -channelCount, -currentPattern, -drawChannelspatternlastStepplayheadStep, -drawRect, -drawRulerplayheadStep, -hitForEvent, -initWithFrameprojectregistry (+3 more)

### Community 161 - "PluginPersistenceTests.cpp"
Cohesion: 0.14
Nodes (11): path, string, uint8_t, vector, gainBlob(), Harness, folder, registry (+3 more)

### Community 162 - "SidechainFixture"
Cohesion: 0.14
Nodes (10): ConstantSourceInsert, level_, CommandRegistry, SidechainFixture, bassStrip, compressorSlot, kickStrip, project (+2 more)

### Community 163 - "Plugin Misbehaviour Matrix"
Cohesion: 0.18
Nodes (14): Audio Units (v2 + v3) Support, Plugin Crash Policy, Out-of-Process Isolation Strategy, INCDAW Plugin Host, Shared-Memory Audio Ring Buffer (host boundary), VST2 Exclusion, VST3 Format Support, Audio Logger (60 s master ring buffer) (+6 more)

### Community 164 - "ChannelCommands.cpp"
Cohesion: 0.03
Nodes (73): RemovedContent, AddChannelCommand, channel_, execute, index_, minted_, undo, size_t (+65 more)

### Community 165 - "ClapLibrary.cpp"
Cohesion: 0.18
Nodes (14): blobRead(), closeEditor, refreshLatencyIfChanged, close, open, clap_host_t, clap_istream_t, path (+6 more)

### Community 166 - "AudioFileData"
Cohesion: 0.12
Nodes (34): applyGain(), applyRamp(), clampedRegion(), FramePosition, Sample, deleteRegion(), extractRegion(), fadeIn() (+26 more)

### Community 167 - "Realtime Safety Guard (debug builds)"
Cohesion: 0.17
Nodes (13): Headless Deterministic Framework-Free Core, Downward-Only Layer Model (ui/app/project/engine/plugins/platform), Automated Layering Test, Denormal Handling (FTZ/DAZ at callback entry), Audio Thread Prime Directive, Realtime Safety Guard (debug builds), D-001 — Core implementation language: C++20, D-025 — Streaming is a window, and starving it is audible, not fatal (+5 more)

### Community 168 - "RenderGraph (compiled immutable graph)"
Cohesion: 0.15
Nodes (13): Offline/Realtime Render Equivalence, RenderGraph (compiled immutable graph), AudioClipNode, AutomationNode, Sample-Accurate Block Splitting at Event Boundaries, InstrumentNode, Offline Rendering Pipeline, ProcessContext::playing (+5 more)

### Community 169 - "INCDAW Testing Strategy"
Cohesion: 0.15
Nodes (13): Scriptable Command Surface Evidence, INCDAW Requirements, Testable Exit Criterion, Phase 15 — Built-in DSP, Phase 16 — MIDI Hardware and Controller Linking, Phase 17 — Rendering and Export, Phase 18 — Performance, Phase 19 — QA (+5 more)

### Community 170 - "ConvolutionReverbEffect"
Cohesion: 0.05
Nodes (38): Impulse, ConvolutionReverbEffect, accumulateImaginary_, accumulateReal_, binCount, dampState_, fdl_, fdlCursor_ (+30 more)

### Community 171 - "SplitClipCommand"
Cohesion: 0.22
Nodes (7): SplitClipCommand, clip_, minted_, previous_, right_, splitTick_, undo

### Community 172 - "RemoveTrackCommand"
Cohesion: 0.04
Nodes (40): AddTrackCommand, execute, index_, minted_, track_, undo, RemovedClip, size_t (+32 more)

### Community 173 - "PluginParameterInfo"
Cohesion: 0.15
Nodes (11): ParameterSink, StateIO, string, uint32_t, PluginParameterInfo, defaultValue, id, maxValue (+3 more)

### Community 174 - "AnalyzerEffect"
Cohesion: 0.11
Nodes (17): AnalyzerEffect, accumulate_, accumulated_, binCount, fft_, fftSize, generation_, maxChannels (+9 more)

### Community 175 - "VocoderEffect"
Cohesion: 0.07
Nodes (28): Band, size_t, KeyedEffect, keyInput, noKeyInput, setKeyInput, coefficientFor(), FrameCount (+20 more)

### Community 176 - "INCDAWConvolverPanel"
Cohesion: 0.09
Nodes (21): NSObject, INCDAWConvolverPanel, -chooseImpulse, -clearImpulse, +makePanelWithTitlerowsimpulseonWriteonImpulse, +refreshAppearance, +refreshWindowvaluesimpulse, -sliderMoved (+13 more)

### Community 177 - "renderNode"
Cohesion: 0.21
Nodes (11): FrameCount, path, Sample, shared_ptr, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 178 - "Node"
Cohesion: 0.10
Nodes (20): CatalogueEntry, info, make, function, SampleRate, string, unique_ptr, findBuiltinEffect() (+12 more)

### Community 179 - "Phase 18 Measured Baseline (2026-08-16)"
Cohesion: 0.18
Nodes (12): Theme (single drawn design language for the shell), DelayLineNode, engine::GraphBuilder::compile, Plugin Delay Compensation, D-006 — UI: AppKit shell + INCDAW-owned Metal-rendered widget layer, D-015 — The Channel Rack is drawn with CoreGraphics, not Metal, D-019 — Delay compensation lives in the graph compiler, D-035 — One drawn design language for the shell: FL Studio's density, GarageBand's calm (+4 more)

### Community 180 - "D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded)"
Cohesion: 0.18
Nodes (12): D-002 — Build system: CMake + Ninja, D-003 — Audio I/O: CoreAudio HAL directly, no wrapper framework, D-005 — Platform strategy: macOS first, Windows later, Linux not precluded, D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded), D-008 — Licensing: INCDAW is closed-source, D-011 — Metal shaders are compiled at runtime, not built offline, D-027 — CLAP SDK vendored, pinned at 1.2.6, D-031 — Plugin instances live for their slot's lifetime, not the graph's (+4 more)

### Community 181 - "Phase 1 — Foundation and Build System"
Cohesion: 0.20
Nodes (12): CLAP Format Support (first), Ad-Hoc Signing Instead of Notarization (D-009), tools/make-dmg.sh, Release Notes 0.9.0, INCDAW Release Process, Out-of-Scope Product Decisions, Deliberately Out of Scope, Phase 0 — Research and Architecture (+4 more)

### Community 182 - "UI Build-Out (post-phase increments)"
Cohesion: 0.17
Nodes (12): Plugin Editor / UI Bridge, Plugin Parameter System (D-029), ParameterRegistry (plugin registration), Event-Based Parameter Delivery (ParameterSink), params->flush() Recorded As Not Currently Applicable, Autosave, Backup and Crash Recovery (history/), Increment 11 — Settings, MIDI In, Command Search, Increment 1 — Project Lifecycle Safety (+4 more)

### Community 183 - "BeatGateEffect"
Cohesion: 0.10
Nodes (23): beatGatePoints, beatGateCurveAt(), BeatGateEffect, BeatGateEffect::BeatGateEffect(), history_, historyWrite_, maxChannels, maxHistorySeconds (+15 more)

### Community 184 - "KernelTable"
Cohesion: 0.21
Nodes (9): SampleRate, vector, KernelTable, phases, weights_, width, resample(), sinc() (+1 more)

### Community 185 - "ClapDescriptor"
Cohesion: 0.13
Nodes (17): ClapDescriptor, id, name, vendor, version, string, path, string (+9 more)

### Community 186 - "Pattern"
Cohesion: 0.08
Nodes (38): AutomationCurve, Emit, AutomationPoint, curve, tension, tick, value, Tick (+30 more)

### Community 187 - "incdaw_engine layer library"
Cohesion: 0.13
Nodes (15): engine/dsp/Fft (own radix-2), Compiled graph owns its own tempo map, engine::dsp::MetronomeNode, SetTempoCommand, SetTimeSignatureCommand, Spectrum analyzer (seqlock-published dBFS bins), INCDAW_REALTIME_GUARD option, incdaw_warnings interface target (+7 more)

### Community 188 - "AutomationNode"
Cohesion: 0.11
Nodes (11): AutomationNode, bindings_, tempoMap_, Binding, size_t, vector, controlChange(), path (+3 more)

### Community 189 - "CompiledGraph"
Cohesion: 0.10
Nodes (17): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, process (+9 more)

### Community 190 - "renderArrangement"
Cohesion: 0.23
Nodes (10): FrameCount, path, Sample, size_t, vector, makeAudio(), renderArrangement(), ScratchDir (+2 more)

### Community 191 - "Fixture"
Cohesion: 0.20
Nodes (10): Tick, vector, Fixture, channel, pattern, project, trackA, trackB (+2 more)

### Community 192 - "capturePluginState"
Cohesion: 0.24
Nodes (16): captureBuiltinInsertState(), capturePluginState(), CarriedInsertState, blob, slot, path, string, uint8_t (+8 more)

### Community 193 - "ChildResult"
Cohesion: 0.18
Nodes (10): End, ChildResult, code, end, output, path, string, vector (+2 more)

### Community 194 - "SettingsWindow.mm"
Cohesion: 0.14
Nodes (32): NSAlert, NSButton, NSTabView, NSTabViewItem, -addRowtoContentatYwidth, -applyEditedPalette, -buildAppearancePage, -buildAudioPage (+24 more)

### Community 195 - "INCDAWBeatGateView"
Cohesion: 0.14
Nodes (19): NSObject, INCDAWBeatGatePanel, +makePanelWithTitlerowsonWrite, +refreshAppearance, +refreshWindowvalues, -sliderMoved, INCDAWBeatGateView, -drawCurveintotitle (+11 more)

### Community 196 - "FactoryPresetTable"
Cohesion: 0.18
Nodes (12): FactoryPreset, name, valueCount, values, FactoryPresetTable, count, items, string_view (+4 more)

### Community 197 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 198 - "Release"
Cohesion: 0.15
Nodes (14): string, Release, draft, name, notes, prerelease, tag, url (+6 more)

### Community 199 - "timeStretch"
Cohesion: 0.29
Nodes (9): size_t, vector, detectOnsets(), monoMixOf(), similarityAt(), StretchOptions, pitchSemitones, ratio (+1 more)

### Community 200 - "SharedLibrary"
Cohesion: 0.25
Nodes (7): path, string, SharedLibrary, close, handle_, open, symbol

### Community 201 - "humanizeNoteStarts"
Cohesion: 0.29
Nodes (10): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+2 more)

### Community 202 - "UtilityEffects.cpp"
Cohesion: 0.24
Nodes (7): process, publishSpectrum, readSpectrum, vector, completeHop, integratedLufs, lufsOf()

### Community 203 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 204 - "AddPatternClipCommand"
Cohesion: 0.15
Nodes (11): AddPatternClipCommand, clip_, execute, index_, length_, minted_, pattern_, start_ (+3 more)

### Community 205 - "TonePanel.mm"
Cohesion: 0.27
Nodes (15): INCDAWToneView, -advancedSliderRectAt, -advancedToggleRect, -curveRect, -drawAdvanced, -drawBands, -drawCurve, -drawRect (+7 more)

### Community 206 - "ParsedHeader"
Cohesion: 0.17
Nodes (18): path, Result, size_t, uint16_t, uint32_t, uint8_t, vector, fillMetadata() (+10 more)

### Community 207 - "Fixture"
Cohesion: 0.18
Nodes (9): CommandRegistry, Tick, vector, Fixture, channel, pattern, project, registry (+1 more)

### Community 208 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 209 - "processThrough"
Cohesion: 0.38
Nodes (9): FrameCount, Sample, size_t, vector, processThrough(), requireBitExact(), requireFiniteAndBounded(), rmsOf() (+1 more)

### Community 210 - "CountingCommand"
Cohesion: 0.12
Nodes (8): CountingCommand, counter_, delta_, string, Tick, makeProjectWithNotes(), NoOpCommand, note()

### Community 211 - "PluginFolder"
Cohesion: 0.24
Nodes (7): path, PluginFolder, crash, dir, gain, ScratchDir, path

### Community 212 - "SamplerStreamingTests.cpp"
Cohesion: 0.16
Nodes (13): FrameCount, path, Sample, size_t, string, vector, rmsOver(), ScratchDirectory (+5 more)

### Community 213 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 214 - "Atomic Graph Pointer Swap"
Cohesion: 0.28
Nodes (9): Atomic Graph Pointer Swap, Lock-Free SPSC Command Queue (UI to Audio, Audio to UI), Reaper Queue (off-thread destruction), Four-Class Threading Model, AudioCaptureSink (atomic capture sink pointer), AudioIOCallback::captureAudioBlock, D-023 — Capture is a second clock domain, reconciled by timestamps, D-024 — A take is placed by clock correlation, not by counting (+1 more)

### Community 215 - "D-028 — Hosted plugins reach the graph through an injected factory"
Cohesion: 0.22
Nodes (9): Insert Chain Compilation (inserts -> fader -> pan -> mute -> outputs), MixerStripNode and the compiled mixer, D-028 — Hosted plugins reach the graph through an injected factory, D-029 — Plugin parameters automate through a sink target and an event queue, D-030 — Plugin state travels as blob files inside the package, D-033 — Builtin effects are inserts with the plugin identity, built by the compiler, P4 — Functional Sidechain Routing (compressor external key input), P5 — LUFS Meter (EBU R128), Stereo Separation, True Pre-Fader Sends (+1 more)

### Community 216 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 217 - "PianoModelSpec"
Cohesion: 0.15
Nodes (13): PianoModelSpec, decayScale, fm, fmIndex, fmIndexTime, fmRatio, hammerLevel, hfDecay (+5 more)

### Community 218 - "MidiDeviceInfo"
Cohesion: 0.40
Nodes (5): string, MidiDeviceInfo, identifier, isInput, name

### Community 219 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (8): size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_, step_

### Community 220 - "SendFixture"
Cohesion: 0.14
Nodes (12): ConstantSourceInsert, level_, CommandRegistry, FrameCount, pair, feedSine(), SendFixture, project (+4 more)

### Community 221 - "StereoImagerTests.cpp"
Cohesion: 0.44
Nodes (8): Stereo, size_t, midPair(), midRms(), processThrough(), sidePair(), sideRms(), sum()

### Community 222 - "RemoveSamplerZoneCommand"
Cohesion: 0.22
Nodes (7): size_t, RemoveSamplerZoneCommand, channelId_, execute, removed_, undo, zoneIndex_

### Community 223 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 224 - "BuiltinEffect"
Cohesion: 0.11
Nodes (37): appendF64(), appendString(), appendU32(), BuiltinEffect, BuiltinEffect::BuiltinEffect(), decodeState, decodeStateStrings, encodeState (+29 more)

### Community 225 - "exportArrangement"
Cohesion: 0.25
Nodes (7): size_t, notes_, path, Result, uint64_t, exportArrangement(), importAsPattern()

### Community 226 - "Smoother"
Cohesion: 0.18
Nodes (9): atomic, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds, sampleRate_ (+1 more)

### Community 227 - "main.mm"
Cohesion: 0.20
Nodes (6): string, HttpResponse, body, error, statusCode, -applicationDidFinishLaunching

### Community 228 - "ConnectMixerCommand"
Cohesion: 0.12
Nodes (12): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+4 more)

### Community 229 - "Denormals.h"
Cohesion: 0.39
Nodes (5): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister()

### Community 230 - "AudioDevice (platform device interface)"
Cohesion: 0.29
Nodes (8): app::AppSettings::audio (device configuration), AudioDevice (platform device interface), Audio Logger (60-second master ring buffer), os_workgroup Realtime Thread Scheduling, Separate Input and Output Device Selection, D-004 — Realtime thread scheduling: os_workgroup / Audio Workgroups, D-036 — Machine settings live in their own versioned file, never in the project, FL Studio 2026 Gap Analysis (INCDAW 0.9.0)

### Community 231 - "INCDAWSettingsWindow"
Cohesion: 0.25
Nodes (7): NSWindowDelegate, NSObject, NSString, INCDAWSettingsWindow, -initWithSettingsthemesDirectory, -refreshStatus, -show

### Community 232 - "ConvolutionReverb.cpp"
Cohesion: 0.24
Nodes (14): adoptImpulse, applyStateString, collectStateStrings, generateDefaultImpulse, loadImpulse, processPartition, waitForRenderToPass, pair (+6 more)

### Community 233 - "Fixture"
Cohesion: 0.22
Nodes (8): CommandRegistry, Tick, Fixture, channel, pattern, project, registry, note()

### Community 234 - "Preset"
Cohesion: 0.14
Nodes (15): string, vector, Preset, name, uid, values, Entry, path (+7 more)

### Community 235 - "renderClickFrames"
Cohesion: 0.15
Nodes (12): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, FramePosition (+4 more)

### Community 236 - "Version"
Cohesion: 0.25
Nodes (7): updateUserAgent(), Version, major, minor, patch, phase, string

### Community 237 - "TimestampedMidiMessage"
Cohesion: 0.22
Nodes (9): midiMessageReceived, sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos (+1 more)

### Community 238 - "LoadSampleCommand"
Cohesion: 0.18
Nodes (9): vector, LoadSampleCommand, channelId_, import_, minted_, path_, previousInstrument_, previousStateFile_ (+1 more)

### Community 239 - "NoteCommands.cpp"
Cohesion: 0.29
Nodes (12): NoteIndices, size_t, vector, execute, findEvents(), execute, undo, execute (+4 more)

### Community 240 - "Fixture"
Cohesion: 0.25
Nodes (6): Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 241 - "InsertAudioCommand"
Cohesion: 0.25
Nodes (7): FramePosition, InsertAudioCommand, asset_, at_, insertedAt_, minted_, piece_

### Community 242 - "RenderTests.cpp"
Cohesion: 0.25
Nodes (5): path, string, makeArrangedProject(), ScratchDirectory, path

### Community 243 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 244 - "project::compileProjectGraph"
Cohesion: 0.33
Nodes (7): project::compileProjectGraph, InstrumentFactory (injected), Pattern Mode and Song Mode, D-014 — Swing displaces only notes exactly on the grid, D-017 — Pattern mode and song mode are a compile-time distinction, D-018 — Track mute and solo are resolved when the arrangement compiles, D-034 — Instrument parameter values live in the model, applied at compile

### Community 245 - "BlobReader"
Cohesion: 0.16
Nodes (13): BlobReader, cursor, data, BlobWriter, out, overflowed, loadState, saveState (+5 more)

### Community 246 - "EditAssetRegionCommand"
Cohesion: 0.18
Nodes (10): AudioEditOp, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_, minted_ (+2 more)

### Community 247 - "INCDAWPianoRollView"
Cohesion: 0.29
Nodes (6): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -pruneSelectionAfterHistoryChange, -requestRedraw

### Community 248 - "ScratchDir"
Cohesion: 0.33
Nodes (5): FrameCount, path, ScratchDir, path, writeWav()

### Community 249 - "processThrough"
Cohesion: 0.29
Nodes (11): convolverWith(), path, Sample, size_t, unique_ptr, vector, noiseSignal(), partitionGain() (+3 more)

### Community 250 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 251 - "StoppedTransportTests.cpp"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 252 - "StressTests.cpp"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 253 - "engine/audio/AudioRecorder"
Cohesion: 0.33
Nodes (6): engine/audio/AudioLogger (last 60 s of master), engine/audio/AudioRecorder, project/RecordingSession, engine::TimelineAnchor (D-024), Capture is a second clock domain (D-023), Unfinalized take probes as zero frames

### Community 254 - "AppSettingsTests.cpp"
Cohesion: 0.50
Nodes (4): path, string, scratchFile(), writeText()

### Community 255 - "ClipCommands.cpp"
Cohesion: 0.04
Nodes (57): ClipIds, MovedAudioClip, Snapshot, string, DuplicateClipsCommand, clips_, created_, createdIds_ (+49 more)

### Community 256 - "AddNoteCommand"
Cohesion: 0.20
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, undo, size_t

### Community 257 - "NoteSequence"
Cohesion: 0.18
Nodes (9): Tick, Tick, uint32_t, vector, NoteSequence, byEnd_, length_, loopLength_ (+1 more)

### Community 258 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 259 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 260 - "v1.2/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 261 - "v1.3/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 262 - "v1.4/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 263 - "v1.5/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 264 - "v1.6/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 265 - "ThemePaletteTests.cpp"
Cohesion: 0.50
Nodes (4): path, string, scratchFile(), writeText()

### Community 266 - "QuantizeNotesCommand"
Cohesion: 0.20
Nodes (9): Tick, QuantizeNotesCommand, channel_, execute, grid_, pattern_, previousEvents_, strength_ (+1 more)

### Community 267 - "setParameter"
Cohesion: 0.22
Nodes (3): FilterMode, uint32_t, setParameter

### Community 268 - "main"
Cohesion: 0.32
Nodes (7): activeVoiceCount, activeVoiceCount, time_point, vector, main(), median(), millisecondsSince()

### Community 269 - "prepare"
Cohesion: 0.67
Nodes (4): prepare, FrameCount, SampleRate, prepare

### Community 271 - "ScratchDirectory"
Cohesion: 0.33
Nodes (4): path, string, ScratchDirectory, path

### Community 272 - "MoveNotesCommand"
Cohesion: 0.18
Nodes (10): MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, canMergeWith, channel_, indices_, keyDelta_, mergeWith (+2 more)

### Community 273 - "collectForBlock"
Cohesion: 0.29
Nodes (6): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock, resetCounters

### Community 274 - "StrumNotesCommand"
Cohesion: 0.20
Nodes (9): Tick, StrumNotesCommand, channel_, downward_, indices_, pattern_, previousDurations_, previousTicks_ (+1 more)

### Community 275 - "nanosForFrame"
Cohesion: 0.40
Nodes (5): FrameCount, SampleRate, uint64_t, nanosForFrame(), timestamped()

### Community 276 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 279 - "ProjectSession.cpp"
Cohesion: 0.33
Nodes (8): autosaveIsNewer(), autosavePathFor(), path, size_t, string, vector, exportFileName(), updatedRecents()

### Community 281 - "SessionFixture"
Cohesion: 0.40
Nodes (4): path, string, SessionFixture, root

### Community 282 - "Performance Targets (audio, UI, project)"
Cohesion: 0.67
Nodes (3): Audio Callback Performance Budget (2.67 ms at 48 kHz / 128 frames), Reference Machine (Apple M5, 10 cores, 16 GB, macOS 26.2), Performance Targets (audio, UI, project)

### Community 283 - "create"
Cohesion: 0.25
Nodes (8): clap_event_param_value_t, create, array, string, unique_ptr, PendingParamEvents, count, events

### Community 285 - "StateIO"
Cohesion: 0.40
Nodes (3): StateIO, loadState, saveState

### Community 299 - "PluginStateTests.cpp"
Cohesion: 0.12
Nodes (17): anyNonZero(), compileLoaded(), InsertFactory, path, Sample, uint8_t, vector, factoryFor() (+9 more)

### Community 300 - "ClapInstance"
Cohesion: 0.38
Nodes (7): ClapInstance, GraphCompileOptions::insertFactory, Plugin Latency Reporting and PDC Feed, PluginInstance (format-agnostic interface), PluginInstanceManager, PluginNode, plugins::PluginParameterInfo

### Community 301 - "MintedAsset"
Cohesion: 0.29
Nodes (7): size_t, MintedAsset, copy, created, id, index, ok

### Community 302 - "MidiImportResult"
Cohesion: 0.25
Nodes (7): string, vector, MidiImportResult, error, newChannels, pattern, succeeded

### Community 303 - "ParameterTable"
Cohesion: 0.40
Nodes (5): size_t, parametersFor(), ParameterTable, count, items

### Community 304 - "AutomationFixture"
Cohesion: 0.20
Nodes (9): AutomationFixture, channel, pattern, project, tempo, AutomationPoint, Tick, enginePoint() (+1 more)

### Community 305 - "collectForRange"
Cohesion: 0.22
Nodes (8): FrameCount, FramePosition, MidiBuffer, vector, clear, collectForRange, rebuildIndices, setNotes

### Community 306 - "ArpeggiateNotesCommand"
Cohesion: 0.22
Nodes (8): Direction, ArpeggiateNotesCommand, channel_, direction_, indices_, pattern_, previousEvents_, step_

### Community 307 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (9): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo (+1 more)

### Community 308 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 309 - "INCDAWTonePanel"
Cohesion: 0.40
Nodes (4): NSObject, INCDAWTonePanel, +makePanelWithTitlerowssampleRateonWrite, +refreshWindowvalues

### Community 310 - "SequencedNote"
Cohesion: 0.23
Nodes (11): SequencedNote, channel, key, lengthTicks, startTick, velocity, Tick, vector (+3 more)

### Community 311 - "UpdateCheckTests.cpp"
Cohesion: 0.67
Nodes (3): string, entry(), feed()

### Community 312 - "DiskStreamer"
Cohesion: 0.22
Nodes (9): DiskStreamer, mutex_, running_, serviceOnce, streams_, atomic, mutex, vector (+1 more)

### Community 313 - "SampleCache"
Cohesion: 0.24
Nodes (9): size_t, Entry, mutex, string, SampleCache, clear, entries_, entryCount (+1 more)

### Community 315 - "emptyOutTryPush"
Cohesion: 0.33
Nodes (6): clap_event_header_t, clap_input_events_t, clap_output_events_t, emptyOutTryPush(), pendingInGet(), pendingInSize()

### Community 316 - "build"
Cohesion: 0.27
Nodes (9): bucketize(), Bucket, FrameCount, path, Result, Sample, vector, sizeBuckets() (+1 more)

### Community 317 - "RenderResult"
Cohesion: 0.20
Nodes (9): FrameCount, string, vector, RenderResult, arrangementFrames, audio, error, succeeded (+1 more)

### Community 318 - "processThrough"
Cohesion: 0.33
Nodes (9): ParameterSink, ParametricBandType, Sample, size_t, vector, processThrough(), rmsOf(), setBand() (+1 more)

### Community 319 - "BuiltinInstrumentInfo"
Cohesion: 0.20
Nodes (9): BuiltinInstrumentInfo, displayName, parameterCount, parameters, presets, uid, string, findBuiltinInstrument() (+1 more)

### Community 320 - ".incdaw versioned project format"
Cohesion: 0.22
Nodes (9): engine/audio/AudioEdits (editor verbs), The hum at idle (block-rate retrigger defect), .incdaw versioned project format, app/ProjectSession (lifecycle decisions), Core data model (no collapsed entities), Definition of success (three signal chains), Format 1.5/1.6 renumbering on merge, Project format requirements (+1 more)

### Community 321 - "ResizeNotesCommand"
Cohesion: 0.22
Nodes (8): ResizeNotesCommand, canMergeWith, channel_, durationDelta_, indices_, mergeWith, pattern_, previousDurations_

### Community 322 - "uint32_t"
Cohesion: 0.22
Nodes (9): size, blobWrite(), hasEditor, openEditor, process, readParameter, setParameter, clap_ostream_t (+1 more)

### Community 323 - "processThrough"
Cohesion: 0.33
Nodes (8): ParameterSink, Sample, size_t, vector, magnitudes(), processThrough(), setHardClip(), sine()

### Community 324 - "AutomationProbe"
Cohesion: 0.29
Nodes (6): AutomationProbe, calls, registry, written, FramePosition, vector

### Community 325 - "ScriptedFactory"
Cohesion: 0.25
Nodes (7): function, InsertFactory, unique_ptr, ScriptedFactory, fail, makers, requests

### Community 326 - "INCDAWControlBarView"
Cohesion: 0.38
Nodes (6): NSInteger, NSTextFieldDelegate, NSString, NSView, INCDAWControlBarView, INCDAWStatusBarView

### Community 327 - "load"
Cohesion: 0.33
Nodes (7): int64_t, path, shared_ptr, string, uintmax_t, load, statFile()

### Community 328 - "BuiltinEffectInfo"
Cohesion: 0.29
Nodes (7): BuiltinEffectInfo, displayName, parameterCount, parameters, presets, uid, size_t

### Community 329 - "InsertFixture"
Cohesion: 0.33
Nodes (4): InsertFixture, pattern, project, tempo

### Community 330 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 331 - "prepare"
Cohesion: 0.40
Nodes (4): prepare, FrameCount, SampleRate, size_t

### Community 332 - "transformImpulse"
Cohesion: 0.40
Nodes (5): transformImpulse, array, maxChannels, size_t, Spectra

### Community 333 - "ScratchDir"
Cohesion: 0.50
Nodes (3): path, ScratchDir, path

### Community 334 - "loopWithHits"
Cohesion: 0.50
Nodes (3): size_t, vector, loopWithHits()

## Ambiguous Edges - Review These
- `Clip / Project Data Model` → `Undo / Redo`  [AMBIGUOUS]
  CLAUDE.md · relation: shares_data_with

## Knowledge Gaps
- **1878 isolated node(s):** `currentVersion`, `audio`, `openInputAtLaunch`, `midiInputIdentifiers`, `workspace` (+1873 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **10 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `Clip / Project Data Model` and `Undo / Redo`?**
  _Edge tagged AMBIGUOUS (relation: shares_data_with) - confidence is low._
- **Why does `EntityId` connect `EntityId` to `MixerCommands.cpp`, `AddNoteCommand`, `SetVelocityCommand`, `WriteAutomationCommand`, `ApplyInsertPresetCommand`, `PatternCommands.cpp`, `PianoRollModel`, `QuantizeNotesCommand`, `SamplerWiringTests.cpp`, `Channel`, `Project`, `MoveNotesCommand`, `AddInsertCommand`, `StrumNotesCommand`, `AddMarkerCommand`, `EditFixture`, `SetNoteLabelCommand`, `MidiEvent`, `StretchAssetCommand`, `NudgeChordCommand`, `InstrumentNode`, `SidechainFixture`, `ChannelCommands.cpp`, `MixerTests.cpp`, `SplitClipCommand`, `RemoveTrackCommand`, `MintedAsset`, `MidiImportResult`, `RemoveMixerNodeCommand`, `CompiledProjectGraph`, `AutomationFixture`, `ArpeggiateNotesCommand`, `DeleteNotesCommand`, `AddSamplerZoneCommand`, `ImportAudioClipCommand`, `Pattern`, `Fixture`, `capturePluginState`, `ResizeNotesCommand`, `GraphCompileOptions`, `Clip`, `read`, `AddMidiMappingCommand`, `InsertFixture`, `AddPatternClipCommand`, `ChannelSamplerZone`, `Fixture`, `SetSamplerZoneCommand`, `CountingCommand`, `ParameterFixture`, `AutomationWriteSession`, `InsertRecordedTakeCommand`, `RenderOptions`, `SendFixture`, `RemoveSamplerZoneCommand`, `PluginStateTests.cpp`, `ConnectMixerCommand`, `PlaylistModel`, `Fixture`, `LoadSampleCommand`, `NoteCommands.cpp`, `load`, `InsertAudioCommand`, `Fixture`, `SliceAssetCommand`, `EditAssetRegionCommand`, `AudioAssetImport`, `SplitFixture`, `allocate`, `ClipCommands.cpp`?**
  _High betweenness centrality (0.105) - this node is a cross-community bridge._
- **Why does `Project` connect `Project` to `MixerCommands.cpp`, `AddNoteCommand`, `WriteAutomationCommand`, `ApplyInsertPresetCommand`, `PatternCommands.cpp`, `QuantizeNotesCommand`, `SamplerWiringTests.cpp`, `Channel`, `AddInsertCommand`, `AddMarkerCommand`, `EditFixture`, `MidiEvent`, `CommandRegistry`, `NudgeChordCommand`, `SidechainFixture`, `ChannelCommands.cpp`, `MixerTests.cpp`, `TempoMap`, `SplitClipCommand`, `RemoveTrackCommand`, `CompiledProjectGraph`, `PluginStateTests.cpp`, `RemoveMixerNodeCommand`, `AutomationFixture`, `DeleteNotesCommand`, `SetTempoCommand`, `Pattern`, `renderArrangement`, `Fixture`, `capturePluginState`, `Clip`, `AutomationProbe`, `read`, `AddMidiMappingCommand`, `InsertFixture`, `AddPatternClipCommand`, `Fixture`, `SetSamplerZoneCommand`, `CountingCommand`, `ParameterFixture`, `InsertRecordedTakeCommand`, `SendFixture`, `RemoveSamplerZoneCommand`, `PluginIdentifier`, `exportArrangement`, `ConnectMixerCommand`, `PlaylistModel`, `Fixture`, `NoteCommands.cpp`, `load`, `Fixture`, `RenderTests.cpp`, `AudioAssetImport`, `SplitFixture`, `EntityId`, `allocate`, `ClipCommands.cpp`?**
  _High betweenness centrality (0.097) - this node is a cross-community bridge._
- **Why does `BuiltinEffect` connect `BuiltinEffect` to `LoudnessMeterEffect`, `ParametricEqEffect`, `Command`, `ProcessContext`, `MultibandCompressorEffect`, `BiquadCoefficients`, `LookaheadLimiterEffect`, `atomic`, `FlangerEffect`, `ConvolutionReverbEffect`, `PluginParameterInfo`, `EqEffect`, `VocoderEffect`, `AnalyzerEffect`, `Node`, `Wavetable`, `BeatGateEffect`, `WaveshaperEffect`, `StereoImagerEffect`?**
  _High betweenness centrality (0.092) - this node is a cross-community bridge._
- **What connects `currentVersion`, `audio`, `openInputAtLaunch` to the rest of the system?**
  _1878 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `MixerCommands.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.028282828282828285 - nodes in this community are weakly interconnected._
- **Should `Browser` be split into smaller, more focused modules?**
  _Cohesion score 0.07179487179487179 - nodes in this community are weakly interconnected._