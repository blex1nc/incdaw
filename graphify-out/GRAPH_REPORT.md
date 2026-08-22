# Graph Report - timbre  (2026-08-23)

## Corpus Check
- 395 files · ~355,479 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 6256 nodes · 11227 edges · 323 communities (313 shown, 10 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 625 edges (avg confidence: 0.81)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `4794f74e`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- RemoveMixerNodeCommand
- Browser
- NoteCommands.cpp
- WavStreamWriter
- WriteAutomationCommand
- PresetLibrary.cpp
- LoudnessMeterEffect
- PatternCommands.cpp
- TestGainPlugin.cpp
- PianoRollModel
- AudioEngine
- Command
- PluginInstanceManager
- ProcessContext
- ConnectMixerCommand
- Transport
- AddInsertCommand
- CoreAudioDevice
- Sampler
- AddMarkerCommand
- EntityId
- ArpeggiateNotesCommand
- Release
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
- DelayLineNode
- INCDAW
- ChannelRackModel
- WavStreamReader
- TempoMap
- BuiltinEffect
- AudioUnitInstance
- CompiledProjectGraph
- MusicTheory.cpp
- EqEffect
- INCDAWMixerView
- BuiltinEffect.cpp
- ClapInstance
- MetronomeNode
- AudioLogger
- read
- LoadSampleCommand
- SetTempoCommand
- DynamicsEffects.cpp
- DelayEffect
- PluginRegistry.cpp
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
- Json
- AutomationWriteSession
- InsertRecordedTakeCommand
- ParameterRegistry
- RenderOptions
- project::compileProjectGraph
- CompiledGraph
- AudioBufferPool
- AudioFileData
- renderBlock
- PluginIdentifier
- ThemePalette.cpp
- RealtimeGuard.cpp
- INCDAW macOS app bundle target
- Audio Correctness Requirements
- FL Studio 2026 Functional Reference
- SamplerZoneStream
- INCDAWPluginPickerView
- PlaylistModel
- PianoRollHeaderModel
- AudioBufferView
- ClapLibrary
- SamplerZone
- array
- FactoryPresetTable
- load
- FuzzTests.cpp
- incdaw_tests suite target
- write
- CoreMidiDevice
- MidiEvent
- SetInstrumentParameterCommand
- SampleRingBuffer
- MixerStripNode
- MixerStripNode.cpp
- MidiMapNode
- SystemInfo
- AddMixerNodeCommand
- PianoInstrument
- Project
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
- DuplicateClipsCommand
- AudioClipNode
- SampleCache
- LevelMeter
- LockFreeQueue
- compileProjectGraph
- RemoveChannelCommand
- GainNode
- Node
- NoteSequence
- Channel
- capturePluginState
- EditFixture
- LoopbackResult
- BuiltinEffectTests.cpp
- Pattern
- BuiltinEffectInfo
- PluginPickerModel
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
- string
- ClapLibrary.cpp
- SamplerWiringTests.cpp
- Realtime Safety Guard (debug builds)
- RenderGraph (compiled immutable graph)
- INCDAW Testing Strategy
- ParsedHeader
- AddPatternClipCommand
- RemoveTrackCommand
- PluginParameterInfo
- Smoother
- AudioUnitParameterDescription
- INCDAWInsertParameterPanel
- renderNode
- EditAssetRegionCommand
- Phase 18 Measured Baseline (2026-08-16)
- D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded)
- Phase 1 — Foundation and Build System
- ClapInstance
- ChannelCommands.cpp
- KernelTable
- MidiDevice
- SetChannelInstrumentCommand
- incdaw_engine layer library
- AutomationTests.cpp
- Fixture
- renderArrangement
- Fixture
- SetChannelVolumeCommand
- ChildResult
- SettingsWindow.mm
- MixerCommands.cpp
- SetMixerStereoSeparationCommand
- TimelineAnchor
- SetMixerVolumeCommand
- timeStretch
- SharedLibrary
- humanizeNoteStarts
- SetSendGainCommand
- PatternListView.mm
- Instrument
- TonePanel.mm
- AddSamplerZoneCommand
- collectForRange
- RecordedEvent
- processThrough
- DiskStreamer
- PluginFolder
- SamplerStreamingTests.cpp
- make-dmg.sh
- Atomic Graph Pointer Swap
- D-028 — Hosted plugins reach the graph through an injected factory
- AddChannelCommand
- PianoModelSpec
- MidiDeviceInfo
- ToggleStepCommand
- PluginPickerModel.cpp
- SplitClipCommand
- DisconnectMixerCommand
- ScratchDirectory
- BuiltinInstrumentInfo
- exportArrangement
- string
- HttpResponse
- BlobWriter
- Denormals.h
- AudioDevice (platform device interface)
- INCDAWSettingsWindow
- UI Build-Out (post-phase increments)
- ClipCommands.cpp
- TempoEvent
- PluginPickerEntry
- Version
- TimestampedMidiMessage
- InsertAudioCommand
- INCDAWSpectrumView
- Fixture
- AutomationProbe
- RenderTests.cpp
- makeTestSignal
- project::compileProjectGraph
- BlobReader
- RemoveClipsCommand
- INCDAWPianoRollView
- ScratchDir
- StateFixture
- ScratchDirectory
- StoppedTransportTests.cpp
- StressTests.cpp
- engine/audio/AudioRecorder
- RenderResult
- ResizeClipsCommand
- build
- StretchClipsCommand
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- v1.2/Fixture.incdaw/manifest.json
- v1.3/Fixture.incdaw/manifest.json
- v1.4/Fixture.incdaw/manifest.json
- v1.5/Fixture.incdaw/manifest.json
- v1.6/Fixture.incdaw/manifest.json
- SetChannelStepKeyCommand
- Fixture
- setParameter
- main
- SessionFixture
- BuiltinInsertTests.cpp
- ScratchDirectory
- ClipIds
- collectForBlock
- AppSettingsTests.cpp
- MidiTests.cpp
- check
- AudioCaptureSink
- ParameterSink
- ProjectSession.cpp
- SetClipMutedCommand
- Performance Targets (audio, UI, project)
- create
- D-020 — A mixer strip is one node
- INCDAWControlBarView
- CommandRegistry
- Out-of-Process-Ready Plugin Host ABI
- D-009 — Distribution: ad-hoc signed, un-notarized DMG
- D-010 — Version control: git, initialised at Phase 0
- SetChannelOutputCommand
- RemoveSamplerZoneCommand
- SetChannelMutedCommand
- MidiImportResult
- SetChannelSoloedCommand
- SetMixerMutedCommand
- SequencedNote
- SetMixerPolarityCommand
- SetMixerSoloedCommand
- INCDAWAudioEditorView
- INCDAWTonePanel
- ThemePaletteTests.cpp
- EffectParameter
- MintedAsset
- prepare
- Legal / IP Boundary
- emptyOutTryPush
- DitherSource
- StateIO
- uint32_t
- -mouseDragged
- loopWithHits
- UpdateCheckTests.cpp
- StandardActions.cpp

## God Nodes (most connected - your core abstractions)
1. `Project` - 296 edges
2. `EntityId` - 263 edges
3. `Command` - 161 edges
4. `TempoMap` - 83 edges
5. `AudioEngine` - 70 edges
6. `AudioFileData` - 65 edges
7. `Sampler` - 61 edges
8. `PianoInstrument` - 60 edges
9. `AudioBufferPool` - 59 edges
10. `CoreAudioDevice` - 59 edges

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

