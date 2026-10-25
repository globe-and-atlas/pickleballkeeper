#include <assert.h>
#include <stdio.h>

#include "model.h"

static void test_opening_doubles(void) {
  Match m;
  match_start(&m, rules_preset(PRESET_CLASSIC_3), TEAM_US);
  assert(m.state.serving == TEAM_US);
  assert(m.state.server_number == 2);
  assert(match_serve_court(&m) == 0);
  assert(match_rally(&m, TEAM_THEM));
  assert(m.state.points[TEAM_THEM] == 0);
  assert(m.state.serving == TEAM_THEM);
  assert(m.state.server_number == 1);
  assert(match_rally(&m, TEAM_THEM));
  assert(m.state.points[TEAM_THEM] == 1);
  assert(match_serve_court(&m) == 1);
  assert(match_rally(&m, TEAM_US));
  assert(m.state.server_number == 2);
  assert(m.state.serving == TEAM_THEM);
  assert(match_serve_court(&m) == 0);
  assert(match_rally(&m, TEAM_US));
  assert(m.state.serving == TEAM_US);
  assert(m.state.server_number == 1);
  assert(match_serve_court(&m) == 0);

  // US gets an odd score, loses service, then regains it: side-out starts right.
  assert(match_rally(&m, TEAM_US));
  assert(m.state.points[TEAM_US] == 1);
  assert(match_serve_court(&m) == 1);
  assert(match_rally(&m, TEAM_THEM));
  assert(m.state.server_number == 2);
  assert(match_serve_court(&m) == 0);
  assert(match_rally(&m, TEAM_THEM));
  assert(m.state.serving == TEAM_THEM);
  assert(match_rally(&m, TEAM_US));
  assert(m.state.serving == TEAM_THEM);
  assert(m.state.server_number == 2);
  assert(match_rally(&m, TEAM_US));
  assert(m.state.serving == TEAM_US);
  assert(m.state.points[TEAM_US] == 1);
  assert(match_serve_court(&m) == 0);
}

static void test_singles(void) {
  Match m;
  Rules r = rules_preset(PRESET_CLASSIC_3);
  r.doubles = 0;
  match_start(&m, r, TEAM_THEM);
  assert(m.state.server_number == 0);
  assert(match_rally(&m, TEAM_US));
  assert(m.state.serving == TEAM_US);
  assert(m.state.points[TEAM_US] == 0);
  assert(match_rally(&m, TEAM_US));
  assert(m.state.points[TEAM_US] == 1);
  assert(match_serve_court(&m) == 1);
}

static void test_rally_variants(void) {
  Match m;
  Rules r = rules_preset(PRESET_USAP_RALLY);
  r.best_of = 1;
  match_start(&m, r, TEAM_THEM);
  m.state.points[TEAM_US] = 10;
  m.state.points[TEAM_THEM] = 8;
  assert(match_rally(&m, TEAM_US));
  assert(m.state.points[TEAM_US] == 11);
  assert(m.state.phase == MATCH_OVER);
  assert(m.state.serving == TEAM_US);

  r = rules_preset(PRESET_COLLEGE_GAME);
  match_start(&m, r, TEAM_THEM);
  m.state.points[TEAM_US] = 20;
  m.state.points[TEAM_THEM] = 18;
  assert(match_rally(&m, TEAM_US));
  assert(m.state.points[TEAM_US] == 20);
  assert(m.state.phase == PLAYING);
  assert(m.state.serving == TEAM_US);
  assert(match_rally(&m, TEAM_US));
  assert(m.state.points[TEAM_US] == 21);
  assert(m.state.phase == MATCH_OVER);

  match_start(&m, r, TEAM_THEM);
  m.state.points[TEAM_US] = 20;
  m.state.points[TEAM_THEM] = 20;
  assert(match_rally(&m, TEAM_US));
  assert(m.state.points[TEAM_US] == 21);
  assert(m.state.phase == PLAYING);
}

static void test_match_and_undo(void) {
  Match m;
  Rules r = rules_preset(PRESET_CLASSIC_3);
  match_start(&m, r, TEAM_US);
  m.id = 42;
  m.state.points[0] = 10;
  m.state.points[1] = 10;
  assert(match_rally(&m, TEAM_US));
  assert(m.state.phase == PLAYING);
  assert(match_undo(&m));
  assert(m.state.points[0] == 10);
  assert(!match_undo(&m));
  assert(match_rally(&m, TEAM_US));
  assert(match_rally(&m, TEAM_US));
  assert(m.state.phase == GAME_OVER);
  assert(m.state.games[0] == 1);
  assert(m.state.game_scores[0][0] == 12);
  assert(match_next_game(&m, TEAM_THEM));
  assert(m.state.serving == TEAM_THEM);
  m.state.points[0] = 10;
  m.state.points[1] = 0;
  m.state.serving = TEAM_US;
  assert(match_rally(&m, TEAM_US));
  assert(m.state.phase == MATCH_OVER);
  assert(!match_next_game(&m, TEAM_US));
  MatchRecord record;
  assert(match_record(&m, 2026, 10, 24, &record));
  assert(record.id == 42);
  assert(record.winner == TEAM_US);
  assert(record.games[0] == 2);
  assert(record.game_scores[0][0] == 12);
  assert(record.game_scores[1][0] == 11);
}

int main(void) {
  test_opening_doubles();
  test_singles();
  test_rally_variants();
  test_match_and_undo();
  puts("pickleball model tests passed");
  return 0;
}
