#include <pebble.h>
#include <stdio.h>
#include <string.h>

#include "model.h"

#define KEY_MATCH 1
#define KEY_DEFAULTS 2
#define KEY_HISTORY_META 3
#define KEY_TOUCH 4
#define KEY_US_COLOR 5
#define KEY_THEM_COLOR 6
#define KEY_ROLLOVER_BACKUP 7
#define KEY_GPS_ENABLED 8
#define KEY_RECORD_BASE 100
#define HISTORY_CAPACITY 30
#define COLOR_COUNT 6

typedef struct { uint8_t version, count, next; } HistoryMeta;
typedef struct {
  uint8_t slot;
  uint8_t next_before;
  MatchRecord previous;
} RolloverBackup;
typedef enum {
  SCREEN_SCORE, SCREEN_MENU, SCREEN_FORMATS, SCREEN_CUSTOM,
  SCREEN_COLORS, SCREEN_START, SCREEN_NEXT, SCREEN_CALENDAR,
  SCREEN_HISTORY, SCREEN_PLACES, SCREEN_CONFIRM
} Screen;

static Window *s_window;
static Layer *s_canvas;
static Match s_match;
static Rules s_defaults;
static MatchRecord s_records[HISTORY_CAPACITY];
static uint8_t s_count, s_next;
static uint8_t s_colors[2];
static bool s_gps_enabled;
static bool s_touch_enabled, s_touch_subscribed, s_visible, s_touch_active;
static int16_t s_touch_x, s_touch_y;
static Screen s_touch_screen;
static Screen s_screen;
static uint8_t s_cursor, s_history_cursor;
static int16_t s_month_offset;
static char s_notice[32];

static void redraw(void) { if (s_canvas) layer_mark_dirty(s_canvas); }
static void sync_touch(void);
static MatchRecord *record_at(uint8_t order);
static void set_notice(const char *message) {
  snprintf(s_notice, sizeof(s_notice), "%s", message);
  redraw();
}

static bool save_match(void) {
  return persist_write_data(KEY_MATCH, &s_match, sizeof(s_match)) == sizeof(s_match);
}

static void save_defaults(void) {
  persist_write_data(KEY_DEFAULTS, &s_defaults, sizeof(s_defaults));
}

static void load_data(void) {
  s_defaults = rules_preset(PRESET_CLASSIC_3);
  Rules loaded_rules;
  if (persist_read_data(KEY_DEFAULTS, &loaded_rules, sizeof(loaded_rules)) ==
      sizeof(loaded_rules) && rules_valid(loaded_rules)) s_defaults = loaded_rules;
  match_start(&s_match, s_defaults, TEAM_US);
  Match loaded_match;
  if (persist_read_data(KEY_MATCH, &loaded_match, sizeof(loaded_match)) ==
      sizeof(loaded_match) && loaded_match.version == 2 &&
      rules_valid(loaded_match.rules) && loaded_match.state.phase <= MATCH_OVER &&
      loaded_match.state.game_index < loaded_match.rules.best_of &&
      loaded_match.state.serving <= 1) s_match = loaded_match;
  if (!s_match.id) s_match.id = 1;
  HistoryMeta meta;
  if (persist_read_data(KEY_HISTORY_META, &meta, sizeof(meta)) == sizeof(meta) &&
      meta.version == 1 && meta.count <= HISTORY_CAPACITY &&
      meta.next < HISTORY_CAPACITY) {
    s_count = meta.count;
    s_next = meta.next;
  }
  for (uint8_t i = 0; i < HISTORY_CAPACITY; i++)
    persist_read_data(KEY_RECORD_BASE + i, &s_records[i], sizeof(MatchRecord));
  RolloverBackup backup;
  if (persist_read_data(KEY_ROLLOVER_BACKUP, &backup, sizeof(backup)) ==
      sizeof(backup)) {
    if (backup.slot < HISTORY_CAPACITY && s_count == HISTORY_CAPACITY &&
        s_next == backup.next_before) {
      s_records[backup.slot] = backup.previous;
      if (persist_write_data(KEY_RECORD_BASE + backup.slot, &backup.previous,
                             sizeof(backup.previous)) == sizeof(backup.previous))
        persist_delete(KEY_ROLLOVER_BACKUP);
    } else persist_delete(KEY_ROLLOVER_BACKUP);
  }
  if (s_count && s_match.state.phase == MATCH_OVER && !s_match.saved) {
    MatchRecord *latest = record_at(s_count - 1);
    if (latest && latest->id == s_match.id) {
      s_match.saved = 1;
      save_match();
    }
  }
  s_touch_enabled = !persist_exists(KEY_TOUCH) || persist_read_bool(KEY_TOUCH);
  s_gps_enabled = !persist_exists(KEY_GPS_ENABLED) ||
                  persist_read_bool(KEY_GPS_ENABLED);
  s_colors[0] = 0;
  s_colors[1] = 1;
  if (persist_exists(KEY_US_COLOR)) {
    int value = persist_read_int(KEY_US_COLOR);
    if (value >= 0 && value < COLOR_COUNT) s_colors[0] = value;
  }
  if (persist_exists(KEY_THEM_COLOR)) {
    int value = persist_read_int(KEY_THEM_COLOR);
    if (value >= 0 && value < COLOR_COUNT) s_colors[1] = value;
  }
}