## Communities (323 total, 10 thin omitted)

### Community 0 - "RemoveMixerNodeCommand"
Cohesion: 0.15
Nodes (10): RemovedRouting, vector, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_ (+2 more)

### Community 1 - "Browser"
Cohesion: 0.07
Nodes (63): directory_entry, Browser, addDefaultRoots, addRoot, canDecodeAudio, classify, clear, defaultSearchLimit (+55 more)

### Community 2 - "NoteCommands.cpp"
Cohesion: 0.04
Nodes (59): undo, NoteIndices, size_t, string, vector, DeleteNotesCommand, channel_, execute (+51 more)

### Community 3 - "WavStreamWriter"
Cohesion: 0.05
Nodes (59): ofstream, appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample (+51 more)

### Community 4 - "WriteAutomationCommand"
Cohesion: 0.04
Nodes (52): AddAutomationLaneCommand, execute, index_, key_, lane_, minted_, target_, undo (+44 more)

### Community 5 - "PresetLibrary.cpp"
Cohesion: 0.06
Nodes (72): ApplyInsertPresetCommand, after_, before_, execute, presetName_, undo, write, writer_ (+64 more)

### Community 6 - "LoudnessMeterEffect"
Cohesion: 0.04
Nodes (52): Biquad, AnalyzerEffect, accumulate_, accumulated_, binCount, fft_, fftSize, generation_ (+44 more)

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
Cohesion: 0.06
Nodes (52): RetiredGraph, AudioCaptureSink, AudioEngine, active_, anchor_, anchorVersion_, audioDeviceAboutToStart, audioDeviceStopped (+44 more)

### Community 11 - "Command"
Cohesion: 0.07
Nodes (7): string, vector, Command, execute, id, name, undo

### Community 12 - "PluginInstanceManager"
Cohesion: 0.06
Nodes (40): Held, Library, size_t, string, uint32_t, uint64_t, unique_ptr, vector (+32 more)

### Community 13 - "ProcessContext"
Cohesion: 0.10
Nodes (36): dbToGain(), sumInputsInto(), coefficientFor(), process, size_t, process, process, linkedPeakAt() (+28 more)

### Community 14 - "ConnectMixerCommand"
Cohesion: 0.12
Nodes (12): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+4 more)

### Community 15 - "Transport"
Cohesion: 0.06
Nodes (34): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FramePosition, size_t (+26 more)

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
Cohesion: 0.07
Nodes (25): AddMarkerCommand, execute, index_, length_, marker_, minted_, tick_, undo (+17 more)

### Community 20 - "EntityId"
Cohesion: 0.09
Nodes (34): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, colourForIndex(), size_t (+26 more)

### Community 21 - "ArpeggiateNotesCommand"
Cohesion: 0.06
Nodes (46): Direction, map, ArpeggiateNotesCommand, channel_, direction_, execute, indices_, pattern_ (+38 more)

### Community 22 - "Release"
Cohesion: 0.08
Nodes (32): automaticCheckIsDue(), int64_t, optional, size_t, string, vector, evaluateFeed(), string (+24 more)

### Community 23 - "AuditionPlayer"
Cohesion: 0.05
Nodes (35): Retired, AuditionPlayer, collect, current_, gain_, generation_, play, playing_ (+27 more)

### Community 24 - "AudioStream"
Cohesion: 0.10
Nodes (24): AudioStream, fillSegment, lastRequested_, open, prefill, read, reader_, segmentFrames_ (+16 more)

### Community 25 - "LookaheadLimiterEffect"
Cohesion: 0.06
Nodes (30): CompressorEffect, envelope_, noKeyInput, reduction_, sampleRate_, GateEffect, gain_, holdLeft_ (+22 more)

### Community 26 - "PluginInsertTests.cpp"
Cohesion: 0.08
Nodes (27): anyNonZero(), ClipInsert, threshold_, FrameCount, function, InsertFactory, path, Sample (+19 more)

### Community 27 - "GraphBuilder"
Cohesion: 0.08
Nodes (32): Connection, process, FrameCount, FramePosition, MidiBuffer, NodeIndex, SampleRate, size_t (+24 more)

### Community 28 - "StretchAssetCommand"
Cohesion: 0.06
Nodes (27): DeleteAudioRegionCommand, applied_, asset_, minted_, region_, removed_, FrameCount, Sample (+19 more)

### Community 29 - "PresetBar.mm"
Cohesion: 0.10
Nodes (22): NSMenuItem, NSString, NSView, INCDAWPresetBar, +attachToWindow, +barInWindow, -refreshAppearance, +refreshAppearanceInWindow (+14 more)

### Community 30 - "AudioRecorder"
Cohesion: 0.07
Nodes (28): AudioCaptureSink, AudioRecorder, captureAudioBlock, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_ (+20 more)

### Community 31 - "CommandRegistry"
Cohesion: 0.10
Nodes (28): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+20 more)

### Community 32 - "NudgeChordCommand"
Cohesion: 0.09
Nodes (26): vector, findEvents(), Scale, size_t, string, vector, InsertNotesCommand, channel_ (+18 more)

### Community 33 - "atomic"
Cohesion: 0.10
Nodes (7): thread_, unordered_map, atomic, MidiBuffer, mutex, allocationSize(), size_t

### Community 34 - "InstrumentNode"
Cohesion: 0.10
Nodes (16): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, ParameterSink, unique_ptr, InstrumentNode (+8 more)

### Community 35 - "TestLatencyPlugin.cpp"
Cohesion: 0.10
Nodes (31): clap_host_t, clap_istream_t, clap_ostream_t, clap_plugin_descriptor_t, clap_plugin_factory_t, clap_plugin_t, clap_process_status, clap_process_t (+23 more)

### Community 36 - "DelayLineNode"
Cohesion: 0.05
Nodes (35): FrameCount, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_, prepare (+27 more)

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
Cohesion: 0.08
Nodes (34): execute, execute, execute, FrameCount, SampleRate, prepare, process, triggerClick (+26 more)

### Community 41 - "BuiltinEffect"
Cohesion: 0.06
Nodes (35): BuiltinEffect, values_, atomic, vector, ChorusEffect, centreMs, line_, mask_ (+27 more)

### Community 42 - "AudioUnitInstance"
Cohesion: 0.09
Nodes (30): AudioUnitHandle, closeEditor, hasEditor, latencyFrames, open, openEditor, process, restoreState (+22 more)

### Community 43 - "CompiledProjectGraph"
Cohesion: 0.07
Nodes (28): CompiledProjectGraph, automation, builtInserts, builtSlots, channels, channelStrips, error, graph (+20 more)

### Community 44 - "MusicTheory.cpp"
Cohesion: 0.24
Nodes (19): pitchClasses, Scale, size_t, string, vector, degreeOf(), detectChord(), diatonicChordPitchClasses() (+11 more)

### Community 45 - "EqEffect"
Cohesion: 0.07
Nodes (28): BiquadCoefficients, a1, a2, b0, b1, b2, FrameCount, SampleRate (+20 more)

