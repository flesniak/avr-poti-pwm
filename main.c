#define F_CPU 9600000

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdbool.h>

// generated by generate-lut.py
#include "table.h"

#define CUR_PER_POT_MEASURES 255 // 8-bit value, each n current measurements 1 potentiometer measurement is done
#define TARGET_DIFF_THRESHOLD 12 // low-pass filter on current measurement
#define ADMUX_CUR ((1 << REFS0) | (1 << MUX1) | (0 << MUX0)) // REFS0=1 internal reference 1.1V, PB4/ADC2
#define ADMUX_POT ((0 << REFS0) | (1 << MUX1) | (1 << MUX0)) // REFS0=0 Vcc reference 5V, PB3/ADC3
#define WAIT_ADC_READY do {} while(ADCSRA & (1 << ADSC))

// #include "debug-led.h"
#define LED_ON do { PORTB &= ~(1<<PB1); } while(0)
#define LED_OFF do { PORTB |= (1<<PB1); } while(0)

uint16_t sense_target = SENSE_TARGET;
volatile uint16_t adc_result = 0;
volatile uint8_t adc_result_ready = false;

// uint16_t sense_target = 76;
// int8_t threshold = 12;

ISR(ADC_vect) {
  if (!adc_result_ready) {
    adc_result = ADC;
    adc_result_ready = true;
  }
}

int8_t target_diff = 0;

// called when a new current measurement is catched
// updates the output pwm controller
// void update_pwm(const uint16_t adc) {
void update_pwm(void) {
  // static int8_t target_diff = 0;
  // static uint8_t history = 0b00001010; // saves 4 bits of history

  // regulator with low-pass filtering using target_diff
  // deviations from sense_target increase target_diff
  // OCR0A will only be modified on high values of target_diff
  if (adc_result < sense_target) {
    // if ((sense_target-adc) > 20UL) // fast adaption of high deviations
    //   target_diff += 10;
    // else
      target_diff++;
  } else if (adc_result > sense_target) {
    // if ((adc-sense_target) > 20UL) // fast adaption of high deviations
    //   target_diff -= 10;
    // else
      target_diff--;
  }

  /*uint8_t temp;
  if (adc < sense_target) {
    temp = ((sense_target-adc)>>3);
    if (temp == 0)
      temp = 1;
    target_diff += temp;
  } else if (adc > sense_target) {
    temp = ((adc-sense_target)>>3);
    if (temp == 0)
      temp = 1;
    target_diff -= temp;
  }*/

  if (target_diff >= TARGET_DIFF_THRESHOLD) {
    if (OCR0A < 0xff) {
      // if ((history & 0xf) != 0b1010) // suppress oscillation
        OCR0A++;
      // history = (history << 1) | 1;
    }
    target_diff = 0;
  } else if (target_diff <= -1*TARGET_DIFF_THRESHOLD) {
    if (OCR0A > 0) {
      // if ((history & 0xf) != 0b0101) // suppress oscillation
        OCR0A--;
      // history = (history << 1);
    }
    target_diff = 0;
  }
}

// called when a new potentiometer value is catched
// sets the new target brightness
void update_pot(void) {
  // static uint8_t delay = 0;
  // static uint8_t idx = 255;
  // if (delay == 3) {
  //   delay = 0;
  //   if (idx > 0)
  //     idx--;
  //   else
  //     idx = 255;
  //   sense_target = pgm_read_byte(&brightness_lut[idx]);
  // } else
  //   delay++;
  // const uint8_t idx = (adc_result>>ADC_SHIFT_BITS) & ~15;
  // sense_target = pgm_read_byte(&brightness_lut[idx]);
  sense_target = pgm_read_byte(&brightness_lut[192]);
  //OCR0A = value>>2;
  // sense_target = pgm_read_byte(&brightness_lut[adc_result>>ADC_SHIFT_BITS]);
  // if (sense_target > 40)
  //   threshold = 12;
  // else
  //   threshold = 24;
  // uint16_t new_sense_target = pgm_read_byte(&(brightness_lut[value>>ADC_SHIFT_BITS]));
  // uint16_t new_sense_target = value>>2;
  /*if (new_sense_target > sense_target && new_sense_target-sense_target > 3)
    debug_led_set_number(1);
  if (new_sense_target < sense_target && sense_target-new_sense_target > 3)
    debug_led_set_number(2);*/
  // sense_target = new_sense_target;
//   debug_led_set_number(sense_target-15);
}

