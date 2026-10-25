---
generated_by: "OpenAI Codex (GPT-6; exact variant not exposed)"
timestamp: "2026-10-24T22:40:00-05:00"
---

# Project Context

## Identity

- Name: PickleballKeeper
- Audience: pickleball players and spectators who want wrist scoring without remembering server transitions.
- Profile: template `workflow-python`; actual runtime native Pebble C plus PebbleKit JS.
- Deploy: local PBW for Emery (Pebble Time 2).

## Purpose

Record who won each rally, show the correct score and serve, and keep a dated calendar of results. The user chooses an explicit scoring ruleset rather than relying on an ambiguous school or pro label.

## Constraints

- Physical buttons always work; touch can be disabled.
- Side-out doubles begins at 0–0–2.
- USA 2026 optional rally and school win-on-serve freeze are distinct modes.
- Match history is watch-local and limited to the latest 30 saves.
- GPS is optional and supplied by the connected phone after save.
- A clean PBW build does not prove hardware behavior.

## Out of Scope

- Entire interschool team events with multiple distinct doubles pairings and DreamBreaker rotation.
- Tournament submission, DUPR synchronization, rankings, or a hosted web account.
- Public watchapp store publication in this initial request.
