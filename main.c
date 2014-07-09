//hfuse 0b11111111 lfuse 0b01110110

#define HYSTERESIS 4

#include <avr/io.h>
#include <util/delay.h>

int main() {
  DDRB = 0b00000001;
  PORTB = 0b00000001;

  TCNT0 = 0;
  OCR0A = 128; //start at low luminence
  TCCR0A = (1 << COM0A1) | (1 << COM0A0) | (1 << WGM01) | (1 << WGM00); //set on compare match, clear at top
  TCCR0B = (0 << CS02) | (1 << CS01) | (1 << CS00);

  ADMUX = (0 << REFS0) | (1 << MUX1) | (0 << MUX0);
  ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADPS2) | (0 << ADPS1) | (0 << ADPS0);

  while( ADCSRA & (1 << ADSC) );
  unsigned char res = ADC >> 2;
  const unsigned char delay = 16;
  for(unsigned char i=0; i<res; i++) {
    OCR0A = i;
    _delay_ms(delay);
  }
  OCR0A = res;

  unsigned char old = res;
  while( 1 ) {
    ADCSRA |= (1 << ADSC);
    while( ADCSRA & (1 << ADSC) );
    res = ADC >> 2;
   if( res > old+HYSTERESIS || res < old-HYSTERESIS ) {
      OCR0A = res;
      old = res;
    }
    _delay_ms(20);
  }

  return 0;
}
