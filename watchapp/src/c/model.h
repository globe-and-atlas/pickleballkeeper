#ifndef PICKLEBALL_MODEL_H
#define PICKLEBALL_MODEL_H

#include <stdbool.h>
#include <stdint.h>

enum { TEAM_US = 0, TEAM_THEM = 1, MAX_GAMES = 5 };
typedef enum { SIDE_OUT = 0, RALLY_2026 = 1, RALLY_FREEZE = 2 } Scoring;
typedef enum { PLAYING = 0, GAME_OVER = 1, MATCH_OVER = 2 } Phase;
typedef enum {
  PRESET_CLASSIC_3 = 0,
  PRESET_CLASSIC_1_15,
  PRESET_PPA_5,
  PRESET_USAP_RALLY,
  PRESET_COLLEGE_GAME,
  PRESET_COUNT
} Preset;

typedef struct {
  uint8_t scoring;
  uint8_t doubles;
  uint8_t target;
  uint8_t best_of;
} Rules;

typedef struct {
  uint16_t points[2];
  uint16_t game_scores[MAX_GAMES][2];
  uint8_t games[2];
  uint8_t game_index;
  uint8_t serving;
  uint8_t server_number;
  uint8_t serve_court;
  uint8_t phase;
} GameState;

typedef struct {
  uint8_t version;
  uint32_t id;
  Rules rules;
  GameState state;
  GameState undo;
  uint8_t undo_available;
  uint8_t saved;
} Match;

typedef struct {
  uint32_t id;
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t winner;
  Rules rules;
  uint8_t games[2];
  uint16_t game_scores[MAX_GAMES][2];
  int32_t latitude_e6;
  int32_t longitude_e6;
  uint16_t accuracy_m;
  uint8_t has_location;
} MatchRecord;

Rules rules_preset(Preset preset);
bool rules_valid(Rules rules);
void match_start(Match *match, Rules rules, uint8_t first_serve);
bool match_rally(Match *match, uint8_t winner);
bool match_undo(Match *match);
bool match_next_game(Match *match, uint8_t first_serve);
bool match_record(const Match *match, uint16_t year, uint8_t month,
                  uint8_t day, MatchRecord *out);
uint8_t match_serve_court(const Match *match);

#endif
