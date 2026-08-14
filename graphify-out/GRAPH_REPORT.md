# Graph Report - project-continuation-670d11  (2026-08-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 2943 nodes · 5069 edges · 201 communities (167 shown, 34 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 242 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `e35fbe3d`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- INCDAW
- string
- RemoveTrackCommand
- string
- PlaylistModel
- WavBytes.h
- PianoRollModel
- CoreAudioDevice
- SimpleSynth
- MixerNode
- Pattern
- Transport
- CommandRegistry
- ChannelRackModel
- NoteSequence
- Project
- TempoMap
- main
- Clip
- MetronomeNode
- AudioDeviceConfig
- CoreMidiDevice
- Model.cpp
- MusicalPosition
- CoreAudioDevice.cpp
- ProjectFile.cpp
- MixerView.mm
- AudioEngine
- AudioRecorder
- MidiInput
- Options
- Json
- CallbackProfiler
- MidiMessage
- AudioDevice
- AudioBufferPool
- NoteCommands.cpp
- InsertRecordedTakeCommand
- AudioBufferView
- atomic
- PlaylistView.mm
- RecordingSession
- compileArrangement
- DelayLineNode
- CompiledGraph
- CountingCommand
- RealtimeGuard.cpp
- GraphBuilder
- WavStreamReader
- -applicationDidFinishLaunching
- AudioClipNode
- GraphCompileOptions
- ProcessContext
- BasicMidiBuffer
- SystemInfo
- RemoveClipsCommand
- MixerCommands.cpp
- LevelMeter
- InstrumentNode
- ConstantNode
- vector
- ClipCommands.cpp
- vector
- LockFreeQueue
- SampleRingBuffer
- Smoother
- MixerStripNode
- compile
- Parser
- LoopbackResult
- LatentProcessorNode
- ioProcTrampoline
- WavStreamWriter
- SetVelocityCommand
- GainNode
- SineOscillatorNode
- EntityId
- TimingProbeInstrument
- MidiEvent
- AudioStream
- open
- PluginIdentifier
- Json.cpp
- CompiledProjectGraph
- AddPatternClipCommand
- string
- EntityId
- QuantizeNotesCommand
- AddPatternCommand
- DuplicatePatternCommand
- PatternCommands.cpp
- AudioStream.cpp
- renderClickFrames
- renderArrangement
- Fixture
- AutomationPoint
- AutomationNode
- ToggleStepCommand
- Channel
- ParameterRegistry
- MidiRecorder
- humanizeNoteStarts
- RemoveMixerNodeCommand
- Instrument
- MixerTests.cpp
- WavStreamWriterTests.cpp
- DiskStreamer
- renderNode
- MidiTests.cpp
- AddMixerNodeCommand
- AddNoteCommand
- captureAudioBlock
- AudioFileData
- TimelineAnchor
- MixerStripNode.cpp
- TimeSignatureEvent
- MidiDevice
- main.mm
- MixerFixture
- DuplicateClipsCommand
- DisconnectMixerCommand
- DeleteNotesCommand
- readAt
- RecordedEvent
- RoutingConnection
- make-dmg.sh
- ProjectGraphCompiler.cpp
- MoveClipsCommand
- SetChannelOutputCommand
- SetMixerPanCommand
- SetMixerVolumeCommand
- MoveNotesCommand
- SetPatternLengthCommand
- MidiDevice.h
- AutomationFixture
- Denormals.h
- ResizeClipsCommand
- Command
- MixerCommands.h
- SetMixerSoloedCommand
- ResizeNotesCommand
- RemovePatternCommand
- Version
- TimestampedMidiMessage
- makeTestSignal
- compileProjectGraph
- PatternTests.cpp
- ScratchDirectory
- INCDAWMixerView
- ProjectGraphCompiler.h
- INCDAWPianoRollView
- InstrumentTests.cpp
- string
- noteAtStep
- v1.0/Fixture.incdaw/manifest.json
- v1.1/Fixture.incdaw/manifest.json
- ChannelRackTests.cpp
- OrderRecordingNode
- MidiRecorder.cpp
- check
- Command
- AudioCaptureSink
- INCDAWChannelRackView
- INCDAWPatternListView
- INCDAWPlaylistView
- .operator==
- CallbackProfiler
- FrameCount
- FramePosition
- NSMenu
- Result
- Sample
- SampleRate
- AutomationLane
- Project
- AutomationLane
- Rect
- Node
- T
- Kind
- friend
- MidiEventType
- MixerNodeType
- PluginIdentifier
- uint32_t
- TempoMap
- CompiledGraph
- string
- unique_ptr
- NSView
- Step
- Project
- Tick
- uint64_t
- uint8_t
- Viewport

## God Nodes (most connected - your core abstractions)
1. `Project` - 173 edges
2. `AudioEngine` - 60 edges
3. `CoreAudioDevice` - 59 edges
4. `Json` - 44 edges
5. `Clip` - 40 edges
6. `TempoMap` - 38 edges
7. `Transport` - 37 edges
8. `MidiEvent` - 36 edges
9. `SimpleSynth` - 36 edges
10. `PianoRollModel` - 34 edges

