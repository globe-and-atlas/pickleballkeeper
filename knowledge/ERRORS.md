# Errors

Record deterministic errors, root causes, and fixes here.

## 2026-10-24 - Side-out doubles court shown incorrectly

- **Error:** The first server after a side-out could be shown on the left when that team had an odd score. The second server could also be shown on the wrong side.
- **Cause:** The UI derived serving court from total score parity, which is valid for singles and rally scoring but not for traditional doubles service turns.
- **Fix:** Track serving court in `GameState`: start each side-out on the right, toggle after a serving point, toggle on server-1 fault, reset on side-out. Host tests include an odd-score side-out.
- **Graduated-to:** `knowledge/domain/scoring.md`.
