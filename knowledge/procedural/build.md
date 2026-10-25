---
generated_by: "OpenAI Codex (GPT-6; exact variant not exposed)"
timestamp: "2026-10-24T22:47:00-05:00"
---

# Native build and emulator check

Run `make test` before `make build`. The build wrapper calls `pebble clean`, builds the Emery app, and copies the PBW to `dist/pickleballkeeper.pbw`.

The emulator is shared with other Pebble projects. Check that no other installation is active, then run `pebble install --emulator emery` from `watchapp/`. Use `pebble screenshot --no-open --no-correction` with a path under the project `.tmp/` directory. If the install succeeds while the visible app remains stale, follow `_PEBBLE` skill guidance and install again after the emulator boots.

Host tests compile `model.c` without Pebble headers. `python3 scripts/health_check.py --strict` checks the native project shape and runs `make test`.