## Surprising Connections (you probably didn't know these)
- `Audio Engine` --semantically_similar_to--> `Audio Engine Priority`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Controller Linking` --semantically_similar_to--> `Parameter System`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Clip / Project Data Model` --semantically_similar_to--> `Core Data Model`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Development Phases (0-20)` --semantically_similar_to--> `Feature Roadmap (Phase 0-20)`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Required Documentation Set` --semantically_similar_to--> `Handoff Rule`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Approval-Gated Development Protocol** — claude_absolute_user_control_rule, claude_graphify_mandate, claude_feature_workflow, claude_scope_control, claude_dependency_policy, handoff_critical_operating_rule, handoff_feature_protocol, handoff_handoff_rule [EXTRACTED 1.00]
- **Plugin Host Pipeline** — handoff_plugin_scanner, handoff_plugin_registry, handoff_plugin_instance, handoff_parameter_system, handoff_plugin_state_system, handoff_plugin_ui_bridge, handoff_crash_isolation_strategy [EXTRACTED 1.00]
- **Master Signal Chain Convergence** — handoff_midi_signal_flow, handoff_audio_signal_flow, handoff_plugin_automation_flow, handoff_shared_transport_state, claude_mixer, claude_automation, claude_offline_render_engine, claude_core_transport [INFERRED 0.85]

## Communities (201 total, 34 thin omitted)

### Community 0 - "INCDAW"
Cohesion: 0.06
Nodes (84): Absolute User Control Rule, Audio Editor, Audio Engine, Audio Correctness Requirements, Automation Subsystem, Browser, Channel / Instrument System, Command Registry / Keyboard Shortcuts (+76 more)

### Community 1 - "string"
Cohesion: 0.04
Nodes (49): RemovedContent, AddChannelCommand, channel_, execute, index_, minted_, undo, Command (+41 more)

### Community 2 - "RemoveTrackCommand"
Cohesion: 0.05
Nodes (43): AddTrackCommand, execute, index_, minted_, track_, undo, Command, Command (+35 more)

### Community 3 - "string"
Cohesion: 0.05
Nodes (45): AddAutomationLaneCommand, index_, key_, lane_, minted_, target_, Command, AutomationPoint (+37 more)

### Community 4 - "PlaylistModel"
Cohesion: 0.07
Nodes (43): Rect, EntityId, size_t, Tick, vector, EntityId, size_t, Tick (+35 more)

### Community 5 - "WavBytes.h"
Cohesion: 0.09
Nodes (46): appendCanonicalHeader(), bitsFor(), codeFor(), decodeSample(), encodeSample(), FormatInfo, bitsPerSample, channels (+38 more)

### Community 6 - "PianoRollModel"
Cohesion: 0.09
Nodes (32): NoteList, size_t, Tick, vector, size_t, Tick, Viewport, PianoRollModel (+24 more)

### Community 7 - "CoreAudioDevice"
Cohesion: 0.06
Nodes (31): AudioDeviceIOProcID, CoreAudioDevice, bufferSize_, bufferSizeToRestore_, callback_, deviceID_, inputBufferSize_, inputBufferSizeToRestore_ (+23 more)

### Community 8 - "SimpleSynth"
Cohesion: 0.07
Nodes (35): FrameCount, SampleRate, size_t, Voice, Waveform, frequencyForKey(), array, atomic (+27 more)

### Community 9 - "MixerNode"
Cohesion: 0.05
Nodes (40): MixerNodeType, PluginIdentifier, AudioAsset, absolutePath, channelCount, contentHash, embedded, frameCount (+32 more)

### Community 10 - "Pattern"
Cohesion: 0.06
Nodes (28): AutomationCurve, friend, AutomationLane, id, parameterKey, points, targetEntity, AutomationPoint (+20 more)

### Community 11 - "Transport"
Cohesion: 0.09
Nodes (24): FrameCount, FramePosition, size_t, atomic, FramePosition, size_t, Tick, Transport (+16 more)

### Community 12 - "CommandRegistry"
Cohesion: 0.11
Nodes (27): CommandRegistry, actions_, clearHistory, execute, executeMerging, findAction, invoke, project_ (+19 more)

### Community 13 - "ChannelRackModel"
Cohesion: 0.16
Nodes (25): Hit, Layout, ChannelRackModel, contentHeight, hitTest, layout_, muteRect, nameRect (+17 more)

### Community 14 - "NoteSequence"
Cohesion: 0.09
Nodes (25): FrameCount, FramePosition, MidiBuffer, Tick, vector, size_t, Tick, uint32_t (+17 more)

### Community 15 - "Project"
Cohesion: 0.10
Nodes (28): IdGenerator, execute, undo, AutomationPoint, EntityId, vector, findLane(), execute (+20 more)

### Community 16 - "TempoMap"
Cohesion: 0.12
Nodes (26): Segment, clampTempo(), FramePosition, SampleRate, Tick, vector, FramePosition, SampleRate (+18 more)

