#include <avr/io.h>
#include <util/setbaud.h>
#include <util/delay.h>

#include <ctype.h>
#include <stdlib.h>

#define SELECT_DDR DDRD
#define SELECT PORTD
#define NONE   0b01110000
#define ADDR_L 0b00110100
#define ADDR_H 0b00111000
#define READ   0b00100000
#define WRITE  0b00010000

#define DATA_DDR DDRB
#define DATA_OUT PORTB
#define DATA_IN  PINB

int uart_putc(unsigned char c)
{
  while (!(UCSRA & (1<<UDRE)));
  UDR = c;
  return 0;
}

void uart_puts(char *s)
{
  while (*s)
  {
    uart_putc((unsigned char) *s);
    s++;
  }
}

unsigned char uart_getc()
{
  while (!(UCSRA & (1<<RXC)));
  return UDR;
}

unsigned int address = 0;

void print_nibble(unsigned char nibble)
{
  if (nibble<10)
  {
    uart_putc(nibble+'0');
  }
  else
  {
    uart_putc(nibble-10+'a');
  }
}

void print_hex(unsigned char byte)
{
  print_nibble((byte&0xF0)>>4);
  print_nibble( byte&0x0F    );
}

char input_hex_digit()
{
  char c;
  do
  {
    c = tolower(uart_getc());
  } while (!((c>='0' && c<='9') || ((c>='a' && c<='f'))));
  uart_putc(c);
  if(c>='0' && c<='9')
  {
    c=c-0x30;
  }
  else
  {
    switch(c)
    {
      case 'A': case 'a': c=10; break;
      case 'B': case 'b': c=11; break;
      case 'C': case 'c': c=12; break;
      case 'D': case 'd': c=13; break;
      case 'E': case 'e': c=14; break;
      case 'F': case 'f': c=15; break;
      default: c=0;
    }
  }
  return c;
}

unsigned char input_hex_byte()
{
  unsigned char upper = input_hex_digit();
  unsigned char lower = input_hex_digit();
  return (upper<<4) + lower;
}

unsigned int input_hex_word()
{
  unsigned char upper = input_hex_byte();
  unsigned char lower = input_hex_byte();
  return (upper<<8) + lower;
}

void set_address(int addr)
{
  DATA_DDR = 0xff;
  SELECT = ADDR_H;
//  _delay_us(1);
  DATA_OUT = addr >> 8;
//  _delay_us(1);
  SELECT = ADDR_L;
//  _delay_us(1);
  DATA_OUT = addr;
//  _delay_us(1);
  SELECT = NONE;
//  _delay_us(1);
  DATA_DDR = 0x00;
  DATA_OUT = 0xFF;
  address = addr;
}

void read_byte()
{
  DATA_DDR = 0x00;
  DATA_OUT = 0xFF;
  SELECT = READ;
  _delay_us(1);
  unsigned char byte = DATA_IN;
  SELECT = NONE;
  print_hex(byte);
  set_address(address+1);
}

void write_byte(unsigned char byte)
{
  DATA_DDR = 0xff;
  SELECT = WRITE;
  DATA_OUT = byte;
//  _delay_us(1);
  SELECT = NONE;
//  _delay_us(1);
  DATA_DDR = 0x00;
  DATA_OUT = 0xFF;
  set_address(address+1);
}


void get_address()
{
  print_hex(address>>8);
  print_hex(address);
}

void main()
{
  UBRRH = UBRRH_VALUE;
  UBRRL = UBRRL_VALUE;
#if USE_2X
  UCSRA = (1 << U2X);
#endif
  UCSRC = (1<<UCSZ1) | (1<<UCSZ0);
  UCSRB = (1 << TXEN) | (1 << RXEN);

  SELECT_DDR = 0xff;
  SELECT = NONE;

  set_address(0);

  while (1)
  {
    switch(tolower(uart_getc()))
    {
      case 'r':
        uart_putc('R');
        read_byte();
      break;

      case 'w':
        uart_putc('W');
        write_byte(input_hex_byte());
      break;

      case 'a':
        uart_putc('A');
        set_address(input_hex_word());
      break;

      case '?':
        uart_putc('A');
        get_address();
      break;
    }
  }
}