### Community 46 - "INCDAWMixerView"
Cohesion: 0.09
Nodes (53): incdaw, NSDraggingDestination, NSArray, NSDictionary, NSView, INCDAWMixerView, -acceptsFirstResponder, -addStripRect (+45 more)

### Community 47 - "BuiltinEffect.cpp"
Cohesion: 0.32
Nodes (15): appendF64(), appendU32(), BuiltinEffect::BuiltinEffect(), decodeState, loadState, saveState, setParameter, value (+7 more)

### Community 48 - "ClapInstance"
Cohesion: 0.07
Nodes (26): clap_plugin_gui_t, clap_plugin_latency_t, clap_plugin_params_t, clap_plugin_state_t, ParamEvent, ClapInstance, editorOpen_, gui_ (+18 more)

### Community 49 - "MetronomeNode"
Cohesion: 0.10
Nodes (17): atomic, FrameCount, Sample, SampleRate, size_t, vector, MetronomeNode, amplitude_ (+9 more)

### Community 50 - "AudioLogger"
Cohesion: 0.09
Nodes (22): AudioLogger, capacityFrames_, circle_, enabled_, grab, log, prepare, ready_ (+14 more)

### Community 51 - "read"
Cohesion: 0.08
Nodes (39): appendBigU16(), appendBigU32(), appendChunk(), appendVlq(), path, Result, size_t, Tick (+31 more)

### Community 52 - "LoadSampleCommand"
Cohesion: 0.12
Nodes (10): string, vector, LoadSampleCommand, channelId_, import_, minted_, path_, previousInstrument_ (+2 more)

### Community 53 - "SetTempoCommand"
Cohesion: 0.09
Nodes (20): string, vector, SetTempoCommand, canMergeWith, captured_, clampTempo, execute, maximumTempo (+12 more)

### Community 54 - "DynamicsEffects.cpp"
Cohesion: 0.16
Nodes (13): prepare, FrameCount, SampleRate, prepare, prepare, linkedPeakOf(), LookaheadLimiterEffect::LookaheadLimiterEffect(), prepare (+5 more)

### Community 55 - "DelayEffect"
Cohesion: 0.08
Nodes (24): Allpass, Comb, FrameCount, SampleRate, DelayEffect, capacity_, lines_, maxChannels (+16 more)

### Community 56 - "PluginRegistry.cpp"
Cohesion: 0.08
Nodes (34): Located, ClapDescriptor, id, name, vendor, version, string, int64_t (+26 more)

### Community 57 - "ImportAudioClipCommand"
Cohesion: 0.06
Nodes (33): AudioAssetImport, asset, created, id, index, string, size_t, importAudioAsset() (+25 more)

### Community 58 - "The work, in order"
Cohesion: 0.11
Nodes (17): A10 — parametric EQ, more bands and a draggable curve, A11 — convolution reverb, A12/A13 — vocoder, and time/volume gating, A1 — wavetable synth, A2 — FM synth, A4 — drum machine / pad instrument, A5 — the preset system (do this first), A6 — multiband compressor (+9 more)

### Community 59 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 60 - "Json.cpp"
Cohesion: 0.19
Nodes (21): size_t, string, escapeInto(), formatDouble(), asBool, asString, contains, dump (+13 more)

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
Nodes (36): ClipType, AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount, id (+28 more)

### Community 68 - "INCDAWAppDelegate"
Cohesion: 0.08
Nodes (21): NSApplicationDelegate, NSMenuDelegate, NSOutlineViewDataSource, NSOutlineViewDelegate, NSView, INCDAWBrowserView, -initWithFramebrowser, -reload (+13 more)

### Community 69 - "AppSettings"
Cohesion: 0.11
Nodes (25): AppSettings, appearance, audio, currentVersion, fromJson, load, midiInputIdentifiers, openInputAtLaunch (+17 more)

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
Cohesion: 0.06
Nodes (18): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t (+10 more)

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
Cohesion: 0.16
Nodes (22): FrameCount, Sample, SampleRate, vector, Voice, interpolate(), activeVoiceCount, allNotesOff (+14 more)

### Community 79 - "SimpleSynth"
Cohesion: 0.06
Nodes (38): FrameCount, SampleRate, size_t, uint32_t, Voice, Waveform, frequencyForKey(), array (+30 more)

### Community 80 - "SetSamplerZoneCommand"
Cohesion: 0.16
Nodes (14): execute, string, ensureAssetForFile(), undo, execute, SetSamplerZoneCommand, canMergeWith, channelId_ (+6 more)

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

### Community 86 - "Json"
Cohesion: 0.11
Nodes (15): nullptr_t, int64_t, int64_t, pair, string, vector, Json, asDouble (+7 more)

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
Cohesion: 0.10
Nodes (25): engine/audio/AudioClipNode, engine/audio/AudioEdits (editor verbs), engine/audio/AudioStream + DiskStreamer (D-025), Export Audio options dialog, The hum at idle (block-rate retrigger defect), engine::InstrumentNode, EBU R128 loudness (incdaw.loudness), engine/dsp/MixerStripNode (+17 more)

### Community 92 - "CompiledGraph"
Cohesion: 0.13
Nodes (13): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+5 more)

### Community 93 - "AudioBufferPool"
Cohesion: 0.11
Nodes (17): AudioBufferPool, channelPointers_, reset, samples_, FrameCount, Sample, size_t, unique_ptr (+9 more)

### Community 94 - "AudioFileData"
Cohesion: 0.12
Nodes (34): applyGain(), applyRamp(), clampedRegion(), FramePosition, Sample, deleteRegion(), extractRegion(), fadeIn() (+26 more)

### Community 95 - "renderBlock"
Cohesion: 0.47
Nodes (5): FrameCount, Sample, vector, renderBlock(), tone()

### Community 96 - "PluginIdentifier"
Cohesion: 0.10
Nodes (18): builtinPiano(), builtinSampler(), builtinSimpleSynth(), Format, string, formatName(), Format, friend (+10 more)

### Community 97 - "ThemePalette.cpp"
Cohesion: 0.06
Nodes (65): Entry, path, string, string_view, vector, path, ThemeLibrary, contains (+57 more)

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

### Community 103 - "INCDAWPluginPickerView"
Cohesion: 0.12
Nodes (24): NSDraggingSource, NSSearchFieldDelegate, NSArray, NSDictionary, NSView, INCDAWPluginPickerView, -acceptsFirstResponder, -chooseRow (+16 more)

### Community 104 - "PlaylistModel"
Cohesion: 0.10
Nodes (30): Rect, size_t, Tick, vector, size_t, Tick, vector, Viewport (+22 more)

### Community 105 - "PianoRollHeaderModel"
Cohesion: 0.07
Nodes (31): Snap, Layout, Rect, Scale, size_t, Tick, PianoRollHeaderModel, layout_ (+23 more)

### Community 106 - "AudioBufferView"
Cohesion: 0.20
Nodes (7): AudioBufferView, channels_, frames_, offset_, FrameCount, Sample, size_t

### Community 107 - "ClapLibrary"
Cohesion: 0.08
Nodes (15): clap_plugin_entry_t, ClapLibrary, entry_, factory_, library_, clap_plugin_factory_t, FrameCount, path (+7 more)

### Community 108 - "SamplerZone"
Cohesion: 0.11
Nodes (18): FrameCount, shared_ptr, handleMessage, SamplerZone, end, gain, keyHigh, keyLow (+10 more)