### Community 17 - "main"
Cohesion: 0.12
Nodes (27): AudioDeviceConfig, audioDeviceAboutToStart, audioDeviceStopped, bufferSize, captureAudioBlock, collectRetiredGraphs, deviceName, inputChannels (+19 more)

### Community 18 - "Clip"
Cohesion: 0.07
Nodes (29): ClipType, Clip, colour, fadeInFrames, fadeOutFrames, gain, id, length (+21 more)

### Community 19 - "MetronomeNode"
Cohesion: 0.08
Nodes (22): FrameCount, SampleRate, atomic, FrameCount, Sample, SampleRate, size_t, vector (+14 more)

### Community 20 - "AudioDeviceConfig"
Cohesion: 0.08
Nodes (23): AudioDeviceConfig, bufferSize, defaultInput, inputChannels, inputDeviceIdentifier, outputChannels, outputDeviceIdentifier, sampleRate (+15 more)

### Community 21 - "CoreMidiDevice"
Cohesion: 0.14
Nodes (24): CFStringRef, MIDIClientRef, MIDIEndpointRef, MIDIObjectRef, MIDIPacketList, MIDIPortRef, CoreMidiDevice, callback_ (+16 more)

### Community 22 - "Model.cpp"
Cohesion: 0.17
Nodes (25): undo, EntityId, size_t, vector, operator==(), events, totalEventCount, findChannel (+17 more)

### Community 23 - "MusicalPosition"
Cohesion: 0.11
Nodes (20): Tick, framesToSeconds(), FrameCount, friend, int64_t, SampleRate, Tick, MusicalPosition (+12 more)

### Community 24 - "CoreAudioDevice.cpp"
Cohesion: 0.27
Nodes (24): AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyScope, AudioObjectPropertySelector, address(), allDeviceIDs(), channelCount(), close (+16 more)

### Community 25 - "ProjectFile.cpp"
Cohesion: 0.20
Nodes (23): Json, automationPointFrom(), bindUnassignedContent(), AutomationPoint, EntityId, path, PluginIdentifier, Result (+15 more)

### Community 26 - "MixerView.mm"
Cohesion: 0.18
Nodes (25): -acceptsFirstResponder, -addMixerTrack, -addStripRect, -applyFaderAtindex, -applyPanAtindex, -drawRect, -drawStripnode, -faderRectAt (+17 more)

### Community 27 - "AudioEngine"
Cohesion: 0.11
Nodes (22): AudioDevice, AudioIOCallback, MidiBuffer, mutex, RetiredGraph, AudioCaptureSink, AudioEngine, active_ (+14 more)

### Community 28 - "AudioRecorder"
Cohesion: 0.10
Nodes (20): AudioCaptureSink, AudioRecorder, firstBlockHostTimeNanos_, framesOnDisk_, interleaveScratch_, options_, path_, ring_ (+12 more)

### Community 29 - "MidiInput"
Cohesion: 0.11
Nodes (19): FrameCount, MidiBuffer, SampleRate, uint64_t, atomic, queueCapacity, size_t, uint64_t (+11 more)

### Community 30 - "Options"
Cohesion: 0.09
Nodes (22): AudioDeviceInfo, availableDevices, vector, CallbackProfiler, int64_t, string, Options, amplitude (+14 more)

### Community 31 - "Json"
Cohesion: 0.10
Nodes (15): nullptr_t, pair, int64_t, int64_t, string, vector, Json, asInt (+7 more)

### Community 32 - "CallbackProfiler"
Cohesion: 0.13
Nodes (13): CallbackProfiler, bucketCount, buckets_, bucketWidth, loadPercentile, overrunBucket, reset, underruns_ (+5 more)

### Community 33 - "MidiMessage"
Cohesion: 0.11
Nodes (9): FrameCount, uint8_t, MidiMessage, data1, data2, frameOffset, status, vector (+1 more)

### Community 34 - "AudioDevice"
Cohesion: 0.09
Nodes (22): AudioDevice, actualBufferSize, actualInputChannels, actualOutputChannels, actualSampleRate, close, create, deviceName (+14 more)

### Community 35 - "AudioBufferPool"
Cohesion: 0.13
Nodes (13): AudioBufferPool, allocate, channelPointers_, reset, samples_, FrameCount, size_t, FrameCount (+5 more)

### Community 36 - "NoteCommands.cpp"
Cohesion: 0.17
Nodes (20): undo, Command, EntityId, NoteIndices, size_t, vector, execute, findEvents() (+12 more)

### Community 37 - "InsertRecordedTakeCommand"
Cohesion: 0.11
Nodes (16): Command, EntityId, Placement, size_t, InsertRecordedTakeCommand, asset_, assetIndex_, clip_ (+8 more)

### Community 38 - "AudioBufferView"
Cohesion: 0.19
Nodes (9): renderAudioBlock, uint64_t, AudioBufferView, channels_, frames_, offset_, FrameCount, Sample (+1 more)

### Community 39 - "atomic"
Cohesion: 0.14
Nodes (5): atomic, MidiBuffer, array, Node, process

