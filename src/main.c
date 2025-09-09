#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

/* IO

                 ATtiny85
                 ┌──◡──┐
   SWITCH    PB5 □1   8□ VCC
   ENCODER-B PB3 □2   7□ PB2 CLOCK-2
   ENCODER-A PB4 □3   6□ PB1 CLOCK-1
             GND □4   5□ PB0 MIDI-RX
                 └─────┘
 */
void setup_io() {
  DDRB = _BV(DDB2) | _BV(DDB1);      // pins 6 and 7 are outputs, others are inputs
  PORTB = _BV(PORTB2) | _BV(PORTB1); // set clock outputs high, pull-ups disabled
}

#define PPQN 24
// sent 24 timers per quarter note
#define MIDI_TIMING_CLOCK 0b11111000
#define MIDI_START 0b11111010

#define NOPTS 11
uint8_t CLKOPTS[NOPTS] = {
  1,        // original clock
  PPQN/8,   // 32nd
  PPQN/6,   // triplet 16th
  PPQN/4,   // 16th
  PPQN/3,   // triplet 8th
  PPQN/2,   // 8th
  PPQN*2/3, // triplet quarter
  PPQN,     // quarter
  PPQN*2,   // half
  PPQN*4,   // whole
  PPQN*8,   // double whole
};
uint8_t clksel1 = 3; // 16th
uint8_t clksel2 = 7; // quarter
uint8_t clkcnt1 = 0;
uint8_t clkcnt2 = 0;

uint8_t* cursel = &clksel1;

/* Set Timer1 to limit clock trigger time. */
void setup_clock_trigger_timer() {
  TCCR1 = _BV(CS13) | _BV(CS12) | _BV(CS11); // set prescaler to /8192 (~ 1 KHz)
  OCR1A = 2;                                 // set delay to ~ 3 ms
}

/* Turn off clock trigger. */
ISR (TIMER1_COMPA_vect) {
  PORTB |= _BV(PORTB2) | _BV(PORTB1); // set clocks high
  TIMSK &= ~_BV(OCIE1A);              // disable output compare match interrupt
}

/* Activate trigger and start timer to turn the signal off. */
void trigger_clocks() {
  if (clkcnt1 == 0 || clkcnt2 == 0) {
    TCNT1 = 0;            // reset counter
    TIFR |= _BV(OCF1A);   // clear output compare flag
    TIMSK |= _BV(OCIE1A); // enable output compare match interrupt
  }

  if (clkcnt1 == 0) {
    PORTB &= ~_BV(PORTB1); // set clock low
    clkcnt1 = CLKOPTS[clksel1];
  }
  --clkcnt1;

  if (clkcnt2 == 0) {
    PORTB &= ~_BV(PORTB2); // set clock low
    clkcnt2 = CLKOPTS[clksel2];
  }
  --clkcnt2;
}

void reset_clocks() {
  clkcnt1 = 0;
  clkcnt2 = 0;
}

void process_midi_message(uint8_t m) {
  switch (m) {
  case MIDI_START:
    reset_clocks();
    break;
  case MIDI_TIMING_CLOCK:
    trigger_clocks();
    break;
  }
}

void update_encoder_state() {
  static uint8_t last_value;
  // read encoder pins value (AB)
  uint8_t value = (PINB & (_BV(PINB4) | _BV(PINB3))) >> PINB3;
  if (value == last_value) {
    return;
  }
  last_value = value;

  static uint8_t last_enc;
  last_enc = (last_enc << 2) | value;
  if (last_enc == 0b01001011) { // one turn clockwise
    if (*cursel < NOPTS-1) {
      ++*cursel;
    }
  } else if (last_enc == 0b10000111) { // one turn counter clockwise
    if (*cursel > 0) {
      --*cursel;
    }
  }
}

void update_switch_state() {
  static uint8_t last_value;
  uint8_t value = bit_is_set(PINB, PINB5);
  if (value == last_value) {
    return;
  }

  // if (last_value != 0 && value == 0) {
  //   reset_clocks();
  // }

  last_value = value;

  if (value == 0) {
    cursel = &clksel1;
  } else {
    cursel = &clksel2;
  }
}

