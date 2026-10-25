---
generated_by: "OpenAI Codex (GPT-6; exact variant not exposed)"
timestamp: "2026-10-24T22:40:00-05:00"
---

# PickleballKeeper

A Pebble Time 2 watch app for scoring pickleball from the wrist. Press **Up** when US wins a rally and **Down** when THEM wins; the app awards points according to the chosen rules and shows whose serve is next.

[Install from the RePebble appstore](https://apps.repebble.com/b0559475cc924cc3805140c5) · Published version 0.1.0

## Formats

| Watch preset | Rules |
|---|---|
| Classic 11 best of 3 | Side-out doubles, first to two games, each to 11, win by 2 |
| Classic 15 one game | Side-out doubles, one game to 15, win by 2 |
| Pro final 11 / 5 | Side-out doubles, first to three games, each to 11, win by 2 |
| USAP rally 11 / 3 | 2026 optional rally scoring, first to two games to 11; a receiver can win game point |
| School rally 21 | One rally-scored game to 21 with win-on-serve freeze at game point, matching the 2025–26 CPT game format |
| Custom rules | Select 11/15/21, best of 1/3/5, side-out/rally/freeze, and singles/doubles |

The watch tracks the opening 0–0–2 exception and both doubles servers in side-out mode. After each side-out, server 1 begins on the right, regardless of the team's score; the service court alternates after points and on the transition to server 2. For rally scoring, there is one server per side out and the service court follows the serving team's score parity.

The school preset scores **one game**, not an entire interschool team event. College formats can include four doubles games and a DreamBreaker; high-school events vary by organizer. Check the specific event rules before selecting a preset.

## Controls

- **Up / Down:** rally winner (US / THEM), always available.
- **Select:** menu, next game, or save a completed match.
- **Long Select:** undo the last rally.
- **Back:** return to the score screen.
- **Touch:** tap either score panel to record a rally; Settings can disable touch without changing button controls.

The menu offers formats, color swatches, calendar/history, a relative location plot, touch, optional phone GPS on match save, and a new match. A new match asks who serves first. The next game asks again because service choice can vary by match procedure. A completed match is saved on **Select**, with dated game scores and optional coordinates. History retains the latest 30 saved matches. The location plot shows relative positions of saved GPS results; it is not a street map. GPS requires the phone connection and permission; scoring works entirely offline.

## Build

```bash
make test
make build
pebble install --emulator emery
```

The independent package is `dist/pickleballkeeper.pbw`. Its Pebble UUID and watch storage are separate from VolleyKeeper. The appstore listing is live; physical Time 2 and real-phone GPS testing remain open.

## Rule references

- [USA Pickleball 2026 rulebook landing page](https://usapickleball.org/rules/)
- [USA Pickleball side-out scoring](https://usapickleball.org/pickleball-skills/level-one/how-to-play-pickleball/)
- [2026 approved rally game-point change](https://rules.usapickleball.org/view/rule-changes-view-all/entry/2378/?pagenum=1)
- [PPA pro format](https://www.ppatour.com/about/what-is-pickleball/)
- [MLP 2026 hybrid scoring](https://majorleaguepickleball.co/abcs-of-mlp/)
- [DUPR Collegiate Pickleball Tour 2025–26 rules](https://www.dupr.com/rules/collegiate-pickleball-rules)
- [DUPR High School 2025–26 rules](https://www.dupr.com/rules/high-school-pickleball-rules)