### Community 40 - "PlaylistView.mm"
Cohesion: 0.17
Nodes (20): -acceptsFirstResponder, -addTrackRect, -drawBarLinesInLaneAtheight, -drawClips, -drawRect, -drawRuler, -drawTracks, -gridPointFor (+12 more)

### Community 41 - "RecordingSession"
Cohesion: 0.12
Nodes (13): AudioRecorder, string, Placement, string, FrameCount, uint64_t, RecordingSession, arm (+5 more)

### Community 42 - "compileArrangement"
Cohesion: 0.23
Nodes (19): Emit, NoteSequence, content, arrangementLengthTicks(), compileArrangement(), compileArrangementInto(), compilePattern(), compilePatternInto() (+11 more)

### Community 43 - "DelayLineNode"
Cohesion: 0.12
Nodes (16): FrameCount, ProcessContext, SampleRate, DelayLineNode, capacity_, channelCount_, delayFrames_, history_ (+8 more)

### Community 44 - "CompiledGraph"
Cohesion: 0.12
Nodes (14): CompiledGraph, hasMaster_, inputViews_, masterBuffer_, nodes_, order_, pool_, steps_ (+6 more)

### Community 45 - "CountingCommand"
Cohesion: 0.12
Nodes (10): CountingCommand, counter_, delta_, Command, EntityId, string, Tick, makeProjectWithNotes() (+2 more)

### Community 46 - "RealtimeGuard.cpp"
Cohesion: 0.15
Nodes (13): align_val_t, nothrow_t, allocationViolations(), size_t, deallocationViolations(), isInsideRealtimeContext(), operator delete(), operator new() (+5 more)

### Community 47 - "GraphBuilder"
Cohesion: 0.14
Nodes (12): Connection, GraphBuilder, compensate_, connections_, error_, master_, nodes_, Node (+4 more)

### Community 48 - "WavStreamReader"
Cohesion: 0.12
Nodes (16): ifstream, FrameCount, path, SampleRate, size_t, uint16_t, uint64_t, uint8_t (+8 more)

### Community 49 - "-applicationDidFinishLaunching"
Cohesion: 0.14
Nodes (19): INCDAWChannelRackView, INCDAWMixerView, INCDAWPatternListView, INCDAWPianoRollView, INCDAWPlaylistView, NSApplicationDelegate, NSObject, NSScrollView (+11 more)

### Community 50 - "AudioClipNode"
Cohesion: 0.13
Nodes (15): Node, ProcessContext, AudioClipNode, addClip, clips_, fetchScratch_, prepare, process (+7 more)

### Community 51 - "GraphCompileOptions"
Cohesion: 0.11
Nodes (19): PlaybackSource, GraphCompileOptions, channelCount, diskStreamer, instrumentFactory, masterGain, maxBlockSize, parameters (+11 more)

### Community 52 - "ProcessContext"
Cohesion: 0.12
Nodes (15): FrameCount, FramePosition, MidiBuffer, SampleRate, size_t, ProcessContext, frameCount, inputCount (+7 more)

### Community 53 - "BasicMidiBuffer"
Cohesion: 0.12
Nodes (8): BasicMidiBuffer, count_, messages_, overflowed_, array, Capacity, FrameCount, size_t

### Community 54 - "SystemInfo"
Cohesion: 0.13
Nodes (15): size_t, string, size_t, string, sysctlString(), sysctlUInt(), SystemInfo, cpuBrand (+7 more)

### Community 55 - "RemoveClipsCommand"
Cohesion: 0.14
Nodes (14): ClipIds, string, Command, RemovedClip, vector, RemoveClipsCommand, clips_, name (+6 more)

### Community 56 - "MixerCommands.cpp"
Cohesion: 0.16
Nodes (14): undo, Command, canMergeWith, mergeWith, canMergeWith, mergeWith, SetSendGainCommand, canMergeWith (+6 more)

### Community 57 - "LevelMeter"
Cohesion: 0.14
Nodes (13): atomic, AudioBufferView, FrameCount, Sample, SampleRate, LevelMeter, heldPeak_, peakDecayDbPerSecond (+5 more)

### Community 58 - "InstrumentNode"
Cohesion: 0.13
Nodes (14): FrameCount, SampleRate, atomic, FramePosition, MidiBuffer, unique_ptr, InstrumentNode, blockMidi_ (+6 more)

### Community 59 - "ConstantNode"
Cohesion: 0.13
Nodes (10): ConstantNode, latency_, value_, FrameCount, Sample, size_t, SometimesSilentNode, writeThisBlock (+2 more)

### Community 60 - "vector"
Cohesion: 0.12
Nodes (8): Command, execute, id, name, undo, vector, allocationSize(), size_t

### Community 61 - "ClipCommands.cpp"
Cohesion: 0.18
Nodes (15): undo, Command, EntityId, execute, undo, canMergeWith, execute, mergeWith (+7 more)

### Community 62 - "vector"
Cohesion: 0.17
Nodes (7): vector, Node, AudioBufferPool, AutomationPoint, Tick, enginePoint(), modelPoint()

