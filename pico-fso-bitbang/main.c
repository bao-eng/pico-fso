#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/util/queue.h"

#define irda_false 0b0000000011100000
#define irda_true 0b0000000000000000

bool timer_callback(repeating_timer_t *rt);

queue_t sample_fifo;

const int FIFO_LENGTH = 32;
const uint led_pin = 25;
const uint ext_led_pin = 2;

int main() {
  // Initialize LED pin
  gpio_init(led_pin);
  gpio_set_dir(led_pin, GPIO_OUT);
  gpio_init(ext_led_pin);
  gpio_set_dir(ext_led_pin, GPIO_OUT);
  gpio_pull_down(ext_led_pin);

  // Initialize chosen serial port
  stdio_init_all();

  queue_init(&sample_fifo, sizeof(char), FIFO_LENGTH);

  repeating_timer_t timer;

  // negative timeout means exact delay (rather than delay between callbacks)
  int16_t baud =
      50;  // 50,75,110,134,150,200,300,600,1200,1800,2400,4800,9600,19200
  if (!add_repeating_timer_us(-(int64_t)round(1000000.0f / baud / 16),
                              timer_callback, NULL, &timer)) {
    printf("Failed to add timer\n");
    return 1;
  }
  // timer.delay_us=(int64_t)round(1000000 / 9600.0f / 16);
  // //https://github.com/raspberrypi/pico-sdk/issues/737

  // Loop forever
  printf(">");
  while (true) {
    int16_t ch = getchar_timeout_us(100);
    while (ch != PICO_ERROR_TIMEOUT) {
      char chr = (char)ch;
      if (isprint(chr)) {
        printf("%c", chr);
        if (!queue_try_add(&sample_fifo, &chr)) {
          printf("\nFIFO was full\n");
        }
      }
      ch = getchar_timeout_us(100);
    }
  }
}

bool timer_callback(repeating_timer_t *rt) {
  static int i = 0;
  static char element = 0;
  static int16_t frame_bitstream[10];
  if (i == 0) {
    if (!queue_try_remove(&sample_fifo, &element))
      return true;
    else {
      frame_bitstream[0] = irda_false;
      for (int8_t j = 0; j < 8; j++) {
        if ((element >> j) & 1U) {
          frame_bitstream[j + 1] = irda_true;
        } else {
          frame_bitstream[j + 1] = irda_false;
        }
      }
      frame_bitstream[9] = irda_true;
    }
  }
  // printf("i=%d, byte=%d, bit=%d\n", i, i/16, i%16);
  bool output = (frame_bitstream[i / 16] >> (i % 16)) & 1U;
  gpio_put(led_pin, output);
  gpio_put(ext_led_pin, output);
  // printf("%d", (frame_bitstream[i/16] >> (i%16)) & 1U);
  i++;
  if (i >= 10 * 16) i = 0;
  return true;  // keep repeating
}
