#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


/* 8 io pins used (not contiguous since leds on d<0:1>):
   portb<0:1>
   portd<2:7>
 */


static void wait_for_long(void)
{
  uint16_t counter;
  for (counter = 0; counter != 200; ++counter) _delay_loop_2(30000);
}


static void wait_for_short(void)
{
  unsigned char counter;
  for (counter = 0; counter != 5; ++counter) _delay_loop_2(10000);
}


static void set_pin(uint8_t x)
{
  if (x) PORTB |= 1 << 1;
  else PORTB &= ~(1 << 1);
}


static void pulse_pin(void)
{
  set_pin(1);
  wait_for_short();
  set_pin(0);
}

static void uart_setup(void)
{
  /* #define CONFIG_FOSC (F_CPU * 2) */
  /* const uint16_t x = CONFIG_FOSC / (16 * BAUDS) - 1; */
#if 1 /* (bauds == 9600) */
  const uint16_t x = 206;
#elif 0 /* (bauds == 115200) */
  const uint16_t x = 16;
#elif 0 /* (bauds == 500000) */
  const uint16_t x = 3;
#elif 0 /* (bauds == 1000000) */
  const uint16_t x = 1;
#endif

  UBRR0H = (uint8_t)((x >> 8) & 0xff);
  UBRR0L = (uint8_t)((x >> 0) & 0xff);

  /* UCSR0B = (1 << RXEN0) | (1 << TXEN0); */
  UCSR0B = (1 << 4) | (1 << 3);

  /* 8n1 framing */
  UCSR0C = (3 << 1);
}


static void uart_write(void)
{
  uint8_t x = 0;

  for (; ; ++x)
  {
    /* wait for transmit buffer to be empty */
    while (!(UCSR0A & (1 << UDRE0))) ;
    UDR0 = x;
  }
}


#if 0
static void uart_sync(void)
{
  uint8_t n = 0;
  uint8_t x;
  while (n < 4)
  {
    if (UCSR0A & (1 << 7))
    {
      x = UDR0;
      if (x == 'x') ++n;
    }
  }
}
#endif


static volatile uint8_t has_changed = 0;
static volatile uint32_t start;
static volatile uint32_t stop;


static inline uint32_t read_counter(void)
{
  return 0;
}


ISR(PCINT0_vect)
{
  /* pin change 0 interrupt handler */

  /* TODO: check it comes from PB0 */
  if (PCIFR & (1 << 0))
  {
    stop = read_counter();

    /* int ack */
    PCIFR = 1;
  }
}


static void setup_counter(void)
{
  TCCR1A = 0;
}


int main(void)
{
#if 0
  uart_setup();
#endif

  /* digital output mode */
  DDRB = 0xff;
  PORTB = 0x00;

  while (1)
  {
    wait_for_long();
    pulse_pin();
    wait_for_short();
    pulse_pin();

#if 0
    has_changed = 0;
    start = read_counter();
    wait_for_short();
    pulse_pins(0 << 0);
    while (has_changed == 0) ;
    uart_write();
#endif
  }

  return 0;
}