static MatchRecord *record_at(uint8_t order) {
  if (order >= s_count) return NULL;
  return &s_records[(s_next + HISTORY_CAPACITY - s_count + order) % HISTORY_CAPACITY];
}

static void request_location(uint8_t slot) {
  if (!s_gps_enabled) return;
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) != APP_MSG_OK) return;
  dict_write_uint8(out, MESSAGE_KEY_LOC_REQUEST, 1);
  dict_write_uint8(out, MESSAGE_KEY_LOC_SLOT, slot);
  dict_write_uint32(out, MESSAGE_KEY_LOC_MATCH_ID, s_records[slot].id);
  app_message_outbox_send();
}

static void inbox_received(DictionaryIterator *iterator, void *context) {
  (void)context;
  Tuple *slot_value = dict_find(iterator, MESSAGE_KEY_LOC_SLOT);
  Tuple *match_id = dict_find(iterator, MESSAGE_KEY_LOC_MATCH_ID);
  Tuple *status = dict_find(iterator, MESSAGE_KEY_LOC_STATUS);
  Tuple *latitude = dict_find(iterator, MESSAGE_KEY_LOC_LAT_E6);
  Tuple *longitude = dict_find(iterator, MESSAGE_KEY_LOC_LON_E6);
  Tuple *accuracy = dict_find(iterator, MESSAGE_KEY_LOC_ACCURACY_M);
  if (!slot_value || !match_id || !status || !latitude || !longitude || !accuracy ||
      status->value->uint8 != 1) return;
  uint8_t slot = slot_value->value->uint8;
  if (slot >= HISTORY_CAPACITY || s_records[slot].id != match_id->value->uint32)
    return;
  int32_t lat = latitude->value->int32;
  int32_t lon = longitude->value->int32;
  if (lat < -90000000 || lat > 90000000 ||
      lon < -180000000 || lon > 180000000) return;
  MatchRecord updated = s_records[slot];
  updated.latitude_e6 = lat;
  updated.longitude_e6 = lon;
  updated.accuracy_m = accuracy->value->uint16;
  updated.has_location = 1;
  if (persist_write_data(KEY_RECORD_BASE + slot, &updated,
                         sizeof(updated)) == sizeof(updated)) {
    s_records[slot] = updated;
    redraw();
  }
}

static bool save_record(void) {
  if (s_match.saved) return true;
  if (s_match.state.phase != MATCH_OVER) return false;
  time_t now = time(NULL);
  struct tm *date = localtime(&now);
  MatchRecord record;
  if (!date || !match_record(&s_match, date->tm_year + 1900,
                             date->tm_mon + 1, date->tm_mday, &record)) return false;
  uint8_t slot = s_next;
  bool replacing = s_count == HISTORY_CAPACITY;
  MatchRecord previous = s_records[slot];
  if (replacing) {
    RolloverBackup backup = {.slot = slot, .next_before = s_next,
                             .previous = previous};
    if (persist_write_data(KEY_ROLLOVER_BACKUP, &backup,
                           sizeof(backup)) != sizeof(backup)) return false;
  }
  if (persist_write_data(KEY_RECORD_BASE + slot, &record,
                         sizeof(record)) != sizeof(record)) {
    if (replacing && persist_write_data(KEY_RECORD_BASE + slot, &previous,
                                       sizeof(previous)) == sizeof(previous))
      persist_delete(KEY_ROLLOVER_BACKUP);
    return false;
  }
  s_records[slot] = record;
  HistoryMeta meta = {.version = 1,
                      .count = s_count < HISTORY_CAPACITY ? s_count + 1 : s_count,
                      .next = (uint8_t)((slot + 1) % HISTORY_CAPACITY)};
  if (persist_write_data(KEY_HISTORY_META, &meta, sizeof(meta)) != sizeof(meta)) {
    s_records[slot] = previous;
    if (replacing && persist_write_data(KEY_RECORD_BASE + slot, &previous,
                                       sizeof(previous)) == sizeof(previous))
      persist_delete(KEY_ROLLOVER_BACKUP);
    return false;
  }
  s_count = meta.count;
  s_next = meta.next;
  if (replacing) persist_delete(KEY_ROLLOVER_BACKUP);
  s_match.saved = 1;
  if (!save_match()) set_notice("Saved; state retry later");
  request_location(slot);
  return true;
}

