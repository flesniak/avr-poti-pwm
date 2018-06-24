/* debug led helper functions
 * used to output an integer using a flashing led
 * needs LED_ON and LED_OFF to be defined
 * call debug_led_init() once
 * call debug_led_handler regularly
 * call debug_led_set_number to show a new number
 *  (request is ignored if still displaying another number)
 */

#define DEBUG_LED_GRANULARITY 1

struct {
  unsigned char value;
  bool value_set;
  enum { debug_led_idle, debug_led_on, debug_led_off, debug_led_done } state;
  uint16_t wait;
} debug_led;

void debug_led_init(void) {
  LED_OFF;
  debug_led.value = 0;
  debug_led.value_set = false;
  debug_led.state = debug_led_idle;
  debug_led.wait = 0;
}

void debug_led_set_number(unsigned char n) {
  if (debug_led.value_set == false) {
    debug_led.value_set = true;
    debug_led.value = n;
  }
}

void debug_led_handler(void) {
  switch (debug_led.state) {
    case debug_led_idle: // no number saved
      if (debug_led.value_set) {
        debug_led.state = debug_led_off;
        debug_led.wait = 0;
      }
      break;
    case debug_led_on: // while led is on
      if (debug_led.wait == 0) {
        LED_OFF;
        debug_led.state = debug_led_off;
        debug_led.wait = 3000;
      } else
        debug_led.wait--;
      break;
    case debug_led_off: // while led is off
      if (debug_led.wait == 0) {
        if (debug_led.value > DEBUG_LED_GRANULARITY-1) {
          debug_led.value -= DEBUG_LED_GRANULARITY;
          LED_ON;
          debug_led.wait = 1000;
          debug_led.state = debug_led_on;
        } else {
          debug_led.wait = 8000;
          debug_led.state = debug_led_done;
        }
      } else
        debug_led.wait--;
      break;
    case debug_led_done:
      if (debug_led.wait == 0) {
        debug_led.state = debug_led_idle;
        debug_led.value_set = false; // idle condition
      } else
        debug_led.wait--;
      break;
  }
}
