#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "UART.h"
#include "i2c-lcd1602.h" 

#define TASK_STACK_SIZE 2048

void app_main(void) {
  xTaskCreatePinnedToCore(uart_task, "uart_task", TASK_STACK_SIZE, NULL, 10, NULL, 1);
}