static GColor team_color(uint8_t team) {
  switch (s_colors[team]) {
    case 0: return GColorBlue;
    case 1: return GColorRed;
    case 2: return GColorDarkGreen;
    case 3: return GColorPurple;
    case 4: return GColorOrange;
    default: return GColorBlack;
  }
}

static const char *color_name(uint8_t color) {
  static const char *names[COLOR_COUNT] = {
    "Blue", "Red", "Green", "Purple", "Orange", "Black"
  };
  return names[color % COLOR_COUNT];
}

static void fill(GContext *ctx, GColor color, GRect rect) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, rect, 0, GCornerNone);
}

static void label(GContext *ctx, const char *text, const char *font,
                  GColor color, GRect rect, GTextAlignment align) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, text, fonts_get_system_font(font), rect,
                     GTextOverflowModeTrailingEllipsis, align, NULL);
}

static void heading(GContext *ctx, const char *title) {
  fill(ctx, GColorBlack, GRect(0, 0, 200, 35));
  label(ctx, title, FONT_KEY_GOTHIC_24_BOLD, GColorWhite,
        GRect(8, 1, 184, 33), GTextAlignmentCenter);
}

static void rows(GContext *ctx, const char *title, const char **items,
                 uint8_t count) {
  fill(ctx, GColorWhite, GRect(0, 0, 200, 228));
  heading(ctx, title);
  uint8_t first = s_cursor >= 5 ? s_cursor - 4 : 0;
  if (count > 5 && first > count - 5) first = count - 5;
  for (uint8_t i = first; i < count && i < first + 5; i++) {
    int16_t y = 39 + (i - first) * 37;
    if (i == s_cursor) fill(ctx, team_color(0), GRect(6, y, 188, 34));
    label(ctx, items[i], FONT_KEY_GOTHIC_18_BOLD,
          i == s_cursor ? GColorWhite : GColorBlack,
          GRect(10, y + 3, 180, 30), GTextAlignmentLeft);
  }
}