### Community 63 - "LockFreeQueue"
Cohesion: 0.15
Nodes (11): array, atomic, Capacity, size_t, T, LockFreeQueue, cacheLineSize, mask (+3 more)

### Community 64 - "SampleRingBuffer"
Cohesion: 0.21
Nodes (10): atomic, Sample, size_t, vector, SampleRingBuffer, cacheLineSize, mask_, readIndex_ (+2 more)

### Community 65 - "Smoother"
Cohesion: 0.18
Nodes (10): atomic, AudioBufferView, FrameCount, Sample, SampleRate, Smoother, coefficient_, defaultSmoothingSeconds (+2 more)

### Community 66 - "MixerStripNode"
Cohesion: 0.12
Nodes (11): ProcessContext, atomic, Node, Sample, MixerStripNode, left_, meter_, muted_ (+3 more)

### Community 67 - "compile"
Cohesion: 0.17
Nodes (16): process, AudioBufferView, FrameCount, FramePosition, MidiBuffer, Node, NodeIndex, SampleRate (+8 more)

### Community 68 - "Parser"
Cohesion: 0.25
Nodes (12): parse, Parser, depth_, error_, maxDepth, parseArray, parseLiteral, parseNumber (+4 more)

### Community 69 - "LoopbackResult"
Cohesion: 0.17
Nodes (15): alignmentErrorFrames(), FrameCount, path, Sample, Take, uint64_t, LoopbackResult, sampleRate (+7 more)

### Community 70 - "LatentProcessorNode"
Cohesion: 0.16
Nodes (10): FrameCount, FramePosition, Node, SampleRate, ImpulseNode, latency_, position_, LatentProcessorNode (+2 more)

### Community 71 - "ioProcTrampoline"
Cohesion: 0.23
Nodes (14): AudioBufferList, AudioTimeStamp, OSStatus, captureFrom, inputProcTrampoline, ioProcTrampoline, renderInto, uint64_t (+6 more)

### Community 72 - "WavStreamWriter"
Cohesion: 0.13
Nodes (14): ofstream, Format, FrameCount, path, size_t, uint8_t, vector, WavStreamWriter (+6 more)

### Community 73 - "SetVelocityCommand"
Cohesion: 0.18
Nodes (10): EntityId, NoteIndices, Tick, SetVelocityCommand, channel_, indices_, pattern_, previousVelocities_ (+2 more)

### Community 74 - "GainNode"
Cohesion: 0.16
Nodes (9): GainNode, current_, process, sampleRate_, target_, atomic, FrameCount, Sample (+1 more)

### Community 75 - "SineOscillatorNode"
Cohesion: 0.14
Nodes (9): atomic, FrameCount, Sample, SampleRate, SineOscillatorNode, amplitude_, frequency_, process (+1 more)

### Community 76 - "EntityId"
Cohesion: 0.21
Nodes (7): EntityId, invalidValue, friend, size_t, IdGenerator, std::hash<incdaw::project::EntityId>, Value

### Community 77 - "TimingProbeInstrument"
Cohesion: 0.15
Nodes (9): Applied, AudioBufferView, FrameCount, MidiMessage, SampleRate, vector, TimingProbeInstrument, applied (+1 more)

### Community 78 - "MidiEvent"
Cohesion: 0.13
Nodes (15): MidiEventType, MidiEvent, channel, duration, fineTune, key, label, pan (+7 more)

### Community 79 - "AudioStream"
Cohesion: 0.15
Nodes (11): AudioStream, lastRequested_, reader_, segmentFrames_, segments_, underruns_, FrameCount, SampleRate (+3 more)

### Community 80 - "open"
Cohesion: 0.28
Nodes (12): Format, FrameCount, path, Result, Sample, SampleRate, size_t, append (+4 more)

### Community 81 - "PluginIdentifier"
Cohesion: 0.17
Nodes (11): Format, string, formatName(), Format, friend, string, PluginIdentifier, format (+3 more)

### Community 82 - "Json.cpp"
Cohesion: 0.22
Nodes (13): size_t, string, escapeInto(), formatDouble(), append, asBool, asDouble, asString (+5 more)

### Community 83 - "CompiledProjectGraph"
Cohesion: 0.14
Nodes (14): CompiledGraph, CompiledProjectGraph, automation, channels, channelStrips, error, graph, instruments (+6 more)

### Community 84 - "AddPatternClipCommand"
Cohesion: 0.16
Nodes (11): AddPatternClipCommand, clip_, execute, index_, length_, minted_, pattern_, start_ (+3 more)

### Community 85 - "string"
Cohesion: 0.16
Nodes (6): string, RenameMixerNodeCommand, execute, nodeId_, previousName_, undo

### Community 86 - "EntityId"
Cohesion: 0.16
Nodes (11): ConnectMixerCommand, connection_, destination_, execute, gain_, index_, isSend_, minted_ (+3 more)

### Community 87 - "QuantizeNotesCommand"
Cohesion: 0.14
Nodes (8): string, QuantizeNotesCommand, channel_, grid_, pattern_, previousEvents_, strength_, undo

