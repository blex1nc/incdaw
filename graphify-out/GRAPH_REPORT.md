# Graph Report - INCDAW X  (2026-08-14)

## Corpus Check
- Corpus is ~5,497 words - fits in a single context window. You may not need a graph.

## Summary
- 84 nodes · 199 edges · 8 communities
- Extraction: 67% EXTRACTED · 30% INFERRED · 3% AMBIGUOUS · INFERRED: 59 edges (avg confidence: 0.9)
- Token cost: 11,500 input · 12,000 output

## Community Hubs (Navigation)
- Approval Governance and Discovery
- MIDI Sequencing and Pattern Editing
- Plugin Host Pipeline
- Undecided Architecture Choices
- Project Data Model and Rendering
- Realtime Audio Foundation
- Transport and Signal Flow
- FL Studio Reference and IP Boundary

## God Nodes (most connected - your core abstractions)
1. `INCDAW` - 33 edges
2. `Proposed Architectural Layers` - 18 edges
3. `Open Decisions` - 12 edges
4. `Mixer` - 11 edges
5. `Audio Engine` - 10 edges
6. `Plugin Host` - 9 edges
7. `Automation Subsystem` - 8 edges
8. `MIDI Engine` - 7 edges
9. `Pattern System` - 7 edges
10. `Playlist / Arrangement` - 7 edges

## Surprising Connections (you probably didn't know these)
- `INCDAW` --semantically_similar_to--> `INCDAW Project Mission`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Proposed Repository Structure` --semantically_similar_to--> `Proposed Architectural Layers`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Absolute User Control Rule` --semantically_similar_to--> `Critical Operating Rule`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `Graphify Mandate` --semantically_similar_to--> `Graphify Workflow`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md
- `FL Studio Feature Parity Objective` --semantically_similar_to--> `FL Studio 2026 Reference`  [INFERRED] [semantically similar]
  CLAUDE.md → HANDOFF.md

## Hyperedges (group relationships)
- **Plugin Host Pipeline** — handoff_plugin_scanner, handoff_plugin_registry, handoff_plugin_instance, handoff_parameter_system, handoff_plugin_state_system, handoff_plugin_ui_bridge, handoff_crash_isolation_strategy [EXTRACTED 1.00]
- **Approval-Gated Development Protocol** — claude_absolute_user_control_rule, claude_graphify_mandate, claude_feature_workflow, claude_scope_control, claude_dependency_policy, handoff_critical_operating_rule, handoff_feature_protocol, handoff_handoff_rule [EXTRACTED 1.00]
- **Master Signal Chain Convergence** — handoff_midi_signal_flow, handoff_audio_signal_flow, handoff_plugin_automation_flow, handoff_shared_transport_state, claude_mixer, claude_automation, claude_offline_render_engine, claude_core_transport [INFERRED 0.85]

## Communities (8 total, 0 thin omitted)

### Community 0 - "Approval Governance and Discovery"
Cohesion: 0.14
Nodes (21): Absolute User Control Rule, Decision Log, Definition of Done, Dependency Policy, Development Phases (0-20), Required Documentation Set, Required Feature Workflow (Steps A-E), First Task: Discovery and Architecture Planning (+13 more)

### Community 1 - "MIDI Sequencing and Pattern Editing"
Cohesion: 0.27
Nodes (11): Browser, Content / Sound Library, MIDI Controllers, MIDI Engine, MPE Readiness, Pattern System, Performance Strategy, Piano Roll (+3 more)

### Community 2 - "Plugin Host Pipeline"
Cohesion: 0.22
Nodes (11): Controller Linking, Built-in Effect Framework, Plugin Host, Crash / Isolation Strategy, Parameter System, Plugin Host Priority, Plugin Instance, Plugin Registry (+3 more)

### Community 3 - "Undecided Architecture Choices"
Cohesion: 0.40
Nodes (10): Audio Editor, Command Registry / Keyboard Shortcuts, INCDAW, Built-in Instrument Framework, Platform Strategy, Proposed Repository Structure, Sampler, Time Stretching / Pitch Architecture (+2 more)

### Community 4 - "Project Data Model and Rendering"
Cohesion: 0.33
Nodes (10): Channel / Instrument System, Clip / Project Data Model, File Formats, Mixer, Offline Render Engine, Versioned Project Format, Project System, Core Data Model (+2 more)

### Community 5 - "Realtime Audio Foundation"
Cohesion: 0.36
Nodes (9): Audio Engine, Audio Correctness Requirements, Engineering Rules, Realtime Audio Thread Safety, Testing Strategy, UI Architecture, Audio Engine Priority, Testing Strategy (+1 more)

### Community 6 - "Transport and Signal Flow"
Cohesion: 0.36
Nodes (8): Automation Subsystem, Core Transport, Playlist / Arrangement, Recording, Audio Recording Signal Flow, Definition of Success, Plugin Automation Flow, Shared Transport / Timing / Undo Foundation

### Community 7 - "FL Studio Reference and IP Boundary"
Cohesion: 0.50
Nodes (4): FL Studio Feature Parity Objective, Legal / IP Boundary, FL Studio 2026 Reference, INCDAW Project Mission

## Ambiguous Edges - Review These
- `Piano Roll` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Step Sequencer` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Pattern System` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Time Stretching / Pitch Architecture` → `Open Decisions`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Content / Sound Library` → `Proposed Architectural Layers`  [AMBIGUOUS]
  CLAUDE.md · relation: conceptually_related_to
- `Undo / Redo` → `Clip / Project Data Model`  [AMBIGUOUS]
  CLAUDE.md · relation: shares_data_with

## Knowledge Gaps
- **1 isolated node(s):** `Plugin State System`
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `Piano Roll` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Step Sequencer` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Pattern System` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Time Stretching / Pitch Architecture` and `Open Decisions`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Content / Sound Library` and `Proposed Architectural Layers`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Undo / Redo` and `Clip / Project Data Model`?**
  _Edge tagged AMBIGUOUS (relation: shares_data_with) - confidence is low._
- **Why does `INCDAW` connect `Undecided Architecture Choices` to `Approval Governance and Discovery`, `MIDI Sequencing and Pattern Editing`, `Plugin Host Pipeline`, `Project Data Model and Rendering`, `Realtime Audio Foundation`, `Transport and Signal Flow`, `FL Studio Reference and IP Boundary`?**
  _High betweenness centrality (0.421) - this node is a cross-community bridge._