static void draw_score(GContext *ctx) {
  GameState *s = &s_match.state;
  char text[48];
  fill(ctx, GColorFromRGB(238, 232, 220), GRect(0, 0, 200, 228));
  snprintf(text, sizeof(text), "GAME %u / %u", s->game_index + 1, s_match.rules.best_of);
  label(ctx, text, FONT_KEY_GOTHIC_18_BOLD, GColorBlack,
        GRect(8, 4, 115, 28), GTextAlignmentLeft);
  snprintf(text, sizeof(text), "%u - %u", s->games[0], s->games[1]);
  label(ctx, text, FONT_KEY_GOTHIC_18_BOLD, GColorBlack,
        GRect(125, 4, 68, 28), GTextAlignmentRight);
  fill(ctx, team_color(0), GRect(6, 36, 91, 110));
  fill(ctx, team_color(1), GRect(103, 36, 91, 110));
  label(ctx, "US", FONT_KEY_GOTHIC_24_BOLD, GColorWhite,
        GRect(8, 39, 87, 32), GTextAlignmentCenter);
  label(ctx, "THEM", FONT_KEY_GOTHIC_24_BOLD, GColorWhite,
        GRect(105, 39, 87, 32), GTextAlignmentCenter);
  snprintf(text, sizeof(text), "%u", s->points[0]);
  label(ctx, text, FONT_KEY_BITHAM_42_BOLD, GColorWhite,
        GRect(7, 69, 89, 67), GTextAlignmentCenter);
  snprintf(text, sizeof(text), "%u", s->points[1]);
  label(ctx, text, FONT_KEY_BITHAM_42_BOLD, GColorWhite,
        GRect(104, 69, 89, 67), GTextAlignmentCenter);
  if (s->phase == PLAYING) {
    if (s_match.rules.scoring == SIDE_OUT && s_match.rules.doubles)
      snprintf(text, sizeof(text), "SERVE: %s #%u  %s", s->serving ? "THEM" : "US",
               s->server_number, match_serve_court(&s_match) ? "LEFT" : "RIGHT");
    else snprintf(text, sizeof(text), "SERVE: %s  %s", s->serving ? "THEM" : "US",
                  match_serve_court(&s_match) ? "LEFT" : "RIGHT");
    label(ctx, text, FONT_KEY_GOTHIC_18_BOLD, GColorBlack,
          GRect(4, 151, 192, 28), GTextAlignmentCenter);
    if (s_match.rules.scoring == SIDE_OUT && s_match.rules.doubles) {
      snprintf(text, sizeof(text), "CALL %u-%u-%u", s->points[s->serving],
               s->points[1 - s->serving], s->server_number);
    } else snprintf(text, sizeof(text), "CALL %u-%u", s->points[s->serving],
                    s->points[1 - s->serving]);
    label(ctx, text, FONT_KEY_GOTHIC_18, GColorBlack,
          GRect(4, 174, 192, 25), GTextAlignmentCenter);
    label(ctx, "UP US WINS  /  DOWN THEM WINS", FONT_KEY_GOTHIC_14_BOLD,
          GColorBlack, GRect(2, 202, 196, 22), GTextAlignmentCenter);
  } else {
    bool won = s->phase == MATCH_OVER ? s->games[0] > s->games[1] :
               s->points[0] > s->points[1];
    snprintf(text, sizeof(text), "%s %s", s->phase == MATCH_OVER ? "MATCH" : "GAME",
             won ? "WON" : "LOST");
    label(ctx, text, FONT_KEY_GOTHIC_28_BOLD, GColorBlack,
          GRect(4, 151, 192, 43), GTextAlignmentCenter);
    label(ctx, s->phase == MATCH_OVER ?
          (s_match.saved ? "SAVED  |  SELECT MENU" : "SELECT SAVE MATCH") :
          "SELECT NEXT GAME", FONT_KEY_GOTHIC_18_BOLD, GColorBlack,
          GRect(3, 195, 194, 28), GTextAlignmentCenter);
  }
  if (s_notice[0]) {
    fill(ctx, GColorBlack, GRect(8, 177, 184, 26));
    label(ctx, s_notice, FONT_KEY_GOTHIC_18_BOLD, GColorWhite,
          GRect(9, 178, 182, 25), GTextAlignmentCenter);
  }
}

