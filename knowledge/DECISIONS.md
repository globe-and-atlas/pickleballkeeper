# Decisions

## Initial Decision

- Template profile: `workflow-python`
- Deploy target: `local-only`
- Template runtime at scaffold: `python`

## 2026-10-24 - Native Pebble scorer

- The template profile remains `workflow-python` as scaffold provenance; `project.profile.json` now declares `pebble-c` as the actual runtime.
- The project health checker detects the native runtime and validates the PBW sources and host tests. Irrelevant Python and prompt-loop template files were removed.
- Build an independent PBW with a new UUID, storage namespace, and package name.
- Interpret Up/Down as the rally winner. The model determines whether a point is awarded and who serves next.
- Expose side-out, USA 2026 rally, and win-on-serve freeze as separate scoring modes.
- Use plain rule-based presets plus custom target and best-of settings. Institution labels cannot define every event's scoring.
- Save individual completed matches on the watch; request optional phone GPS only after a save.