void adc_collect_pot_measures(void) {
  static uint16_t temp = 0;
  static uint8_t n = 0;
  temp += adc_result;
  n++;
  if (n==4) {
    adc_result = temp>>2;
    // adc_result = 8;
    update_pot();
    temp = 0;
    n = 0;
  }
}

void adc_collect_cur_measures(void) {
  static uint16_t temp = 0;
  static uint8_t n = 0;
  temp += adc_result;
  n++;
  if (n==4) {
    adc_result = temp>>2;
    update_pwm();
    temp = 0;
    n = 0;
  }
}

int main(void) {
  DDRB = 0b00000011; // 5=reset, 4=current sense, 3=pot, 2=NC, 1=LED, 0=pwm out
  PORTB = 0b00000110; // pullup for NC, disable LED (active low)

  TCNT0 = 0;
  OCR0A = 0; // start at mosfet off
  TCCR0A = (1 << COM0A1) | (0 << COM0A0) | (0 << WGM01) | (1 << WGM00); // phase correct pwm mode, clear OC0A on compare match when up-counting, set when down-counting
  TCCR0B = (0 << WGM02) | (0 << CS02) | (0 << CS01) | (1 << CS00); // no prescaler -> 9.6MHz/256/2 = 18.75kHz PWM period clock (2 runs for one pwm period)
  DIDR0 = (1 << ADC0D) | (1 << ADC2D) | (1 << ADC3D) | (1 << ADC1D);
  // OCR0A = 100;

  ADMUX = ADMUX_CUR; // start with current measurement
  ADCSRA = (1 << ADEN) | (1 << ADATE) | (1 << ADIE) | (1 << ADPS2) | (0 << ADPS1) | (1 << ADPS0); // enable automatic adc triggering, prescaler 64
  ADCSRB = (1 << ADTS2) | (0 << ADTS1) | (0 << ADTS0); // adc trigger on timer/counter overflow
  // WAIT_ADC_READY; // wait for adc init

  // debug_led_init();

  sei();

  uint8_t convcount = 0;

  while(1) {
    sleep_mode(); // sleep until conversion is complete
    if (adc_result_ready) {
      // cli();
      switch (convcount) {
        case 249:
          // ADCSRA &= ~(1 << ADEN);
          // ADMUX = ADMUX_POT; // switch source
          // update_pwm(); // but still use this result
          // ADCSRA |= (1 << ADEN);
          break;
        case 250:
          break; // throw away one result
        case 251:
          break; // throw away one result
        case 252:
          // adc_collect_pot_measures();
          // update_pot();
          break; // throw away one result
        case 253:
          // ADCSRA &= ~(1 << ADEN);
          // ADMUX = ADMUX_CUR;
          // ADCSRA |= (1 << ADEN);
          break;
        case 254:
          break; // throw away one result
        case 255:
          break; // throw away one result
        default:
          update_pwm();
          // adc_collect_cur_measures();
      }
      convcount++;
      // adc_collect_cur_measures();
      // if (convcount == CUR_PER_POT_MEASURES) {
      //   ADCSRA &= ~(1 << ADEN) & ~(1 << ADATE);
      //   ADMUX = ADMUX_POT; // switch to potentiometer measurement
      //   ADCSRA |= (1 << ADEN);
      //   WAIT_ADC_READY;
      //   ADC;
      //   WAIT_ADC_READY;
      //   ADC;
      //   // adc_collect_pot_measures();
      //   convcount = 0;
      //   ADCSRA &= ~(1 << ADEN);
      //   ADMUX = ADMUX_CUR;
      //   ADCSRA |= (1 << ADEN) | (1 << ADATE);
      //   WAIT_ADC_READY;
      //   ADC;
      //   WAIT_ADC_READY;
      //   ADC;
      // }
      adc_result_ready = false;
      // sei();
    }
    // debug_led_handler();
  }
}