static bool leap(int year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static int month_days(int year, int month) {
  static const uint8_t days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  return days[month - 1] + (month == 2 && leap(year));
}

static void selected_month(int *year, int *month) {
  time_t now = time(NULL);
  struct tm *today = localtime(&now);
  int index = (today ? today->tm_year + 1900 : 2026) * 12 +
              (today ? today->tm_mon : 0) + s_month_offset;
  *year = index / 12;
  *month = index % 12 + 1;
}

static int first_weekday(int year, int month) {
  int y = year, m = month;
  if (m < 3) { m += 12; y--; }
  int h = (1 + 13 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
  return (h + 6) % 7;  // Sunday = 0
}

static int latest_record_in_month(int year, int month) {
  for (int i = s_count - 1; i >= 0; i--) {
    MatchRecord *record = record_at(i);
    if (record && record->year == year && record->month == month) return i;
  }
  return -1;
}

static void draw_calendar(GContext *ctx) {
  int year, month;
  selected_month(&year, &month);
  fill(ctx, GColorWhite, GRect(0, 0, 200, 228));
  char text[32];
  snprintf(text, sizeof(text), "MATCHES %02d/%d", month, year);
  heading(ctx, text);
  label(ctx, "S   M   T   W   T   F   S", FONT_KEY_GOTHIC_18_BOLD,
        GColorBlack, GRect(9, 38, 182, 25), GTextAlignmentCenter);
  int start = first_weekday(year, month);
  int days = month_days(year, month);
  for (int day = 1; day <= days; day++) {
    int cell = start + day - 1;
    int x = 8 + cell % 7 * 27;
    int y = 65 + cell / 7 * 25;
    uint8_t result = 0;
    for (uint8_t i = 0; i < s_count; i++) {
      MatchRecord *record = record_at(i);
      if (record->year == year && record->month == month && record->day == day)
        result = record->winner == 0 ? 1 : 2;
    }
    if (result) fill(ctx, team_color(result - 1), GRect(x, y, 24, 23));
    snprintf(text, sizeof(text), "%d", day);
    label(ctx, text, FONT_KEY_GOTHIC_18_BOLD,
          result ? GColorWhite : GColorBlack,
          GRect(x, y, 24, 23), GTextAlignmentCenter);
  }
  label(ctx, latest_record_in_month(year, month) >= 0 ?
        "SELECT MATCH  |  UP OLDER" : "NO MATCHES  |  UP OLDER",
        FONT_KEY_GOTHIC_14_BOLD,
        GColorBlack, GRect(4, 205, 192, 20), GTextAlignmentCenter);
}

static void draw_history(GContext *ctx) {
  fill(ctx, GColorWhite, GRect(0, 0, 200, 228));
  heading(ctx, "SAVED MATCH");
  if (!s_count) {
    label(ctx, "No saved matches", FONT_KEY_GOTHIC_24_BOLD, GColorBlack,
          GRect(4, 94, 192, 35), GTextAlignmentCenter);
    return;
  }
  MatchRecord *r = record_at(s_history_cursor);
  char text[64];
  snprintf(text, sizeof(text), "%02u/%02u/%04u", r->month, r->day, r->year);
  label(ctx, text, FONT_KEY_GOTHIC_24_BOLD, GColorBlack,
        GRect(4, 42, 192, 35), GTextAlignmentCenter);
  fill(ctx, team_color(r->winner), GRect(13, 83, 174, 51));
  snprintf(text, sizeof(text), "%s  %u - %u", r->winner ? "LOSS" : "WIN",
           r->games[0], r->games[1]);
  label(ctx, text, FONT_KEY_GOTHIC_28_BOLD, GColorWhite,
        GRect(16, 88, 168, 43), GTextAlignmentCenter);
  for (uint8_t i = 0; i < r->rules.best_of; i++) {
    if (r->game_scores[i][0] == 0 && r->game_scores[i][1] == 0) break;
    snprintf(text, sizeof(text), "G%u %u-%u", i + 1,
             r->game_scores[i][0], r->game_scores[i][1]);
    label(ctx, text, FONT_KEY_GOTHIC_18_BOLD, GColorBlack,
          GRect(6 + (i % 2) * 100, 144 + (i / 2) * 22, 94, 22),
          GTextAlignmentCenter);
  }
  if (r->has_location) {
    int32_t lat = r->latitude_e6 < 0 ? -r->latitude_e6 : r->latitude_e6;
    int32_t lon = r->longitude_e6 < 0 ? -r->longitude_e6 : r->longitude_e6;
    snprintf(text, sizeof(text), "GPS %s%ld.%04ld %s%ld.%04ld",
             r->latitude_e6 < 0 ? "-" : "", (long)(lat / 1000000),
             (long)((lat % 1000000) / 100), r->longitude_e6 < 0 ? "-" : "",
             (long)(lon / 1000000), (long)((lon % 1000000) / 100));
    label(ctx, text, FONT_KEY_GOTHIC_14, GColorBlack,
          GRect(2, 190, 196, 18), GTextAlignmentCenter);
  }
  snprintf(text, sizeof(text), "%u OF %u", s_history_cursor + 1, s_count);
  label(ctx, text, FONT_KEY_GOTHIC_14_BOLD, GColorBlack,
        GRect(6, 207, 188, 18), GTextAlignmentCenter);
}

static void draw_places(GContext *ctx) {
  fill(ctx, GColorWhite, GRect(0, 0, 200, 228));
  heading(ctx, "MATCH PLACES");
  int32_t min_lat = 90000000, max_lat = -90000000;
  int32_t min_lon = 180000000, max_lon = -180000000;
  uint8_t plotted = 0;
  for (uint8_t i = 0; i < s_count; i++) {
    MatchRecord *r = record_at(i);
    if (!r->has_location) continue;
    if (r->latitude_e6 < min_lat) min_lat = r->latitude_e6;
    if (r->latitude_e6 > max_lat) max_lat = r->latitude_e6;
    if (r->longitude_e6 < min_lon) min_lon = r->longitude_e6;
    if (r->longitude_e6 > max_lon) max_lon = r->longitude_e6;
    plotted++;
  }
  if (!plotted) {
    label(ctx, "No GPS matches yet", FONT_KEY_GOTHIC_24_BOLD, GColorBlack,
          GRect(4, 93, 192, 36), GTextAlignmentCenter);
    return;
  }
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, GRect(17, 49, 166, 139));
  label(ctx, "N", FONT_KEY_GOTHIC_14_BOLD, GColorBlack,
        GRect(91, 38, 18, 18), GTextAlignmentCenter);
  int64_t lat_span = (int64_t)max_lat - min_lat;
  int64_t lon_span = (int64_t)max_lon - min_lon;
  for (uint8_t i = 0; i < s_count; i++) {
    MatchRecord *r = record_at(i);
    if (!r->has_location) continue;
    int x = lon_span ? 22 + (int)(((int64_t)r->longitude_e6 - min_lon) * 156 / lon_span) : 100;
    int y = lat_span ? 54 + (int)(((int64_t)max_lat - r->latitude_e6) * 129 / lat_span) : 119;
    fill(ctx, team_color(r->winner), GRect(x - 4, y - 4, 9, 9));
  }
  char text[40];
  snprintf(text, sizeof(text), "%u GPS RESULT%s", plotted, plotted == 1 ? "" : "S");
  label(ctx, text, FONT_KEY_GOTHIC_14_BOLD, GColorBlack,
        GRect(2, 194, 196, 20), GTextAlignmentCenter);
}

static const char *menu_items[] = {
  "Undo rally", "Calendar / history", "Match places", "Match format",
  "Colors / touch", "New match"
};
static const char *format_items[] = {
  "Classic 11 best of 3", "Classic 15 one game", "Pro final 11 / 5",
  "USAP rally 11 / 3", "School rally 21", "Custom rules"
};

static void draw_custom(GContext *ctx) {
  char items_buf[4][32];
  snprintf(items_buf[0], sizeof(items_buf[0]), "Points: %u", s_defaults.target);
  snprintf(items_buf[1], sizeof(items_buf[1]), "Best of: %u", s_defaults.best_of);
  snprintf(items_buf[2], sizeof(items_buf[2]), "Scoring: %s",
           s_defaults.scoring == SIDE_OUT ? "side-out" :
           s_defaults.scoring == RALLY_2026 ? "rally 2026" : "rally freeze");
  snprintf(items_buf[3], sizeof(items_buf[3]), "%s", s_defaults.doubles ?
           "Doubles" : "Singles");
  const char *items[5] = {items_buf[0], items_buf[1], items_buf[2], items_buf[3], "Done"};
  rows(ctx, "CUSTOM FORMAT", items, 5);
}

static void draw_colors(GContext *ctx) {
  char a[32], b[32], c[32], d[32];
  snprintf(a, sizeof(a), "US: %s", color_name(s_colors[0]));
  snprintf(b, sizeof(b), "THEM: %s", color_name(s_colors[1]));
  snprintf(c, sizeof(c), "Touch: %s", s_touch_enabled ? "on" : "off");
  snprintf(d, sizeof(d), "Save GPS: %s", s_gps_enabled ? "on" : "off");
  const char *items[5] = {a, b, c, d, "Done"};
  rows(ctx, "COLORS / INPUT", items, 5);
  fill(ctx, GColorBlack, GRect(160, 42, 28, 26));
  fill(ctx, GColorWhite, GRect(161, 43, 26, 24));
  fill(ctx, team_color(0), GRect(162, 44, 24, 22));
  fill(ctx, GColorBlack, GRect(160, 79, 28, 26));
  fill(ctx, GColorWhite, GRect(161, 80, 26, 24));
  fill(ctx, team_color(1), GRect(162, 81, 24, 22));
}

static void update_proc(Layer *layer, GContext *ctx) {
  (void)layer;
  switch (s_screen) {
    case SCREEN_SCORE: draw_score(ctx); break;
    case SCREEN_MENU: rows(ctx, "MATCH MENU", menu_items, 6); break;
    case SCREEN_FORMATS: rows(ctx, "MATCH FORMAT", format_items, 6); break;
    case SCREEN_CUSTOM: draw_custom(ctx); break;
    case SCREEN_COLORS: draw_colors(ctx); break;
    case SCREEN_START: {
      const char *items[] = {"US serves first", "THEM serves first", "Cancel"};
      rows(ctx, "NEW MATCH", items, 3);
      break;
    }
    case SCREEN_NEXT: {
      const char *items[] = {"US serves first", "THEM serves first", "Back"};
      rows(ctx, "NEXT GAME", items, 3);
      break;
    }
    case SCREEN_CALENDAR: draw_calendar(ctx); break;
    case SCREEN_HISTORY: draw_history(ctx); break;
    case SCREEN_PLACES: draw_places(ctx); break;
    case SCREEN_CONFIRM: {
      const char *items[] = {"Yes, discard score", "Keep current match"};
      rows(ctx, "RESET MATCH?", items, 2);
      break;
    }
  }
}

static void award_rally(uint8_t team, bool tapped) {
  if (s_screen != SCREEN_SCORE || !match_rally(&s_match, team)) return;
  s_notice[0] = '\0';
  if (!save_match()) set_notice("Score save failed");
  if (s_match.state.phase != PLAYING || tapped) vibes_short_pulse();
  redraw();
}

static uint8_t list_count(void) {
  switch (s_screen) {
    case SCREEN_MENU: return 6;
    case SCREEN_FORMATS: return 6;
    case SCREEN_CUSTOM: return 5;
    case SCREEN_COLORS: return 5;
    case SCREEN_START: case SCREEN_NEXT: return 3;
    case SCREEN_CONFIRM: return 2;
    default: return 0;
  }
}

static void up_click(ClickRecognizerRef ref, void *context) {
  (void)ref; (void)context;
  if (s_screen == SCREEN_SCORE) award_rally(TEAM_US, false);
  else if (s_screen == SCREEN_CALENDAR && s_month_offset > -120) s_month_offset--;
  else if (s_screen == SCREEN_HISTORY && s_history_cursor > 0) s_history_cursor--;
  else { uint8_t n = list_count(); if (n) s_cursor = (s_cursor + n - 1) % n; }
  redraw();
}

static void down_click(ClickRecognizerRef ref, void *context) {
  (void)ref; (void)context;
  if (s_screen == SCREEN_SCORE) award_rally(TEAM_THEM, false);
  else if (s_screen == SCREEN_CALENDAR && s_month_offset < 0) s_month_offset++;
  else if (s_screen == SCREEN_HISTORY && s_history_cursor + 1 < s_count) s_history_cursor++;
  else { uint8_t n = list_count(); if (n) s_cursor = (s_cursor + 1) % n; }
  redraw();
}

static void select_click(ClickRecognizerRef ref, void *context) {
  (void)ref; (void)context;
  s_notice[0] = '\0';
  if (s_screen == SCREEN_SCORE) {
    if (s_match.state.phase == GAME_OVER) { s_screen = SCREEN_NEXT; s_cursor = 0; }
    else if (s_match.state.phase == MATCH_OVER && !s_match.saved) {
      set_notice(save_record() ? "Match saved" : "Save failed; retry");
    } else { s_screen = SCREEN_MENU; s_cursor = 0; }
  } else if (s_screen == SCREEN_MENU) {
    switch (s_cursor) {
      case 0:
        if (match_undo(&s_match)) { save_match(); set_notice("Rally undone"); }
        else set_notice("Nothing to undo");
        s_screen = SCREEN_SCORE;
        break;
      case 1: s_screen = SCREEN_CALENDAR; s_month_offset = 0; break;
      case 2: s_screen = SCREEN_PLACES; break;
      case 3: s_screen = SCREEN_FORMATS; s_cursor = 0; break;
      case 4: s_screen = SCREEN_COLORS; s_cursor = 0; break;
      default: s_screen = SCREEN_CONFIRM; s_cursor = 0; break;
    }
  } else if (s_screen == SCREEN_FORMATS) {
    if (s_cursor < PRESET_COUNT) {
      s_defaults = rules_preset((Preset)s_cursor);
      save_defaults();
      s_screen = SCREEN_MENU;
      s_cursor = 3;
    } else { s_screen = SCREEN_CUSTOM; s_cursor = 0; }
  } else if (s_screen == SCREEN_CUSTOM) {
    switch (s_cursor) {
      case 0: s_defaults.target = s_defaults.target == 11 ? 15 :
                                  s_defaults.target == 15 ? 21 : 11; break;
      case 1: s_defaults.best_of = s_defaults.best_of == 1 ? 3 :
                                   s_defaults.best_of == 3 ? 5 : 1; break;
      case 2: s_defaults.scoring = (s_defaults.scoring + 1) % 3; break;
      case 3: s_defaults.doubles = !s_defaults.doubles; break;
      default: save_defaults(); s_screen = SCREEN_FORMATS; s_cursor = 5; break;
    }
  } else if (s_screen == SCREEN_COLORS) {
    if (s_cursor < 2) {
      uint8_t team = s_cursor;
      s_colors[team] = (s_colors[team] + 1) % COLOR_COUNT;
      persist_write_int(team ? KEY_THEM_COLOR : KEY_US_COLOR, s_colors[team]);
    } else if (s_cursor == 2) {
      s_touch_enabled = !s_touch_enabled;
      persist_write_bool(KEY_TOUCH, s_touch_enabled);
      sync_touch();
    } else if (s_cursor == 3) {
      s_gps_enabled = !s_gps_enabled;
      persist_write_bool(KEY_GPS_ENABLED, s_gps_enabled);
    } else { s_screen = SCREEN_MENU; s_cursor = 4; }
  } else if (s_screen == SCREEN_CONFIRM) {
    if (s_cursor == 0) { s_screen = SCREEN_START; s_cursor = 0; }
    else { s_screen = SCREEN_SCORE; }
  } else if (s_screen == SCREEN_START) {
    if (s_cursor <= 1) {
      uint32_t id = s_match.id == UINT32_MAX ? 1 : s_match.id + 1;
      match_start(&s_match, s_defaults, s_cursor);
      s_match.id = id;
      save_match();
    }
    s_screen = SCREEN_SCORE;
  } else if (s_screen == SCREEN_NEXT) {
    if (s_cursor <= 1) {
      match_next_game(&s_match, s_cursor);
      save_match();
    }
    s_screen = SCREEN_SCORE;
  } else if (s_screen == SCREEN_CALENDAR) {
    int year, month;
    selected_month(&year, &month);
    int index = latest_record_in_month(year, month);
    if (index >= 0) {
      s_screen = SCREEN_HISTORY;
      s_history_cursor = index;
    }
  }
  redraw();
}

static void back_click(ClickRecognizerRef ref, void *context) {
  (void)ref; (void)context;
  if (s_screen == SCREEN_SCORE) { window_stack_pop(true); return; }
  if (s_screen == SCREEN_HISTORY) s_screen = SCREEN_CALENDAR;
  else if (s_screen == SCREEN_CUSTOM) { save_defaults(); s_screen = SCREEN_FORMATS; s_cursor = 5; }
  else if (s_screen == SCREEN_FORMATS || s_screen == SCREEN_COLORS ||
           s_screen == SCREEN_CONFIRM) { s_screen = SCREEN_MENU; s_cursor = 0; }
  else s_screen = SCREEN_SCORE;
  redraw();
}

static void long_select(ClickRecognizerRef ref, void *context) {
  (void)ref; (void)context;
  if (s_screen != SCREEN_SCORE) return;
  if (match_undo(&s_match)) { save_match(); set_notice("Rally undone"); }
  else set_notice("Nothing to undo");
}

static void click_config(void *context) {
  (void)context;
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
  window_long_click_subscribe(BUTTON_ID_SELECT, 700, long_select, NULL);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click);
}