/* UART

   The ATtiny microcontroller does provide a hardware USI which can be used to
   implement half-duplex UART. The idea is as follows. The start of a byte causes a
   pin change interrupt (on falling edge). Then we want to get into the middle of a
   start bit (using a timer) to enable the USI to start shifting in the data bits. The
   clock source for the USI is a timer in CTC mode. The timer delay is derived from
   the CPU clock speed and baud rate in such a way that compare match occurs in the
   middle of the next bit. When 8 bits have been shifted in the USI overflow interrupt
   occurs so we can read the USI shift register and enable the pin change interrupt
   ready for the next byte.

   ──────┐  ┆  ┌──┆──┐  ┆     ┆  ┌──┆─────┆──┐  ┆  ┌──┆─────┆─────┆───
         └──┆──┘  ┆  └──┆─────┆──┘  ┆     ┆  └──┆──┘  ┆     ┆     ┆
   ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
   │  1  │  0  │  1  │  0  │  0  │  1  │  1  │  0  │  1  │  1  │  1  │
   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
   │idle │start│ D1  │ D2  │ D3  │ D4  │ D5  │ D6  │ D7  │ D8  │stop │
   └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘

   See http://www.technoblogy.com/show?RPY=

   MIDI messages are made up of 8-bit bytes transmitted at 31 250 baud using 8-N-1
   asynchronous serial communication (8 data bits, no parity, 1 stop bit). So in our
   case of 8 MHz clock the duration of one bit is 8000000/31250=256 clock cycles. */
void setup_uart() {
  USICR = 0;            // disable USI
  GIFR = _BV(PCIF);     // clear pin change interrupt flag
  GIMSK |= _BV(PCIE);   // enable pin change interrupts
  PCMSK |= _BV(PCINT0); // enable pin 5 change interrupt
}

/* Wait for a start bit (falling edge) and setup timer to one-half of the bit time. */
ISR (PCINT0_vect) {
  if (!(PINB & _BV(PINB0))) {
    GIMSK &= ~_BV(PCIE);      // disabe pin change interrupts
    TCCR0A = _BV(WGM01);      // set clear timer on compare match (CTC) mode
    TCCR0B = _BV(CS01);       // set prescaler to /8
    TCNT0 = 0;                // reset counter
    OCR0A = 15;               // set delay to (15+1)*8=128 cycles
    TIFR |= _BV(OCF0A);       // clear output compare flag
    TIMSK |= _BV(OCIE0A);     // enable output compare match interrupt
  }
}

/* When start bit is detected setup timer to the one bit time and start receiving. */
ISR (TIMER0_COMPA_vect) {
  TIMSK &= ~_BV(OCIE0A);             // disable output compare match interrupt
  TCNT0 = 0;                         // reset counter
  OCR0A = 31;                        // set delay to (31+1)*8=256 cycles
  USICR = _BV(USIOIE) | _BV(USICS0); // enable USI overflow interrupt and select timer
                                     // compare match as a clock source
  USISR = _BV(USIOIF) | 8;           // clear USI overflow interrupt flag and set
                                     // counter to read one byte
}

uint8_t reverse_byte(uint8_t x) {
  x = ((x >> 1) & 0x55) | ((x << 1) & 0xAA);
  x = ((x >> 2) & 0x33) | ((x << 2) & 0xCC);
  x = ((x >> 4) & 0x0F) | ((x << 4) & 0xF0);
  return x;
}

/* Read byte after 8 bits have been shifted and start waiting for the next. */
ISR (USI_OVF_vect) {
  USICR = 0;                           // disable USI
  uint8_t value = reverse_byte(USIBR); // read value from the buffer (USI assumes MSB
                                       // first but UART sends bits LSB first)

  process_midi_message(value);

  GIFR = _BV(PCIF);   // clear pin change interrupt flag
  GIMSK |= _BV(PCIE); // enable pin change interrupts again
}

/* MAIN LOOP */
int main() {
  cli();
  setup_io();
  setup_clock_trigger_timer();
  reset_clocks();
  setup_uart();
  sei();

  for (;;) {
    update_switch_state();
    update_encoder_state();
    _delay_ms(1);
  }
}
