---
generated_by: "OpenAI Codex (GPT-6; exact variant not exposed)"
timestamp: "2026-10-25T07:33:52-05:00"
---

# RePebble publication

For a first PickleballKeeper release, `execution/publish.py --dry-run` validates the PBW identity, listing, icon sizes, and five Emery screenshots. `execution/publish.py --publish` passes them to `pebble publish --non-interactive --is-published --no-gif-all-platforms` using the signed-in Pebble developer account.

Screenshots are copied to `.tmp/store-submission/` with `emery_` prefixes; the store CLI requires those prefixes. The appstore allows up to five screenshots per platform. The script uses `docs/store/listing.md` for the description and release notes.

The first release was submitted on 2026-10-25 as PickleballKeeper 0.1.0 at https://apps.repebble.com/b0559475cc924cc3805140c5. The unauthenticated page returned HTTP 200; it showed five screenshots, Health & Fitness, Time 2, version 0.1.0, and a PBW download. A GET of the PBW URL returned HTTP 200. A HEAD request to that URL redirected to storage and returned 403, so use GET when verifying the download.

For updates, increment `watchapp/package.json` first. The current publish script pins 0.1.0 and must be updated for a new version before upload. Check the latest CLI `--replace-screenshots` behavior before replacing the existing gallery; replacement changes live store assets.