#if defined(PBL_TOUCH)
static void touch_handler(const TouchEvent *event, void *context) {
  (void)context;
  if (event->type == TouchEvent_Touchdown) {
    s_touch_active = !event->non_navigational;
    s_touch_x = event->x;
    s_touch_y = event->y;
    s_touch_screen = s_screen;
    return;
  }
  if (event->type != TouchEvent_Liftoff || !s_touch_active) return;
  s_touch_active = false;
  if (s_touch_screen != s_screen ||
      event->x - s_touch_x > 15 || s_touch_x - event->x > 15 ||
      event->y - s_touch_y > 15 || s_touch_y - event->y > 15) return;
  if (s_screen == SCREEN_SCORE && s_match.state.phase == PLAYING &&
      event->y >= 36 && event->y < 146 && s_touch_y >= 36 && s_touch_y < 146) {
    if (event->x < 97 && s_touch_x < 97) award_rally(TEAM_US, true);
    else if (event->x >= 103 && s_touch_x >= 103) award_rally(TEAM_THEM, true);
  } else if (s_screen != SCREEN_SCORE && list_count() &&
             event->y >= 39 && event->y < 224 &&
             s_touch_y >= 39 && s_touch_y < 224) {
    uint8_t row = (event->y - 39) / 37;
    if (row == (s_touch_y - 39) / 37 && row < list_count()) {
      uint8_t first = s_cursor >= 5 ? s_cursor - 4 : 0;
      if (list_count() > 5 && first > list_count() - 5)
        first = list_count() - 5;
      s_cursor = first + row;
      select_click(NULL, NULL);
    }
  }
}
#endif

static void sync_touch(void) {
#if defined(PBL_TOUCH)
  bool want = s_visible && s_touch_enabled;
  if (want && !s_touch_subscribed) {
    touch_service_subscribe(touch_handler, NULL);
    s_touch_subscribed = true;
  } else if (!want && s_touch_subscribed) {
    touch_service_unsubscribe();
    s_touch_subscribed = false;
  }
#endif
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, update_proc);
  layer_add_child(root, s_canvas);
  s_visible = true;
  sync_touch();
}

static void window_unload(Window *window) {
  (void)window;
  s_visible = false;
  sync_touch();
  layer_destroy(s_canvas);
  s_canvas = NULL;
}

int main(void) {
  load_data();
  app_message_register_inbox_received(inbox_received);
  app_message_open(256, 128);
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load, .unload = window_unload
  });
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
  app_event_loop();
  window_destroy(s_window);
  return 0;
}
