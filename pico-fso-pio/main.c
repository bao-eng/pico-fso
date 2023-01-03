#include <ctype.h>
#include <stdio.h>

#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "uart_tx.pio.h"

int main() {
  stdio_init_all();

  const uint PIN_TX = 25;
  const uint SERIAL_BAUD = 50;

  PIO pio = pio0;
  uint sm = 0;
  uint offset = pio_add_program(pio, &uart_tx_program);
  uart_tx_program_init(pio, sm, offset, PIN_TX, SERIAL_BAUD);

  while (true) {
    int16_t ch = getchar_timeout_us(100);
    while (ch != PICO_ERROR_TIMEOUT) {
      char chr = (char)ch;
      if (isprint(chr)) {
        printf("%c", chr);
        uart_tx_program_putc(pio, sm, chr);
      }
      ch = getchar_timeout_us(100);
    }
  }
}