### Community 109 - "array"
Cohesion: 0.13
Nodes (14): ChordDetection, bassKey, display, inverted, matched, rootPitchClass, type, ChordType (+6 more)

### Community 110 - "FactoryPresetTable"
Cohesion: 0.16
Nodes (13): FactoryPreset, name, valueCount, values, FactoryPresetTable, count, items, size_t (+5 more)

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

### Community 116 - "MidiEvent"
Cohesion: 0.05
Nodes (36): AddNoteCommand, channel_, execute, index_, note_, pattern_, size_t, FrameCount (+28 more)

### Community 117 - "SetInstrumentParameterCommand"
Cohesion: 0.17
Nodes (11): uint32_t, SetInstrumentParameterCommand, canMergeWith, channelId_, execute, existedBefore_, mergeWith, parameterId_ (+3 more)

### Community 118 - "SampleRingBuffer"
Cohesion: 0.10
Nodes (19): FrameCount, Sample, SampleRate, size_t, vector, InputMonitorNode, channelCount_, ring_ (+11 more)

### Community 119 - "MixerStripNode"
Cohesion: 0.12
Nodes (11): atomic, Sample, MixerStripNode, left_, meter_, muted_, polarityInverted_, right_ (+3 more)

### Community 120 - "MixerStripNode.cpp"
Cohesion: 0.24
Nodes (12): FrameCount, Sample, SampleRate, panGains, prepare, process, refreshTargets, setGain (+4 more)

### Community 121 - "MidiMapNode"
Cohesion: 0.12
Nodes (10): Binding, size_t, vector, MidiMapNode, bindings_, controlChange(), path, string (+2 more)

### Community 122 - "SystemInfo"
Cohesion: 0.14
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 123 - "AddMixerNodeCommand"
Cohesion: 0.18
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 124 - "PianoInstrument"
Cohesion: 0.06
Nodes (27): PianoModel, uint32_t, array, atomic, maxVoices, ParameterSink, SampleRate, uint64_t (+19 more)

### Community 125 - "Project"
Cohesion: 0.05
Nodes (47): execute, undo, execute, undo, clipLengthTicks(), clipStartTicks(), Tick, Project (+39 more)

### Community 126 - "allocate"
Cohesion: 0.07
Nodes (30): allocate, FrameCount, size_t, Sample, size_t, vector, drive(), ConstantSourceInsert (+22 more)

### Community 127 - "SplitFixture"
Cohesion: 0.12
Nodes (14): CommandRegistry, path, string, Tick, note(), ScratchDirectory, path, SplitFixture (+6 more)

### Community 128 - "ConstantNode"
Cohesion: 0.10
Nodes (14): ConstantNode, latency_, value_, FrameCount, Sample, size_t, vector, OrderRecordingNode (+6 more)

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

### Community 136 - "DuplicateClipsCommand"
Cohesion: 0.22
Nodes (8): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_, undo

### Community 137 - "AudioClipNode"
Cohesion: 0.13
Nodes (13): AudioClipNode, addClip, clips_, fetchScratch_, prepare, process, FrameCount, PlacedClip (+5 more)

### Community 138 - "SampleCache"
Cohesion: 0.15
Nodes (16): int64_t, path, shared_ptr, size_t, string, uintmax_t, Entry, mutex (+8 more)

### Community 139 - "LevelMeter"
Cohesion: 0.15
Nodes (12): atomic, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond, rmsWindowSeconds (+4 more)

### Community 140 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 141 - "compileProjectGraph"
Cohesion: 0.20
Nodes (11): channelStripFor, insertNodeFor, insertSinkFor, insertStateFor, stripFor, compileProjectGraph(), InstrumentFactory, ParameterSink (+3 more)

### Community 142 - "RemoveChannelCommand"
Cohesion: 0.18
Nodes (8): RemovedContent, RemoveChannelCommand, channel_, channelId_, content_, execute, index_, undo

### Community 143 - "GainNode"
Cohesion: 0.15
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 144 - "Node"
Cohesion: 0.08
Nodes (13): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+5 more)

### Community 145 - "NoteSequence"
Cohesion: 0.18
Nodes (9): Tick, Tick, uint32_t, vector, NoteSequence, byEnd_, length_, loopLength_ (+1 more)

### Community 146 - "Channel"
Cohesion: 0.02
Nodes (81): AutomationCurve, AutomationPoint, curve, tension, tick, value, Channel, colour (+73 more)

### Community 147 - "capturePluginState"
Cohesion: 0.24
Nodes (16): captureBuiltinInsertState(), capturePluginState(), CarriedInsertState, blob, slot, path, string, uint8_t (+8 more)

### Community 148 - "EditFixture"
Cohesion: 0.15
Nodes (13): FrameCount, path, Sample, size_t, EditFixture, assetId, file, project (+5 more)

### Community 149 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 150 - "BuiltinEffectTests.cpp"
Cohesion: 0.19
Nodes (15): FrameCount, Sample, size_t, vector, processThrough(), RefAllpass, index, line (+7 more)

### Community 151 - "Pattern"
Cohesion: 0.09
Nodes (37): Emit, size_t, Tick, vector, noteAtStep(), execute, undo, vector (+29 more)

### Community 152 - "BuiltinEffectInfo"
Cohesion: 0.15
Nodes (16): BuiltinEffectInfo, displayName, parameterCount, parameters, presets, uid, CatalogueEntry, info (+8 more)

### Community 153 - "PluginPickerModel"
Cohesion: 0.15
Nodes (13): size_t, vector, PluginPickerModel, clear, entries_, moveHighlight, noRow, rowHeight (+5 more)

### Community 154 - "AutomationPoint"
Cohesion: 0.19
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
Cohesion: 0.16
Nodes (11): size_t, Fft, forward, reversed_, setSize, twiddleCos_, twiddleSin_, size_t (+3 more)

### Community 160 - "ChannelRackView.mm"
Cohesion: 0.29
Nodes (11): -acceptsFirstResponder, -channelCount, -currentPattern, -drawChannelspatternlastStepplayheadStep, -drawRect, -drawRulerplayheadStep, -hitForEvent, -initWithFrameprojectregistry (+3 more)

### Community 161 - "PluginPersistenceTests.cpp"
Cohesion: 0.14
Nodes (11): path, string, uint8_t, vector, gainBlob(), Harness, folder, registry (+3 more)

### Community 162 - "SidechainFixture"
Cohesion: 0.13
Nodes (11): ConstantSourceInsert, level_, CommandRegistry, runKeyed(), SidechainFixture, bassStrip, compressorSlot, kickStrip (+3 more)

### Community 163 - "Plugin Misbehaviour Matrix"
Cohesion: 0.18
Nodes (14): Audio Units (v2 + v3) Support, Plugin Crash Policy, Out-of-Process Isolation Strategy, INCDAW Plugin Host, Shared-Memory Audio Ring Buffer (host boundary), VST2 Exclusion, VST3 Format Support, Audio Logger (60 s master ring buffer) (+6 more)

### Community 164 - "string"
Cohesion: 0.18
Nodes (6): string, RenameChannelCommand, channelId_, execute, previousName_, undo

### Community 165 - "ClapLibrary.cpp"
Cohesion: 0.18
Nodes (14): blobRead(), closeEditor, refreshLatencyIfChanged, close, open, clap_host_t, clap_istream_t, path (+6 more)

