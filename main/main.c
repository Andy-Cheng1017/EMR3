#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "cJSON.h"

#include "UART.h"
#include "i2c-lcd1602.h"
#include "ethernet.h"

#define TASK_STACK_SIZE 1024
// void *arg;
void app_main(void) {
  initEth();
  xTaskCreatePinnedToCore(uart_task, "uart_task", TASK_STACK_SIZE * 2, NULL, 3, NULL, 0);
  vTaskDelay(3000/portTICK_RATE_MS);
  while (1) {
    if (!gpio_get_level(9)) {
      EMR3_write_0();
      vTaskDelay(200/portTICK_RATE_MS);
      while (!gpio_get_level(9)) vTaskDelay(100/portTICK_RATE_MS);
    }
    vTaskDelay(100/portTICK_RATE_MS);
  }
}