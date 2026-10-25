#include "model.h"

#include <string.h>

Rules rules_preset(Preset preset) {
  switch (preset) {
    case PRESET_CLASSIC_1_15: return (Rules){SIDE_OUT, 1, 15, 1};
    case PRESET_PPA_5: return (Rules){SIDE_OUT, 1, 11, 5};
    case PRESET_USAP_RALLY: return (Rules){RALLY_2026, 1, 11, 3};
    case PRESET_COLLEGE_GAME: return (Rules){RALLY_FREEZE, 1, 21, 1};
    default: return (Rules){SIDE_OUT, 1, 11, 3};
  }
}

bool rules_valid(Rules rules) {
  return rules.scoring <= RALLY_FREEZE && rules.doubles <= 1 &&
         (rules.target == 11 || rules.target == 15 || rules.target == 21) &&
         (rules.best_of == 1 || rules.best_of == 3 || rules.best_of == 5);
}

static void begin_game(GameState *s, Rules rules, uint8_t first_serve) {
  s->points[0] = s->points[1] = 0;
  s->serving = first_serve;
  s->server_number = rules.scoring == SIDE_OUT && rules.doubles ? 2 : 0;
  s->serve_court = 0;
  s->phase = PLAYING;
}

void match_start(Match *match, Rules rules, uint8_t first_serve) {
  memset(match, 0, sizeof(*match));
  match->version = 2;
  match->rules = rules_valid(rules) ? rules : rules_preset(PRESET_CLASSIC_3);
  begin_game(&match->state, match->rules, first_serve <= 1 ? first_serve : 0);
}

bool match_rally(Match *match, uint8_t winner) {
  if (!match || winner > 1 || match->state.phase != PLAYING ||
      !rules_valid(match->rules)) return false;
  GameState *s = &match->state;
  match->undo = *s;
  match->undo_available = 1;
  uint8_t old_server = s->serving;
  if (match->rules.scoring == SIDE_OUT) {
    if (winner == old_server) {
      s->points[winner]++;
      if (match->rules.doubles) s->serve_court ^= 1;
    } else if (match->rules.doubles && s->server_number == 1) {
      s->server_number = 2;
      s->serve_court ^= 1;
    } else {
      s->serving = winner;
      s->server_number = match->rules.doubles ? 1 : 0;
      s->serve_court = 0;
    }
  } else {
    bool frozen = match->rules.scoring == RALLY_FREEZE &&
                  winner != old_server &&
                  s->points[winner] >= match->rules.target - 1 &&
                  s->points[winner] >= s->points[1 - winner] + 1;
    if (!frozen) s->points[winner]++;
    if (winner != old_server) s->serving = winner;
    s->serve_court = s->points[s->serving] % 2;
  }
  uint8_t loser = 1 - winner;
  if (s->points[winner] >= match->rules.target &&
      s->points[winner] >= s->points[loser] + 2) {
    s->game_scores[s->game_index][0] = s->points[0];
    s->game_scores[s->game_index][1] = s->points[1];
    s->games[winner]++;
    s->phase = s->games[winner] >= (match->rules.best_of / 2 + 1) ?
               MATCH_OVER : GAME_OVER;
  }
  return true;
}

bool match_undo(Match *match) {
  if (!match || !match->undo_available || match->saved) return false;
  match->state = match->undo;
  match->undo_available = 0;
  return true;
}

bool match_next_game(Match *match, uint8_t first_serve) {
  if (!match || match->state.phase != GAME_OVER || first_serve > 1 ||
      match->state.game_index + 1 >= match->rules.best_of) return false;
  match->state.game_index++;
  begin_game(&match->state, match->rules, first_serve);
  match->undo_available = 0;
  return true;
}

bool match_record(const Match *match, uint16_t year, uint8_t month,
                  uint8_t day, MatchRecord *out) {
  if (!match || !out || match->state.phase != MATCH_OVER ||
      year < 2000 || month < 1 || month > 12 || day < 1 || day > 31) return false;
  memset(out, 0, sizeof(*out));
  out->id = match->id;
  out->year = year;
  out->month = month;
  out->day = day;
  out->winner = match->state.games[0] > match->state.games[1] ? 0 : 1;
  out->rules = match->rules;
  memcpy(out->games, match->state.games, sizeof(out->games));
  memcpy(out->game_scores, match->state.game_scores, sizeof(out->game_scores));
  return true;
}

uint8_t match_serve_court(const Match *match) {
  if (!match) return 0;
  if (!match->rules.doubles && match->rules.scoring == SIDE_OUT)
    return (uint8_t)(match->state.points[match->state.serving] % 2);
  return match->state.serve_court;
}