### Community 166 - "SamplerWiringTests.cpp"
Cohesion: 0.19
Nodes (10): path, string, noteAtZero(), SamplerProject, asset, channel, project, ScratchDirectory (+2 more)

### Community 167 - "Realtime Safety Guard (debug builds)"
Cohesion: 0.17
Nodes (13): Headless Deterministic Framework-Free Core, Downward-Only Layer Model (ui/app/project/engine/plugins/platform), Automated Layering Test, Denormal Handling (FTZ/DAZ at callback entry), Audio Thread Prime Directive, Realtime Safety Guard (debug builds), D-001 — Core implementation language: C++20, D-025 — Streaming is a window, and starving it is audible, not fatal (+5 more)

### Community 168 - "RenderGraph (compiled immutable graph)"
Cohesion: 0.15
Nodes (13): Offline/Realtime Render Equivalence, RenderGraph (compiled immutable graph), AudioClipNode, AutomationNode, Sample-Accurate Block Splitting at Event Boundaries, InstrumentNode, Offline Rendering Pipeline, ProcessContext::playing (+5 more)

### Community 169 - "INCDAW Testing Strategy"
Cohesion: 0.15
Nodes (13): Scriptable Command Surface Evidence, INCDAW Requirements, Testable Exit Criterion, Phase 15 — Built-in DSP, Phase 16 — MIDI Hardware and Controller Linking, Phase 17 — Rendering and Export, Phase 18 — Performance, Phase 19 — QA (+5 more)

### Community 170 - "ParsedHeader"
Cohesion: 0.17
Nodes (18): path, Result, size_t, uint16_t, uint32_t, uint8_t, vector, fillMetadata() (+10 more)

### Community 171 - "AddPatternClipCommand"
Cohesion: 0.15
Nodes (11): AddPatternClipCommand, clip_, execute, index_, length_, minted_, pattern_, start_ (+3 more)

### Community 172 - "RemoveTrackCommand"
Cohesion: 0.04
Nodes (40): AddTrackCommand, execute, index_, minted_, track_, undo, RemovedClip, size_t (+32 more)

### Community 173 - "PluginParameterInfo"
Cohesion: 0.09
Nodes (17): AutomationNode, bindings_, tempoMap_, Binding, size_t, vector, ParameterSink, StateIO (+9 more)

### Community 174 - "Smoother"
Cohesion: 0.18
Nodes (9): atomic, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds, sampleRate_ (+1 more)

### Community 175 - "AudioUnitParameterDescription"
Cohesion: 0.15
Nodes (13): AudioUnitDescription, isInstrument, manufacturer, name, uid, AudioUnitParameterDescription, defaultValue, id (+5 more)

### Community 176 - "INCDAWInsertParameterPanel"
Cohesion: 0.16
Nodes (12): NSObject, NSView, INCDAWFlippedView, -drawRect, -isFlipped, INCDAWInsertParameterPanel, +makePanelWithTitlerowsonWrite, +refreshAppearance (+4 more)

### Community 177 - "renderNode"
Cohesion: 0.21
Nodes (11): FrameCount, path, Sample, shared_ptr, size_t, vector, makeAudio(), renderNode() (+3 more)

### Community 178 - "EditAssetRegionCommand"
Cohesion: 0.18
Nodes (10): AudioEditOp, EditAssetRegionCommand, after_, applied_, asset_, before_, factor_, minted_ (+2 more)

### Community 179 - "Phase 18 Measured Baseline (2026-08-16)"
Cohesion: 0.18
Nodes (12): Theme (single drawn design language for the shell), DelayLineNode, engine::GraphBuilder::compile, Plugin Delay Compensation, D-006 — UI: AppKit shell + INCDAW-owned Metal-rendered widget layer, D-015 — The Channel Rack is drawn with CoreGraphics, not Metal, D-019 — Delay compensation lives in the graph compiler, D-035 — One drawn design language for the shell: FL Studio's density, GarageBand's calm (+4 more)

### Community 180 - "D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded)"
Cohesion: 0.18
Nodes (12): D-002 — Build system: CMake + Ninja, D-003 — Audio I/O: CoreAudio HAL directly, no wrapper framework, D-005 — Platform strategy: macOS first, Windows later, Linux not precluded, D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded), D-008 — Licensing: INCDAW is closed-source, D-011 — Metal shaders are compiled at runtime, not built offline, D-027 — CLAP SDK vendored, pinned at 1.2.6, D-031 — Plugin instances live for their slot's lifetime, not the graph's (+4 more)

### Community 181 - "Phase 1 — Foundation and Build System"
Cohesion: 0.20
Nodes (12): CLAP Format Support (first), Ad-Hoc Signing Instead of Notarization (D-009), tools/make-dmg.sh, Release Notes 0.9.0, INCDAW Release Process, Out-of-Scope Product Decisions, Deliberately Out of Scope, Phase 0 — Research and Architecture (+4 more)

### Community 182 - "ClapInstance"
Cohesion: 0.38
Nodes (7): ClapInstance, GraphCompileOptions::insertFactory, Plugin Latency Reporting and PDC Feed, PluginInstance (format-agnostic interface), PluginInstanceManager, PluginNode, plugins::PluginParameterInfo

### Community 183 - "ChannelCommands.cpp"
Cohesion: 0.24
Nodes (8): SetChannelPanCommand, canMergeWith, channelId_, execute, mergeWith, pan_, previousPan_, undo

### Community 184 - "KernelTable"
Cohesion: 0.21
Nodes (9): SampleRate, vector, KernelTable, phases, weights_, width, resample(), sinc() (+1 more)

### Community 185 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 186 - "SetChannelInstrumentCommand"
Cohesion: 0.18
Nodes (9): vector, SetChannelInstrumentCommand, channelId_, execute, instrument_, previousInstrument_, previousParameters_, previousStateFile_ (+1 more)

### Community 187 - "incdaw_engine layer library"
Cohesion: 0.10
Nodes (21): engine/dsp/Fft (own radix-2), Compiled graph owns its own tempo map, engine::dsp::MetronomeNode, engine::dsp::resample (windowed-sinc), SetTempoCommand, SetTimeSignatureCommand, Spectrum analyzer (seqlock-published dBFS bins), engine::dsp::timeStretch (offline WSOLA) (+13 more)

### Community 188 - "AutomationTests.cpp"
Cohesion: 0.38
Nodes (4): AutomationPoint, Tick, enginePoint(), modelPoint()

### Community 189 - "Fixture"
Cohesion: 0.18
Nodes (9): CommandRegistry, Tick, vector, Fixture, channel, pattern, project, registry (+1 more)

### Community 190 - "renderArrangement"
Cohesion: 0.23
Nodes (10): FrameCount, path, Sample, size_t, vector, makeAudio(), renderArrangement(), ScratchDir (+2 more)

### Community 191 - "Fixture"
Cohesion: 0.20
Nodes (10): Tick, vector, Fixture, channel, pattern, project, trackA, trackB (+2 more)

### Community 192 - "SetChannelVolumeCommand"
Cohesion: 0.18
Nodes (8): SetChannelVolumeCommand, canMergeWith, channelId_, execute, mergeWith, previousVolume_, undo, volume_

