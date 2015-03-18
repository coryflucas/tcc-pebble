#include "thermostat_window.h"
#include <pebble.h>

#define ACTION 0
#define CURRENT_TEMP 20
#define COOL_SETPOINT 21
#define HEAT_SETPOINT 22

#define DISPLAY_WIDTH 144

// AppSync and support
static AppSync s_sync;
static uint8_t s_sync_buffer[64];
static char s_current_temp_buffer[8];
static char s_heat_setpoint_buffer[8];
static char s_cool_setpoint_buffer[8];

// Layers
static Window *s_window;
static GFont s_res_bitham_42_bold;
static GFont s_res_gothic_14;
static GFont s_res_gothic_18_bold;
static TextLayer *tl_current_temp;
static TextLayer *tl_current_temp_label;
static TextLayer *tl_cool_label;
static TextLayer *tl_cool_temp;
static TextLayer *tl_heat_label;
static TextLayer *tl_heat_temp;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_fullscreen(s_window, 0);

  s_res_bitham_42_bold = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  s_res_gothic_14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

  // tl_current_temp
  tl_current_temp = text_layer_create(GRect(0, 20, DISPLAY_WIDTH, 50));
  text_layer_set_background_color(tl_current_temp, GColorClear);
  text_layer_set_text_alignment(tl_current_temp, GTextAlignmentCenter);
  text_layer_set_font(tl_current_temp, s_res_bitham_42_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_current_temp);

  // tl_current_temp_label
  tl_current_temp_label = text_layer_create(GRect(0, 3, DISPLAY_WIDTH, 14));
  text_layer_set_background_color(tl_current_temp_label, GColorClear);
  text_layer_set_text(tl_current_temp_label, "Current Temperature");
  text_layer_set_text_alignment(tl_current_temp_label, GTextAlignmentCenter);
  text_layer_set_font(tl_current_temp_label, s_res_gothic_14);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_current_temp_label);

  int heat_x = 0;
  int heat_width = DISPLAY_WIDTH / 2;

  // tl_heat_label
  tl_heat_label = text_layer_create(GRect(heat_x, 80, heat_width, 20));
  text_layer_set_background_color(tl_heat_label, GColorClear);
  text_layer_set_text(tl_heat_label, "Heat");
  text_layer_set_text_alignment(tl_heat_label, GTextAlignmentCenter);
  text_layer_set_font(tl_heat_label, s_res_gothic_14);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_heat_label);

  // tl_heat_temp
  tl_heat_temp = text_layer_create(GRect(heat_x, 100, heat_width, 20));
  text_layer_set_background_color(tl_heat_temp, GColorClear);
  text_layer_set_text_alignment(tl_heat_temp, GTextAlignmentCenter);
  text_layer_set_font(tl_heat_temp, s_res_gothic_18_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_heat_temp);


  int cool_x = DISPLAY_WIDTH / 2 + 1;
  int cool_width = DISPLAY_WIDTH / 2;

  // tl_cool_label
  tl_cool_label = text_layer_create(GRect(cool_x, 80, cool_width, 20));
  text_layer_set_background_color(tl_cool_label, GColorClear);
  text_layer_set_text(tl_cool_label, "Cool");
  text_layer_set_text_alignment(tl_cool_label, GTextAlignmentCenter);
  text_layer_set_font(tl_cool_label, s_res_gothic_14);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_cool_label);

  // tl_cool_temp
  tl_cool_temp = text_layer_create(GRect(cool_x, 100, cool_width, 20));
  text_layer_set_background_color(tl_cool_temp, GColorClear);
  text_layer_set_text_alignment(tl_cool_temp, GTextAlignmentCenter);
  text_layer_set_font(tl_cool_temp, s_res_gothic_18_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_cool_temp);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(tl_current_temp);
  text_layer_destroy(tl_current_temp_label);
  text_layer_destroy(tl_cool_label);
  text_layer_destroy(tl_cool_temp);
  text_layer_destroy(tl_heat_label);
  text_layer_destroy(tl_heat_temp);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

static void sync_temperature_value(const int value, char* buffer, const int buffer_length, TextLayer *target_layer) {
  snprintf(buffer, buffer_length, "%d°", value);
  text_layer_set_text(target_layer, buffer);
}

static void sync_changed_handler(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context) {
  switch(key) {
    case CURRENT_TEMP:
      sync_temperature_value((int)new_tuple->value->int32, s_current_temp_buffer, sizeof(s_current_temp_buffer), tl_current_temp);
      break;
    case HEAT_SETPOINT:
      sync_temperature_value((int)new_tuple->value->int32, s_heat_setpoint_buffer, sizeof(s_heat_setpoint_buffer), tl_heat_temp);
      break;
    case COOL_SETPOINT:
      sync_temperature_value((int)new_tuple->value->int32, s_cool_setpoint_buffer, sizeof(s_cool_setpoint_buffer), tl_cool_temp);
      break;
  }
}

static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Error in app sync: dict_error => %d, app_message_error => %d", dict_error, app_message_error);
}

static void init_app_sync() {
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // Setup initial values
  Tuplet initial_values[] = {
    TupletInteger(CURRENT_TEMP, 0),
    TupletInteger(HEAT_SETPOINT, 0),
    TupletInteger(COOL_SETPOINT, 0),
  };

  // Begin using AppSync
  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_changed_handler, sync_error_handler, NULL);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  Tuplet action_tupple = TupletCString(ACTION, "refresh");

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if(iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &action_tupple);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

void show_thermostat_window(void) {
  initialise_ui();
  init_app_sync();

  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_set_click_config_provider(s_window, click_config_provider);

  window_stack_push(s_window, true);
}

void hide_thermostat_window(void) {
  app_sync_deinit(&s_sync);
  window_stack_remove(s_window, true);
}
