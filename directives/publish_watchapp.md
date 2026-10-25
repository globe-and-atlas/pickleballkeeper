---
generated_by: "OpenAI Codex (GPT-6; exact variant not exposed)"
timestamp: "2026-10-25T07:30:00-05:00"
---

# Publish PickleballKeeper

## Goal

Publish the verified Emery PBW as a public PickleballKeeper watchapp on RePebble.

## Inputs

- `dist/pickleballkeeper.pbw`
- `docs/store/listing.md`
- The two icons in `docs/store/`
- The five ordered Emery screenshots in `docs/screenshots/`

## Procedure

1. Run `make build` and inspect the bundled `appinfo.json` for the expected name, UUID, version, watchapp type, and Emery target.
2. Verify each screenshot is a native 200×228 capture from the release app.
3. Run an independent read-only release check against `task.md`.
4. Copy the five screenshots to `.tmp/store-submission/` with names beginning `emery_`.
5. Run `pebble publish --non-interactive --is-published --no-gif-all-platforms` from `watchapp/`, using the prepared description, release notes, category, icons, and screenshots.
6. Open the returned public app page and verify the version, screenshots, and PBW download.

The CLI may require `pebble login` when the saved account session expires. Never print or commit the login token. The store upload is public and is authorized by the user's 2026-10-25 request.
