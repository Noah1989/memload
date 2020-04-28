#include <avr/io.h>
#include <avr/cpufunc.h>
#include <util/setbaud.h>
#include <util/delay.h>

#include <ctype.h>
#include <stdlib.h>

// PORT D
// ======
// - = unused
// W = write strobe
// S = select:
//     AVR2313 -> 74AC138 -> Signal
//     ----------------------------------
//     000 (0) -> 000 (0) -> Latch CTRL
//     001 (1) -> 100 (4) -> BUSRQ
//     010 (2) -> 010 (2) -> Latch ADDR_H
//     011 (3) -> 110 (6) -> N/A
//     100 (4) -> 001 (1) -> Latch ADDR_L
//     101 (5) -> 101 (5) -> RESET
//     110 (6) -> 011 (3) -> Latch DATA
//     111 (7) -> 111 (7) -> N/A
// A = BUSACK (input)
//               -WSSSA--
#define IDLE   0b11111111
#define ADDR_L 0b11100111
#define ADDR_H 0b11010111
#define DATA   0b11110111
#define CTRL   0b11000111
#define BUSRQ  0b11001111
#define WSTRB  0b10001111
#define RESET  0b11101111
#define OUTS   0b00111000
#define OUTSW  0b01111000
#define BUSACK 0b00000100

// CTRL
// ====
// - = unused
// R = /RD
// W = /WR
// M = /MREQ
// I = /IORQ
//                  ----IMWR
#define READ_MEM  0b11111010
#define WRITE_MEM 0b11111001
#define READ_IO   0b11110110
#define WRITE_IO  0b11110101

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

unsigned char read_byte(unsigned int address)
{
  unsigned char result;
  DDRB  = 0xff;

  PORTB = READ_MEM;
  PORTD = CTRL;
  PORTD = IDLE;

  PORTB = address & 0xff;
  PORTD = ADDR_L;
  PORTD = IDLE;

  PORTB = (address>>8) & 0xff;
  PORTD = ADDR_H;
  PORTD = IDLE;

  PORTB = 0xff;
  DDRB  = 0x00;
  PORTD = BUSRQ;

  while (PIND & BUSACK);
  result = PINB;
  PORTD  = IDLE;

  return result;
}

void write_byte(unsigned int address, unsigned char byte)
{
  DDRB  = 0xff;

  PORTB = WRITE_MEM;
  PORTD = CTRL;
  PORTD = IDLE;

  PORTB = address & 0xff;
  PORTD = ADDR_L;
  PORTD = IDLE;

  PORTB = (address>>8) & 0xff;
  PORTD = ADDR_H;
  PORTD = IDLE;

  PORTB = byte;
  PORTD = DATA;
  PORTD = IDLE;

  PORTB = 0xff;
  DDRB  = 0x00;
  PORTD = BUSRQ;

  while (PIND & BUSACK);
  DDRD  = OUTSW;
  PORTD = WSTRB;
  PORTD = BUSRQ;
  DDRD  = OUTS;
  PORTD = IDLE;
}

void load_block(unsigned int start, unsigned int size)
{
  unsigned int i = size;
  unsigned int addr = start;
  unsigned char byte;
  // write
  while (i--)
  {
    byte = uart_getc();
    write_byte(addr, byte);
    addr++;
  }
  // verify
  i = size;
  addr = start;
  while (i--)
  {
    byte = read_byte(addr);
    uart_putc(byte);
    addr++;
  }
}

void main()
{
  unsigned int addr;
  unsigned int size;

  UBRRH = UBRRH_VALUE;
  UBRRL = UBRRL_VALUE;
#if USE_2X
  UCSRA = (1 << U2X);
#endif
  UCSRC = (1<<UCSZ1) | (1<<UCSZ0);
  UCSRB = (1<< TXEN) | (1<< RXEN);

  DDRD  = OUTS;
  PORTD = IDLE;
  DDRB  = 0x00; // all inputs
  PORTB = 0xFF; // pullups on

  while (1)
  {
    switch(tolower(uart_getc()))
    {
      case 'r':
        uart_putc('R');
        addr = input_hex_word();
        print_hex(read_byte(addr));
      break;

      case 'w':
        uart_putc('W');
        addr = input_hex_word();
        write_byte(addr, input_hex_byte());
      break;

      case 'b':
        uart_putc('B');
        addr = input_hex_word();
        size = input_hex_word();
        load_block(addr, size);
    }
  }
}