### Community 193 - "ChildResult"
Cohesion: 0.18
Nodes (10): End, ChildResult, code, end, output, path, string, vector (+2 more)

### Community 194 - "SettingsWindow.mm"
Cohesion: 0.14
Nodes (32): NSAlert, NSButton, NSTabView, NSTabViewItem, -addRowtoContentatYwidth, -applyEditedPalette, -buildAppearancePage, -buildAudioPage (+24 more)

### Community 195 - "MixerCommands.cpp"
Cohesion: 0.24
Nodes (8): SetMixerPanCommand, canMergeWith, execute, mergeWith, nodeId_, pan_, previous_, undo

### Community 196 - "SetMixerStereoSeparationCommand"
Cohesion: 0.18
Nodes (8): SetMixerStereoSeparationCommand, canMergeWith, execute, mergeWith, nodeId_, previous_, separation_, undo

### Community 197 - "TimelineAnchor"
Cohesion: 0.29
Nodes (6): FramePosition, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 198 - "SetMixerVolumeCommand"
Cohesion: 0.18
Nodes (8): SetMixerVolumeCommand, canMergeWith, execute, mergeWith, nodeId_, previous_, undo, volume_

### Community 199 - "timeStretch"
Cohesion: 0.29
Nodes (9): size_t, vector, detectOnsets(), monoMixOf(), similarityAt(), StretchOptions, pitchSemitones, ratio (+1 more)

### Community 200 - "SharedLibrary"
Cohesion: 0.25
Nodes (7): path, string, SharedLibrary, close, handle_, open, symbol

### Community 201 - "humanizeNoteStarts"
Cohesion: 0.29
Nodes (10): appendRecordedEvents(), Kind, MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts(), nextRandom() (+2 more)

### Community 202 - "SetSendGainCommand"
Cohesion: 0.18
Nodes (8): SetSendGainCommand, canMergeWith, connectionId_, execute, gain_, mergeWith, previous_, undo

### Community 203 - "PatternListView.mm"
Cohesion: 0.22
Nodes (10): -acceptsFirstResponder, -drawRect, -initWithFrameprojectregistry, -isFlipped, -keyDown, -mouseDown, NSMenu, -renameFromMenu (+2 more)

### Community 204 - "Instrument"
Cohesion: 0.14
Nodes (10): MidiBuffer, ParameterSink, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare (+2 more)

### Community 205 - "TonePanel.mm"
Cohesion: 0.27
Nodes (15): INCDAWToneView, -advancedSliderRectAt, -advancedToggleRect, -curveRect, -drawAdvanced, -drawBands, -drawCurve, -drawRect (+7 more)

### Community 206 - "AddSamplerZoneCommand"
Cohesion: 0.18
Nodes (10): AddSamplerZoneCommand, asset_, assetId_, assetIndex_, channelId_, created_, minted_, path_ (+2 more)

### Community 207 - "collectForRange"
Cohesion: 0.22
Nodes (8): FrameCount, FramePosition, MidiBuffer, vector, clear, collectForRange, rebuildIndices, setNotes

### Community 208 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 209 - "processThrough"
Cohesion: 0.38
Nodes (9): FrameCount, Sample, size_t, vector, processThrough(), requireBitExact(), requireFiniteAndBounded(), rmsOf() (+1 more)

### Community 210 - "DiskStreamer"
Cohesion: 0.22
Nodes (9): DiskStreamer, mutex_, running_, serviceOnce, streams_, atomic, mutex, vector (+1 more)

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

### Community 216 - "AddChannelCommand"
Cohesion: 0.20
Nodes (7): AddChannelCommand, channel_, execute, index_, minted_, undo, size_t

### Community 217 - "PianoModelSpec"
Cohesion: 0.15
Nodes (13): PianoModelSpec, decayScale, fm, fmIndex, fmIndexTime, fmRatio, hammerLevel, hfDecay (+5 more)

### Community 218 - "MidiDeviceInfo"
Cohesion: 0.40
Nodes (5): string, MidiDeviceInfo, identifier, isInput, name

### Community 219 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (8): size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_, step_

### Community 220 - "PluginPickerModel.cpp"
Cohesion: 0.35
Nodes (11): categoryFor(), size_t, string, lowered(), matches(), addBuiltinEffects, addHosted, rebuild (+3 more)

### Community 221 - "SplitClipCommand"
Cohesion: 0.22
Nodes (7): SplitClipCommand, clip_, minted_, previous_, right_, splitTick_, undo

### Community 222 - "DisconnectMixerCommand"
Cohesion: 0.20
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 223 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 224 - "BuiltinInstrumentInfo"
Cohesion: 0.20
Nodes (9): BuiltinInstrumentInfo, displayName, parameterCount, parameters, presets, uid, string, findBuiltinInstrument() (+1 more)

### Community 225 - "exportArrangement"
Cohesion: 0.25
Nodes (7): size_t, notes_, path, Result, uint64_t, exportArrangement(), importAsPattern()

### Community 226 - "string"
Cohesion: 0.24
Nodes (6): string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 227 - "HttpResponse"
Cohesion: 0.25
Nodes (5): string, HttpResponse, body, error, statusCode

### Community 228 - "BlobWriter"
Cohesion: 0.25
Nodes (7): BlobWriter, out, overflowed, saveState, descriptors, vector, main()

### Community 229 - "Denormals.h"
Cohesion: 0.39
Nodes (5): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister()

### Community 230 - "AudioDevice (platform device interface)"
Cohesion: 0.29
Nodes (8): app::AppSettings::audio (device configuration), AudioDevice (platform device interface), Audio Logger (60-second master ring buffer), os_workgroup Realtime Thread Scheduling, Separate Input and Output Device Selection, D-004 — Realtime thread scheduling: os_workgroup / Audio Workgroups, D-036 — Machine settings live in their own versioned file, never in the project, FL Studio 2026 Gap Analysis (INCDAW 0.9.0)

### Community 231 - "INCDAWSettingsWindow"
Cohesion: 0.25
Nodes (7): NSWindowDelegate, NSObject, NSString, INCDAWSettingsWindow, -initWithSettingsthemesDirectory, -refreshStatus, -show

### Community 232 - "UI Build-Out (post-phase increments)"
Cohesion: 0.17
Nodes (12): Plugin Editor / UI Bridge, Plugin Parameter System (D-029), ParameterRegistry (plugin registration), Event-Based Parameter Delivery (ParameterSink), params->flush() Recorded As Not Currently Applicable, Autosave, Backup and Crash Recovery (history/), Increment 11 — Settings, MIDI In, Command Search, Increment 1 — Project Lifecycle Safety (+4 more)

### Community 233 - "ClipCommands.cpp"
Cohesion: 0.18
Nodes (14): MovedAudioClip, execute, MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, canMergeWith, clips_, execute (+6 more)

### Community 234 - "TempoEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 235 - "PluginPickerEntry"
Cohesion: 0.25
Nodes (8): friend, string, PluginPickerEntry, category, name, plugin, entryAtRow, highlightedEntry

### Community 236 - "Version"
Cohesion: 0.25
Nodes (7): updateUserAgent(), Version, major, minor, patch, phase, string

### Community 237 - "TimestampedMidiMessage"
Cohesion: 0.22
Nodes (9): midiMessageReceived, sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos (+1 more)

