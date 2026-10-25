# Task: PickleballKeeper

## Objective

Build an independent Pebble Time 2 pickleball scorer that records rally results, tracks the correct serve, and saves completed matches locally.

## Contract

- Target directives: `directives/build_watchapp.md`.
- Intended execution scripts: `execution/build.py`.
- Output artifact: `dist/pickleballkeeper.pbw`.
- Safety: keep generated files in `.tmp/` or ignored build directories.
- Safety: use a new Pebble UUID and independent persistence keys.
- Safety: do not publish the app without a separate request.

## Validation Contract

- Side-out doubles starts with server 2.
- A receiving team's rally win does not add a side-out point.
- The first doubles service fault transfers service to the opponents.
- A side-out starts from the right court when the receiving team's score is odd.
- A server-1 fault moves service to the opposite court for server 2.
- A later doubles server-1 fault advances to server 2.
- A doubles server-2 fault transfers service to the opponents.
- A singles service fault transfers service to the opponent.
- Rally scoring awards a point to each rally winner.
- Collegiate freeze suppresses a receiving team's game-winning point.
- USA 2026 rally scoring permits a receiving team's game-winning point.
- A game ends only at its target with a two-point margin.
- A best-of-three match ends at two game wins.
- Undo restores the complete state of the preceding rally.
- Finished match records contain the final game scores and date.
- A completed game label reflects that game's winner.
- Calendar selection opens a record from the displayed month.
- A GPS callback attaches to the saved record after a new match starts.
- Watch buttons remain functional when touch scoring is disabled.
- `make test` passes.
- `make build` produces the PBW.
- A fresh verifier checks the final project against this contract.

## Progress

- [x] Review USA Pickleball, PPA, MLP, and Collegiate Pickleball Tour scoring formats.
- [x] Create independent project from `project-template`.
- [x] Implement native scorer and formats.
- [x] Implement saved match history and settings.
- [x] Run host tests and build.
- [x] Inspect native emulator screens.
- [x] Run fresh verification.

## Final checks

- `make build` passed after the independent scoring corrections.
- `python3 scripts/health_check.py --strict` passed.
- The Emery emulator showed GAME LOST after game 3 while US led 2–1 in a best-of-five match.
- The PBW appinfo identifies PickleballKeeper with its own UUID.

## 2026-10-25 RePebble release

### Validation Contract

- The release PBW identifies PickleballKeeper version 0.1.0.
- The release PBW retains UUID `6ba1de5c-79e4-4937-a6b3-40e74b9a2d68`.
- The release PBW targets Emery.
- `make build` passes.
- The store package contains five native 200×228 Emery screenshots.
- The store description names the supported scoring modes accurately.
- The store description identifies the school preset as one game.
- The store package includes 80×80 and 144×144 icons.
- A fresh verifier reviews the PBW and store package before submission.
- The published listing has a public appstore URL.
- The published listing displays version 0.1.0.
- The published listing provides a PBW download.

### Checklist

- [x] Prepare listing text, icons, and screenshots.
- [x] Rebuild and inspect the PBW.
- [x] Run independent release verification.
- [x] Publish to RePebble.
- [x] Verify the public listing.

### Result

- 2026-10-25: Published PickleballKeeper 0.1.0 at https://apps.repebble.com/b0559475cc924cc3805140c5.
- The unauthenticated public page returns HTTP 200 and shows five screenshots.
- The page shows a version 0.1.0 PBW download; an HTTP GET returned 200.
- Physical Time 2 controls and connected-phone GPS remain unverified.

## Log

### 2026-10-24 22:40 - Project and rules
- Changed: scaffolded `pickleballkeeper` from `workflow-python` profile.
- Finding: MLP 2026 doubles use side-out to 11; CPT 2025-26 uses rally to 21 with a game-point freeze; USAP 2026 rally has no game-point freeze.
- Next: implement separate scoring modes rather than school/pro labels.

### 2026-10-24 22:47 - Independent review
- Changed: explicit doubles service court, game-result label, month-specific calendar selection, match-ID GPS response, native-runtime health check.
- Test: `make build` passed after scoring fixes; independent verifier re-reviewed them.
- Next: run strict health check and inspect final diff.