### Community 88 - "AddPatternCommand"
Cohesion: 0.13
Nodes (11): AddPatternCommand, index_, minted_, pattern_, Command, string, RenamePatternCommand, execute (+3 more)

### Community 89 - "DuplicatePatternCommand"
Cohesion: 0.18
Nodes (6): DuplicatePatternCommand, index_, minted_, pattern_, source_, EntityId

### Community 90 - "PatternCommands.cpp"
Cohesion: 0.16
Nodes (15): execute, undo, Command, execute, undo, canMergeWith, mergeWith, SetPatternSwingCommand (+7 more)

### Community 91 - "AudioStream.cpp"
Cohesion: 0.21
Nodes (12): fillSegment, open, prefill, read, service, FrameCount, path, Result (+4 more)

### Community 92 - "renderClickFrames"
Cohesion: 0.15
Nodes (12): BlockSegment, length, offset, startFrame, startsAfterLoopWrap, FrameCount, FrameCount, FramePosition (+4 more)

### Community 93 - "renderArrangement"
Cohesion: 0.19
Nodes (12): AudioFileData, FrameCount, path, Project, Sample, size_t, vector, makeAudio() (+4 more)

### Community 94 - "Fixture"
Cohesion: 0.16
Nodes (12): EntityId, SequencedNote, Tick, vector, Fixture, channel, pattern, project (+4 more)

### Community 95 - "AutomationPoint"
Cohesion: 0.19
Nodes (10): AutomationShape, AutomationPoint, shape, tension, tick, value, AutomationSequence, points_ (+2 more)

### Community 96 - "AutomationNode"
Cohesion: 0.18
Nodes (8): Binding, AutomationNode, bindings_, tempoMap_, ProcessContext, size_t, TempoMap, vector

### Community 97 - "ToggleStepCommand"
Cohesion: 0.17
Nodes (9): Command, size_t, Step, string, ToggleStepCommand, createdContent_, index_, note_ (+1 more)

### Community 98 - "Channel"
Cohesion: 0.15
Nodes (13): Channel, colour, id, instrument, instrumentStateFile, muted, name, outputMixerNode (+5 more)

### Community 99 - "ParameterRegistry"
Cohesion: 0.22
Nodes (11): Entry, string, Entry, size_t, vector, ParameterRegistry, entries_, find (+3 more)

### Community 100 - "MidiRecorder"
Cohesion: 0.20
Nodes (10): CapturedMessage, atomic, queueCapacity, size_t, uint64_t, MidiRecorder, captured_, dropped_ (+2 more)

### Community 101 - "humanizeNoteStarts"
Cohesion: 0.26
Nodes (11): Kind, RecordedEvent, appendRecordedEvents(), MidiEventType, Tick, uint64_t, vector, humanizeNoteStarts() (+3 more)

### Community 102 - "RemoveMixerNodeCommand"
Cohesion: 0.17
Nodes (10): RemovedRouting, vector, RemoveMixerNodeCommand, execute, index_, node_, nodeId_, reassignedChannels_ (+2 more)

### Community 103 - "Instrument"
Cohesion: 0.18
Nodes (9): MidiBuffer, Instrument, activeVoiceCount, allNotesOff, handleMessage, name, prepare, processBlock (+1 more)

### Community 104 - "MixerTests.cpp"
Cohesion: 0.17
Nodes (11): AudioBufferPool, AudioBufferView, ProcessContext, Sample, size_t, vector, channel, onsets() (+3 more)

### Community 105 - "WavStreamWriterTests.cpp"
Cohesion: 0.18
Nodes (10): FrameCount, path, size_t, uint8_t, vector, fileBytes(), makeTestSignal(), ScratchFile (+2 more)

### Community 106 - "DiskStreamer"
Cohesion: 0.24
Nodes (10): atomic, shared_ptr, DiskStreamer, add, mutex_, running_, streams_, thread_ (+2 more)

### Community 107 - "renderNode"
Cohesion: 0.25
Nodes (10): AudioFileData, vector, FrameCount, Node, Sample, size_t, vector, makeAudio() (+2 more)

### Community 108 - "MidiTests.cpp"
Cohesion: 0.20
Nodes (10): MidiInput, FrameCount, MidiMessage, SampleRate, Tick, uint64_t, nanosForFrame(), patternNote() (+2 more)

### Community 109 - "AddMixerNodeCommand"
Cohesion: 0.20
Nodes (8): AddMixerNodeCommand, execute, index_, minted_, node_, type_, undo, MixerNodeType

### Community 110 - "AddNoteCommand"
Cohesion: 0.20
Nodes (8): AddNoteCommand, channel_, execute, index_, note_, pattern_, Command, size_t

### Community 111 - "captureAudioBlock"
Cohesion: 0.20
Nodes (9): captureAudioBlock, start, stop, FrameCount, path, Result, size_t, Take (+1 more)

### Community 112 - "AudioFileData"
Cohesion: 0.18
Nodes (10): AudioFileData, channelCount, channels, frameCount, sampleRate, FrameCount, Sample, SampleRate (+2 more)

