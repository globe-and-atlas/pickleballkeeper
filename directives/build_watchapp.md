---
generated_by: "OpenAI Codex (GPT-6; exact variant not exposed)"
timestamp: "2026-10-24T22:40:00-05:00"
---

# Build PickleballKeeper

## Goal

Produce a clean Emery PBW from the native C scorer and phone-side GPS bridge.

## Inputs

- `watchapp/src/c/model.c`: scoring state and match records.
- `watchapp/src/c/main.c`: watch UI and persistence.
- `watchapp/src/pkjs/index.js`: optional phone GPS request after save.

## Procedure

1. Run `make test` from the project root.
2. Run `make build` from the project root.
3. Confirm `dist/pickleballkeeper.pbw` exists.
4. Check no other Pebble emulator install is underway before using `pebble install --emulator emery`.
5. Inspect a native-resolution screenshot with `pebble screenshot --no-open --no-correction`.

`execution/build.py` cleans the Pebble output before packaging, so fixture or stale output cannot be copied to `dist/`.

## Boundaries

- Do not reuse VolleyKeeper's UUID or persistence schema.
- Do not treat a PBW build as physical-watch validation.
- Do not publish a store listing unless requested.
- Keep screenshots and temporary scripts in `.tmp/`.