### Community 238 - "InsertAudioCommand"
Cohesion: 0.25
Nodes (7): FramePosition, InsertAudioCommand, asset_, at_, insertedAt_, minted_, piece_

### Community 239 - "INCDAWSpectrumView"
Cohesion: 0.25
Nodes (6): NSView, INCDAWSpectrumView, -drawRect, -initWithFrame, -isOpaque, -updateWithBinssampleRate

### Community 240 - "Fixture"
Cohesion: 0.25
Nodes (6): Fixture, audioClip, audioTrack, patternClip, patternTrack, project

### Community 241 - "AutomationProbe"
Cohesion: 0.29
Nodes (6): AutomationProbe, calls, registry, written, FramePosition, vector

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
Cohesion: 0.22
Nodes (10): BlobReader, cursor, data, size, blobWrite(), loadState, process, clap_ostream_t (+2 more)

### Community 246 - "RemoveClipsCommand"
Cohesion: 0.20
Nodes (9): string, RemovedClip, vector, RemoveClipsCommand, clips_, execute, name, removed_ (+1 more)

### Community 247 - "INCDAWPianoRollView"
Cohesion: 0.29
Nodes (6): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -pruneSelectionAfterHistoryChange, -requestRedraw

### Community 248 - "ScratchDir"
Cohesion: 0.33
Nodes (5): FrameCount, path, ScratchDir, path, writeWav()

### Community 249 - "StateFixture"
Cohesion: 0.17
Nodes (11): anyNonZero(), Sample, uint8_t, vector, gainBlob(), render(), StateFixture, channel (+3 more)

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

### Community 254 - "RenderResult"
Cohesion: 0.20
Nodes (9): FrameCount, string, vector, RenderResult, arrangementFrames, audio, error, succeeded (+1 more)

### Community 255 - "ResizeClipsCommand"
Cohesion: 0.20
Nodes (9): FrameCount, ResizeClipsCommand, canMergeWith, clips_, lengthDelta_, mergeWith, previousFrameLengths_, previousLengths_ (+1 more)

### Community 256 - "build"
Cohesion: 0.27
Nodes (9): bucketize(), Bucket, FrameCount, path, Result, Sample, vector, sizeBuckets() (+1 more)

### Community 257 - "StretchClipsCommand"
Cohesion: 0.22
Nodes (8): Snapshot, StretchClipsCommand, canMergeWith, clips_, lengthDelta_, mergeWith, previous_, undo

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

### Community 265 - "SetChannelStepKeyCommand"
Cohesion: 0.22
Nodes (6): SetChannelStepKeyCommand, channelId_, execute, key_, previousKey_, undo

### Community 266 - "Fixture"
Cohesion: 0.22
Nodes (8): CommandRegistry, Tick, Fixture, channel, pattern, project, registry, note()

### Community 267 - "setParameter"
Cohesion: 0.22
Nodes (3): FilterMode, uint32_t, setParameter

### Community 268 - "main"
Cohesion: 0.32
Nodes (7): activeVoiceCount, activeVoiceCount, time_point, vector, main(), median(), millisecondsSince()

### Community 269 - "SessionFixture"
Cohesion: 0.40
Nodes (4): path, string, SessionFixture, root

### Community 270 - "BuiltinInsertTests.cpp"
Cohesion: 0.50
Nodes (3): StateIO, uint32_t, decodedValue()

### Community 271 - "ScratchDirectory"
Cohesion: 0.33
Nodes (4): path, string, ScratchDirectory, path

### Community 273 - "collectForBlock"
Cohesion: 0.29
Nodes (6): FrameCount, MidiBuffer, SampleRate, uint64_t, collectForBlock, resetCounters

### Community 274 - "AppSettingsTests.cpp"
Cohesion: 0.50
Nodes (4): path, string, scratchFile(), writeText()

### Community 275 - "MidiTests.cpp"
Cohesion: 0.29
Nodes (7): FrameCount, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote(), timestamped()

### Community 276 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 279 - "ProjectSession.cpp"
Cohesion: 0.33
Nodes (8): autosaveIsNewer(), autosavePathFor(), path, size_t, string, vector, exportFileName(), updatedRecents()

### Community 281 - "SetClipMutedCommand"
Cohesion: 0.13
Nodes (7): string, SetClipMutedCommand, clips_, execute, muted_, previous_, undo

### Community 282 - "Performance Targets (audio, UI, project)"
Cohesion: 0.67
Nodes (3): Audio Callback Performance Budget (2.67 ms at 48 kHz / 128 frames), Reference Machine (Apple M5, 10 cores, 16 GB, macOS 26.2), Performance Targets (audio, UI, project)

### Community 283 - "create"
Cohesion: 0.25
Nodes (8): clap_event_param_value_t, create, array, string, unique_ptr, PendingParamEvents, count, events

### Community 285 - "INCDAWControlBarView"
Cohesion: 0.38
Nodes (6): NSInteger, NSTextFieldDelegate, NSString, NSView, INCDAWControlBarView, INCDAWStatusBarView

### Community 299 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 300 - "RemoveSamplerZoneCommand"
Cohesion: 0.25
Nodes (6): size_t, RemoveSamplerZoneCommand, channelId_, removed_, undo, zoneIndex_

### Community 301 - "SetChannelMutedCommand"
Cohesion: 0.25
Nodes (5): SetChannelMutedCommand, channelId_, execute, muted_, undo

### Community 302 - "MidiImportResult"
Cohesion: 0.25
Nodes (7): string, vector, MidiImportResult, error, newChannels, pattern, succeeded

### Community 303 - "SetChannelSoloedCommand"
Cohesion: 0.25
Nodes (5): SetChannelSoloedCommand, channelId_, execute, soloed_, undo

### Community 304 - "SetMixerMutedCommand"
Cohesion: 0.25
Nodes (5): SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 305 - "SequencedNote"
Cohesion: 0.23
Nodes (11): SequencedNote, channel, key, lengthTicks, startTick, velocity, Tick, vector (+3 more)

### Community 306 - "SetMixerPolarityCommand"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 307 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 308 - "INCDAWAudioEditorView"
Cohesion: 0.40
Nodes (4): NSView, INCDAWAudioEditorView, -initWithFrameprojectregistry, -reloadWaveform

### Community 309 - "INCDAWTonePanel"
Cohesion: 0.40
Nodes (4): NSObject, INCDAWTonePanel, +makePanelWithTitlerowssampleRateonWrite, +refreshWindowvalues

### Community 310 - "ThemePaletteTests.cpp"
Cohesion: 0.50
Nodes (4): path, string, scratchFile(), writeText()

### Community 311 - "EffectParameter"
Cohesion: 0.25
Nodes (7): EffectParameter, defaultValue, id, maxValue, minValue, name, stepped

### Community 312 - "MintedAsset"
Cohesion: 0.29
Nodes (7): size_t, MintedAsset, copy, created, id, index, ok

### Community 313 - "prepare"
Cohesion: 0.43
Nodes (7): prepare, FrameCount, SampleRate, size_t, prepare, prepare, ringSizeFor()

### Community 315 - "emptyOutTryPush"
Cohesion: 0.33
Nodes (6): clap_event_header_t, clap_input_events_t, clap_output_events_t, emptyOutTryPush(), pendingInGet(), pendingInSize()