### Community 113 - "TimelineAnchor"
Cohesion: 0.22
Nodes (7): FramePosition, uint64_t, TimelineAnchor, hostTimeNanos, playing, sampleRate, timelineFrame

### Community 114 - "MixerStripNode.cpp"
Cohesion: 0.31
Nodes (10): FrameCount, Sample, SampleRate, panGains, prepare, refreshTargets, setGain, setMuted (+2 more)

### Community 115 - "TimeSignatureEvent"
Cohesion: 0.22
Nodes (8): friend, Tick, TempoEvent, beatsPerMinute, tick, TimeSignatureEvent, signature, tick

### Community 116 - "MidiDevice"
Cohesion: 0.18
Nodes (10): unique_ptr, MidiDevice::create(), MidiDevice, close, create, enumerateInputs, enumerateOutputs, isOpen (+2 more)

### Community 117 - "main.mm"
Cohesion: 0.29
Nodes (10): -editorChanged, -seekToTick, -showEditorAtSegment, -showMixer, -showPianoRoll, -showPlaylist, -togglePlayback, -transportModeChanged (+2 more)

### Community 118 - "MixerFixture"
Cohesion: 0.18
Nodes (8): buildParallelPaths(), EntityId, TempoMap, unique_ptr, MixerFixture, pattern, project, tempo

### Community 119 - "DuplicateClipsCommand"
Cohesion: 0.22
Nodes (8): DuplicateClipsCommand, clips_, created_, createdIds_, minted_, tickDelta_, trackDelta_, Tick

### Community 120 - "DisconnectMixerCommand"
Cohesion: 0.20
Nodes (7): DisconnectMixerCommand, connection_, connectionId_, execute, index_, undo, size_t

### Community 121 - "DeleteNotesCommand"
Cohesion: 0.20
Nodes (9): string, DeleteNotesCommand, channel_, indices_, name, pattern_, removed_, undo (+1 more)

### Community 122 - "readAt"
Cohesion: 0.22
Nodes (8): FrameCount, path, Result, Sample, size_t, close, open, readAt

### Community 123 - "RecordedEvent"
Cohesion: 0.20
Nodes (10): Kind, Tick, RecordedEvent, channel, duration, key, kind, releaseValue (+2 more)

### Community 124 - "RoutingConnection"
Cohesion: 0.20
Nodes (9): findRouting, RoutingConnection, destination, gain, id, isSend, preFader, sidechain (+1 more)

### Community 125 - "make-dmg.sh"
Cohesion: 0.22
Nodes (9): APP_NAME, APP_PATH, BUILD_DIR, DIST_DIR, DMG_PATH, log(), ROOT, make-dmg.sh script (+1 more)

### Community 126 - "ProjectGraphCompiler.cpp"
Cohesion: 0.31
Nodes (8): AutomationNode, channelStripFor, instrumentFor, stripFor, EntityId, InstrumentNode, MixerStripNode, MixerStripNode

### Community 127 - "MoveClipsCommand"
Cohesion: 0.22
Nodes (6): MoveClipsCommand, appliedTickDelta_, appliedTrackDelta_, clips_, tickDelta_, trackDelta_

### Community 128 - "SetChannelOutputCommand"
Cohesion: 0.22
Nodes (6): SetChannelOutputCommand, channelId_, execute, mixerNode_, previous_, undo

### Community 129 - "SetMixerPanCommand"
Cohesion: 0.22
Nodes (6): SetMixerPanCommand, execute, nodeId_, pan_, previous_, undo

### Community 130 - "SetMixerVolumeCommand"
Cohesion: 0.22
Nodes (6): SetMixerVolumeCommand, execute, nodeId_, previous_, undo, volume_

### Community 131 - "MoveNotesCommand"
Cohesion: 0.22
Nodes (8): MoveNotesCommand, appliedKeyDelta_, appliedTickDelta_, channel_, indices_, keyDelta_, pattern_, tickDelta_

### Community 132 - "SetPatternLengthCommand"
Cohesion: 0.25
Nodes (7): Tick, SetPatternLengthCommand, execute, length_, patternId_, previousLength_, undo

### Community 133 - "MidiDevice.h"
Cohesion: 0.22
Nodes (7): string, MidiDeviceInfo, identifier, isInput, name, MidiInputCallback, midiMessageReceived

### Community 134 - "AutomationFixture"
Cohesion: 0.22
Nodes (7): AutomationFixture, channel, pattern, project, tempo, EntityId, TempoMap

### Community 135 - "Denormals.h"
Cohesion: 0.39
Nodes (5): ControlRegister, readControlRegister(), ScopedNoDenormals, previous_, writeControlRegister()

### Community 136 - "ResizeClipsCommand"
Cohesion: 0.25
Nodes (6): ResizeClipsCommand, clips_, execute, lengthDelta_, previousLengths_, undo

### Community 137 - "Command"
Cohesion: 0.25
Nodes (6): Command, SetMixerMutedCommand, execute, muted_, nodeId_, undo

### Community 138 - "MixerCommands.h"
Cohesion: 0.25
Nodes (5): SetMixerPolarityCommand, execute, inverted_, nodeId_, undo

