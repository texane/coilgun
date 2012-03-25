#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


/* measure the time elapsed between firing and detection.

   we assume the clock frequency is 16mhz and we have a 16
   bits counter incremented once per tick

   the distance resolution (ie. meter per bit) is:
   v_proj * (1 / 16000000)

   the max measurable distance before overflow is:
   65535 * (v_proj * (1 / 16000000))
 */


/* commands:
   'f': fire
 */

/* uart */

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

static void uart_write(uint8_t* s, uint8_t n)
{
  for (; n; --n, ++s)
  {
    /* wait for transmit buffer to be empty */
    while (!(UCSR0A & (1 << UDRE0))) ;
    UDR0 = *s;
  }
}

static inline uint8_t nibble(uint32_t x, uint8_t i)
{
  return (x >> (i * 4)) & 0xf;
}

static inline uint8_t hex(uint8_t x)
{
  return (x >= 0xa) ? 'a' + x - 0xa : '0' + x;
}

static uint8_t* uint32_to_string(uint32_t x)
{
  static uint8_t buf[8];

  buf[7] = hex(nibble(x, 0));
  buf[6] = hex(nibble(x, 1));
  buf[5] = hex(nibble(x, 2));
  buf[4] = hex(nibble(x, 3));
  buf[3] = hex(nibble(x, 4));
  buf[2] = hex(nibble(x, 5));
  buf[1] = hex(nibble(x, 6));
  buf[0] = hex(nibble(x, 7));

  return buf;
}

static inline uint8_t uart_read_byte(void)
{
  while ((UCSR0A & (1 << 7)) == 0) ;
  return UDR0;
}


#if 0 /* unused */
static void wait_for_long(void)
{
  uint16_t counter;
  for (counter = 0; counter != 200; ++counter) _delay_loop_2(30000);
}
#endif /* unused */


static inline void pulse_pin(void)
{
  PORTB |= 1 << 1;
  _delay_loop_2(50000);
  PORTB &= ~(1 << 1);
}


static volatile uint8_t is_done;
static volatile uint16_t low_part;
static volatile uint16_t high_part;
static volatile uint8_t timeout;


ISR(PCINT0_vect)
{
  /* pin change 0 interrupt handler */

  /* TODO: check it comes from PB0 */
  if (PCIFR & (1 << 0))
  {
    /* stop the counters */
    TCCR1B &= ~(1 << 0);
    TCCR2B &= ~(1 << 0);

    /* read the counter */
    low_part = TCNT1;

    is_done = 1;

    /* int ack */
    PCIFR = 1;
  }
}


ISR(TIMER1_OVF_vect)
{
  /* timer1 overflow interrupt */
  /* note: overflow flag automatically cleared */
  ++high_part;
}


ISR(TIMER2_OVF_vect)
{
  /* note: overflow flag automatically cleared */

  if (--timeout == 0)
  {
    /* stop the counters */
    TCCR1B &= ~(1 << 0);
    TCCR2B &= ~(1 << 0);

    /* invalidate the counter */
    low_part = (uint16_t)-1;
    high_part = (uint16_t)-1;
    is_done = 1;
  }
}


int main(void)
{
  uint32_t counter;
  uint8_t cmd;

  uart_setup();

  /* setup interrupt on change for b0 */
  DDRB &= 0xfe;
  /* enable pullup */
  PORTB = 1;

#if 0
  /* setup b1 as digital output */
  DDRB |= 1 << 1;
  PORTB &= ~(1 << 0);
#endif

  /* enable interrupt on port change 0 */
  PCICR = 1 << 0;
  PCMSK0 = 1 << 0;

  /* setup timer1, normal mode, interrupt on overflow */
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 = 1;

  /* setup timer2, interrupt on overflow */
  TCCR2A = 0;
  TCCR2B = 0;
  TIMSK2 = 1;

  /* global interrupt enable (SREG, i bit) */
  sei();

  while (1)
  {
    cmd = uart_read_byte();
    if (cmd != 'f') continue ;

    /* reset the waiting flag */
    is_done = 0;

    /* reset counters */
    timeout = 122;
    low_part = 0;
    high_part = 0;

    /* reload and start counters */
    TCNT1 = 0;
    TCNT2 = 0;

    /* (1 / (16000000 / 1024)) * 122 = 2 seconds timeout */
    TCCR2B = 7 << 0;
    TCCR1B = 1 << 0;

    /* fire */
    pulse_pin();

    /* wait object detection */
    while (is_done == 0) ;

    counter = ((uint32_t)high_part << 16) | (uint32_t)low_part;
    uart_write(uint32_to_string(counter), 8);
  }

  return 0;
}

