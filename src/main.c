#include <pebble.h>
#include "thermostat_window.h"

void handle_init(void) {
  show_thermostat_window();
}

void handle_deinit(void) {
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