### Community 316 - "DitherSource"
Cohesion: 0.47
Nodes (3): uint64_t, DitherSource, state_

### Community 317 - "StateIO"
Cohesion: 0.40
Nodes (3): StateIO, loadState, saveState

### Community 318 - "uint32_t"
Cohesion: 0.40
Nodes (5): hasEditor, openEditor, readParameter, setParameter, uint32_t

### Community 319 - "-mouseDragged"
Cohesion: 0.50
Nodes (4): NSDraggingItem, NSImage, NSPasteboardItem, -mouseDragged

### Community 320 - "loopWithHits"
Cohesion: 0.50
Nodes (3): size_t, vector, loopWithHits()

### Community 321 - "UpdateCheckTests.cpp"
Cohesion: 0.67
Nodes (3): string, entry(), feed()

## Ambiguous Edges - Review These
- `Clip / Project Data Model` → `Undo / Redo`  [AMBIGUOUS]
  CLAUDE.md · relation: shares_data_with

## Knowledge Gaps
- **1721 isolated node(s):** `currentVersion`, `audio`, `openInputAtLaunch`, `midiInputIdentifiers`, `workspace` (+1716 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **10 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `Clip / Project Data Model` and `Undo / Redo`?**
  _Edge tagged AMBIGUOUS (relation: shares_data_with) - confidence is low._
- **Why does `Project` connect `Project` to `RemoveMixerNodeCommand`, `NoteCommands.cpp`, `WriteAutomationCommand`, `PresetLibrary.cpp`, `PatternCommands.cpp`, `PluginInstanceManager`, `ConnectMixerCommand`, `AddInsertCommand`, `AddMarkerCommand`, `EntityId`, `ArpeggiateNotesCommand`, `PluginInsertTests.cpp`, `CommandRegistry`, `NudgeChordCommand`, `DelayLineNode`, `TempoMap`, `CompiledProjectGraph`, `SetTempoCommand`, `ImportAudioClipCommand`, `Clip`, `read`, `AddMidiMappingCommand`, `SetSamplerZoneCommand`, `ParameterFixture`, `InsertRecordedTakeCommand`, `PluginIdentifier`, `PlaylistModel`, `load`, `MidiEvent`, `SetInstrumentParameterCommand`, `AddMixerNodeCommand`, `allocate`, `SplitFixture`, `DuplicateClipsCommand`, `compileProjectGraph`, `RemoveChannelCommand`, `Channel`, `capturePluginState`, `EditFixture`, `Pattern`, `SidechainFixture`, `string`, `SamplerWiringTests.cpp`, `AddPatternClipCommand`, `RemoveTrackCommand`, `ChannelCommands.cpp`, `SetChannelInstrumentCommand`, `Fixture`, `renderArrangement`, `Fixture`, `SetChannelVolumeCommand`, `MixerCommands.cpp`, `SetMixerStereoSeparationCommand`, `SetMixerVolumeCommand`, `SetSendGainCommand`, `AddSamplerZoneCommand`, `AddChannelCommand`, `SplitClipCommand`, `DisconnectMixerCommand`, `exportArrangement`, `string`, `ClipCommands.cpp`, `Fixture`, `AutomationProbe`, `RenderTests.cpp`, `RemoveClipsCommand`, `StateFixture`, `ResizeClipsCommand`, `StretchClipsCommand`, `SetChannelStepKeyCommand`, `Fixture`, `SetClipMutedCommand`, `SetChannelOutputCommand`, `RemoveSamplerZoneCommand`, `SetChannelMutedCommand`, `SetChannelSoloedCommand`, `SetMixerMutedCommand`, `SetMixerPolarityCommand`, `SetMixerSoloedCommand`?**
  _High betweenness centrality (0.110) - this node is a cross-community bridge._
- **Why does `EntityId` connect `EntityId` to `RemoveMixerNodeCommand`, `NoteCommands.cpp`, `WriteAutomationCommand`, `PresetLibrary.cpp`, `PatternCommands.cpp`, `PianoRollModel`, `ConnectMixerCommand`, `AddInsertCommand`, `AddMarkerCommand`, `ArpeggiateNotesCommand`, `PluginInsertTests.cpp`, `StretchAssetCommand`, `NudgeChordCommand`, `InstrumentNode`, `DelayLineNode`, `CompiledProjectGraph`, `LoadSampleCommand`, `ImportAudioClipCommand`, `GraphCompileOptions`, `Clip`, `read`, `AddMidiMappingCommand`, `ChannelSamplerZone`, `SetSamplerZoneCommand`, `ParameterFixture`, `AutomationWriteSession`, `InsertRecordedTakeCommand`, `RenderOptions`, `PlaylistModel`, `load`, `MidiEvent`, `SetInstrumentParameterCommand`, `AddMixerNodeCommand`, `Project`, `allocate`, `SplitFixture`, `compileProjectGraph`, `RemoveChannelCommand`, `Channel`, `capturePluginState`, `EditFixture`, `Pattern`, `SidechainFixture`, `string`, `SamplerWiringTests.cpp`, `AddPatternClipCommand`, `RemoveTrackCommand`, `EditAssetRegionCommand`, `ChannelCommands.cpp`, `SetChannelInstrumentCommand`, `Fixture`, `Fixture`, `SetChannelVolumeCommand`, `MixerCommands.cpp`, `SetMixerStereoSeparationCommand`, `SetMixerVolumeCommand`, `SetSendGainCommand`, `AddSamplerZoneCommand`, `AddChannelCommand`, `SplitClipCommand`, `DisconnectMixerCommand`, `string`, `ClipCommands.cpp`, `InsertAudioCommand`, `Fixture`, `StateFixture`, `SetChannelStepKeyCommand`, `Fixture`, `ClipIds`, `SetChannelOutputCommand`, `RemoveSamplerZoneCommand`, `SetChannelMutedCommand`, `MidiImportResult`, `SetChannelSoloedCommand`, `SetMixerMutedCommand`, `SetMixerPolarityCommand`, `SetMixerSoloedCommand`, `MintedAsset`?**
  _High betweenness centrality (0.094) - this node is a cross-community bridge._
- **Why does `Node` connect `Node` to `ConstantNode`, `AudioClipNode`, `PluginInstanceManager`, `ProcessContext`, `compileProjectGraph`, `GainNode`, `NoteSequence`, `BuiltinEffectTests.cpp`, `BuiltinEffectInfo`, `PluginInsertTests.cpp`, `GraphBuilder`, `atomic`, `InstrumentNode`, `SidechainFixture`, `DelayLineNode`, `BuiltinEffect`, `CompiledProjectGraph`, `PluginParameterInfo`, `Smoother`, `MetronomeNode`, `renderNode`, `AutomationTests.cpp`, `HostedPlugin`, `processThrough`, `CompiledGraph`, `ClapLibrary`, `SampleRingBuffer`, `MixerStripNode`, `MidiMapNode`, `allocate`?**
  _High betweenness centrality (0.067) - this node is a cross-community bridge._
- **What connects `currentVersion`, `audio`, `openInputAtLaunch` to the rest of the system?**
  _1721 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Browser` be split into smaller, more focused modules?**
  _Cohesion score 0.07179487179487179 - nodes in this community are weakly interconnected._
- **Should `NoteCommands.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.04288288288288288 - nodes in this community are weakly interconnected._