### Community 139 - "SetMixerSoloedCommand"
Cohesion: 0.25
Nodes (5): SetMixerSoloedCommand, execute, nodeId_, soloed_, undo

### Community 140 - "ResizeNotesCommand"
Cohesion: 0.25
Nodes (7): ResizeNotesCommand, channel_, durationDelta_, indices_, pattern_, previousDurations_, undo

### Community 142 - "RemovePatternCommand"
Cohesion: 0.25
Nodes (7): size_t, RemovePatternCommand, execute, index_, pattern_, patternId_, undo

### Community 143 - "Version"
Cohesion: 0.29
Nodes (6): Version, major, minor, patch, phase, string

### Community 144 - "TimestampedMidiMessage"
Cohesion: 0.25
Nodes (8): sendMessage, uint64_t, uint8_t, TimestampedMidiMessage, data1, data2, hostTimeNanos, status

### Community 145 - "makeTestSignal"
Cohesion: 0.25
Nodes (6): FrameCount, path, size_t, makeTestSignal(), ScratchFile, path

### Community 146 - "compileProjectGraph"
Cohesion: 0.29
Nodes (7): Channel, compileProjectGraph(), InstrumentFactory, Project, defaultInstrumentFactory(), isAudible(), TempoMap

### Community 147 - "PatternTests.cpp"
Cohesion: 0.48
Nodes (6): SequencedNote, Tick, vector, note(), shapeOf(), startsOf()

### Community 148 - "ScratchDirectory"
Cohesion: 0.29
Nodes (4): path, string, ScratchDirectory, path

### Community 149 - "INCDAWMixerView"
Cohesion: 0.33
Nodes (5): incdaw, NSView, INCDAWMixerView, -initWithFrameprojectregistry, stripLookup

### Community 150 - "ProjectGraphCompiler.h"
Cohesion: 0.33
Nodes (4): Instrument, ParameterRegistry, InstrumentNode, string

### Community 151 - "INCDAWPianoRollView"
Cohesion: 0.33
Nodes (5): NSString, NSView, INCDAWPianoRollView, -initWithFrameprojectregistry, -requestRedraw

### Community 152 - "InstrumentTests.cpp"
Cohesion: 0.40
Nodes (5): SimpleSynth, AudioBufferPool, MidiBuffer, Sample, renderSynth()

### Community 154 - "noteAtStep"
Cohesion: 0.40
Nodes (5): size_t, Tick, vector, noteAtStep(), execute

### Community 155 - "v1.0/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 156 - "v1.1/Fixture.incdaw/manifest.json"
Cohesion: 0.33
Nodes (5): created, created_with, incdaw_project_version, last_saved_with, modified

### Community 157 - "ChannelRackTests.cpp"
Cohesion: 0.33
Nodes (5): EntityId, Step, Tick, note(), stepAt()

### Community 158 - "OrderRecordingNode"
Cohesion: 0.40
Nodes (4): vector, OrderRecordingNode, identifier_, log_

### Community 159 - "MidiRecorder.cpp"
Cohesion: 0.40
Nodes (4): FramePosition, MidiBuffer, capture, reset

### Community 160 - "check"
Cohesion: 0.80
Nodes (4): check(), layer_of(), main(), Path

### Community 163 - "INCDAWChannelRackView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWChannelRackView, -initWithFrameprojectregistry

### Community 164 - "INCDAWPatternListView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWPatternListView, -initWithFrameprojectregistry

### Community 165 - "INCDAWPlaylistView"
Cohesion: 0.50
Nodes (3): NSView, INCDAWPlaylistView, -initWithFrameprojectregistry

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
- **758 isolated node(s):** `channelId_`, `previousVolume_`, `volume_`, `channelId_`, `muted_` (+753 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **34 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Project` connect `Project` to `SetChannelOutputCommand`, `string`, `SetMixerPanCommand`, `SetMixerVolumeCommand`, `SetPatternLengthCommand`, `RemoveTrackCommand`, `PlaylistModel`, `AutomationFixture`, `ResizeClipsCommand`, `Command`, `MixerCommands.h`, `SetMixerSoloedCommand`, `CommandRegistry`, `ResizeNotesCommand`, `RemovePatternCommand`, `MixerNode`, `Pattern`, `Clip`, `Model.cpp`, `ProjectFile.cpp`, `noteAtStep`, `NoteCommands.cpp`, `InsertRecordedTakeCommand`, `compileArrangement`, `CountingCommand`, `RemoveClipsCommand`, `MixerCommands.cpp`, `ClipCommands.cpp`, `SetVelocityCommand`, `AddPatternClipCommand`, `string`, `EntityId`, `QuantizeNotesCommand`, `AddPatternCommand`, `PatternCommands.cpp`, `Fixture`, `Channel`, `RemoveMixerNodeCommand`, `AddMixerNodeCommand`, `AddNoteCommand`, `MixerFixture`, `DisconnectMixerCommand`, `DeleteNotesCommand`, `RoutingConnection`?**
  _High betweenness centrality (0.246) - this node is a cross-community bridge._