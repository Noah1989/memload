#include <avr/io.h>
#include <util/setbaud.h>

int uart_putc(unsigned char c) {
    while (!(UCSRA & (1 << UDRE)));
    UDR = c;
    return 0;
}

void uart_puts(char *s) {
    while (*s) {
        uart_putc((unsigned char) *s);
        s++;
    }
}

int main(void) {

    UBRRH = UBRRH_VALUE;
    UBRRL = UBRRL_VALUE;
#if USE_2X
    UCSRA = (1 << U2X);
#endif
    UCSRB = (1 << TXEN) | (1 << RXEN);

    uart_puts("Hello,  World!\r\n");

    while (1) {

    }
}