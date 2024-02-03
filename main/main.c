#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
// #include "esp_freertos_hooks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#define TASK_STACK_SIZE 2048
#define main_TAG "EMR3"
#define UART_TAG "UART"

#define ECHO_TEST_TXD 21
#define ECHO_TEST_RXD 20
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 9600

#define BUF_SIZE (1024)

uint8_t write_message[10] = {0x7E, 0x01, 0xFF};
uint8_t read_message[20];
uint32_t checksum = 0X100;

// enum Meter_Fields_t{
//   Net_total_quantity_of_current_shift_t = 'a',
//   Gross_total_quantity_of_current_shift_t = 'b',
//   Preset_volume_for_current_product_t = 'c',
// };

uint8_t Real_time_volume_t[2] = {0x47, 0x4B};
uint8_t Real_time_totalize_t[2] = {0x47, 0x4C};

uint8_t Write_Real_time_volume_t[10] = {0x53, 0x4B, 0, 0, 0, 0, 0, 0, 0, 0};

struct meter_fields {
  double Real_time_volume;
  double Real_time_totalize;
};

struct meter_fields *meter_fields;

void EMR3_write(uint8_t *data, uint8_t len) {
  checksum = 0X100;
  for (int i = 0; i < len; i++) {
    write_message[i + 3] = data[i];
    checksum += data[i];
  }
  // ESP_LOGI("main_TAG", "%d", checksum);
  write_message[len + 3] = (~checksum + 1) & 0XFF;
  write_message[len + 4] = 0x7E;
  uart_write_bytes(UART_PORT_NUM, write_message, len + 4);
}

double EMR3_read(uint8_t *data, uint8_t len) {
  checksum = 0X100;
  for (int i = 0; i < len; i++) {
    write_message[i + 3] = data[i];
    checksum += data[i];
  }
  // ESP_LOGI("main_TAG", "%d", checksum);
  write_message[len + 3] = (~checksum + 1) & 0XFF;
  write_message[len + 4] = 0x7E;
  uart_write_bytes(UART_PORT_NUM, (const char *)write_message, len + 5);
  memset(read_message, 0, sizeof(read_message));
  uart_read_bytes(UART_PORT_NUM, read_message, sizeof(read_message),
                  1000 / portTICK_PERIOD_MS);
  uint8_t Read[8];
  // for (int i = 0; i < 8; i++) {
  //   Read[7 - i] = read_message[i + 5];
  // }

  for (int i = 0; i < 8; i++) {
    Read[i] = read_message[i + 5];
  }
  esp_log_buffer_hex("main_TAG", Read, sizeof(Read));
  return *(double *)Read;
}

// void get_register(char Meter_Fields_t){
//   uint8_t data[10] = {0x7E, 0x03, 0x00, register_name};
//   EMR3_read(data, 4);
//   meter_fields->Net_total_quantity_of_current_shift = 0;
//   meter_fields->Gross_total_quantity_of_current_shift = 0;
//   meter_fields->Preset_volume_for_current_product = 0;
//   for (int i = 0; i < 4; i++) {
//     ESP_LOGI("main_TAG", "Recv str: %x", data[i]);
//   }

// }

void uart_task(void *arg) {
  /* Configure parameters of an UART driver,
   * communication pins and install the driver */
  uart_config_t uart_config = {
      .baud_rate = UART_BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      // .source_clk = UART_SCLK_DEFAULT,
  };
  int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
  intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

  ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL,
                                      intr_alloc_flags));
  ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD,
                               ECHO_TEST_RTS, ECHO_TEST_CTS));

  // Configure a temporary buffer for the incoming data
  // uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
  EMR3_write(Write_Real_time_volume_t, sizeof(Write_Real_time_volume_t));
  while (1) {
    // meter_fields->Net_total_quantity_of_current_shift =
    // EMR3_write(Write_Real_time_volume_t, sizeof(Write_Real_time_volume_t));
    // vTaskDelay(1000 / portTICK_PERIOD_MS);

    // EMR3_read(Net_total_quantity_of_current_shift_t,
    // sizeof(Net_total_quantity_of_current_shift_t));
    // EMR3_read(Net_total_quantity_of_current_shift_t,
    // sizeof(Net_total_quantity_of_current_shift_t));
    ESP_LOGI("main_TAG", "volume: %f",
             EMR3_read(Real_time_volume_t, sizeof(Real_time_volume_t)));
    // EMR3_read(Real_time_volume_t, sizeof(Real_time_volume_t));
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ESP_LOGI("main_TAG", "totalize: %f",
             EMR3_read(Real_time_totalize_t, sizeof(Real_time_totalize_t)));
    // EMR3_read(Real_time_totalize_t, sizeof(Real_time_totalize_t));
    // ESP_LOGI("main_TAG", "Hello world!");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    // Read data from the UART
    // int len = uart_read_bytes(UART_PORT_NUM, data, (BUF_SIZE - 1), 20 /
    // portTICK_PERIOD_MS);
    // // Write data back to the UART
    // uart_write_bytes(UART_PORT_NUM, (const char *)data, len);
    // if (len) {
    //   data[len] = '\0';
    //   ESP_LOGI(UART_TAG, "Recv str: %s", (char *)data);
    // }
  }
}

void app_main(void) {
  ESP_LOGI("main_TAG", "Hello world!");
  xTaskCreatePinnedToCore(uart_task, "uart_task", TASK_STACK_SIZE, NULL, 10,
                          NULL, 1);
}