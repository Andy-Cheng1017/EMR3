#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "UART.h"

#define main_TAG "EMR3"
#define UART_TAG "UART"

#define ECHO_TEST_TXD 0
#define ECHO_TEST_RXD 5
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
uint8_t Meter_net_totalizer[2] = {0X47, 0X65};
uint8_t Meter_gross_totalizer[2] = {0X47, 0X66};
uint8_t Gross_totalizer[2] = {0X47, 0X6A};
uint8_t Net_total_quantity_of_current_shift[2] = {0X47, 0X61};
uint8_t Gross_total_quantity_of_current_shift[2] = {0X47, 0X62};

uint8_t Write_Real_time_volume_t[] = {0x7E, 0x01, 0xFF, 0x53, 0x4B, 0, 0, 0, 0, 0, 0, 0X24, 0X40, 0XFE, 0X7E};

// struct meter_fields {
//   double Real_time_volume;
//   double Real_time_totalize;
// };

// struct meter_fields *meter_fields;

void EMR3_write_0() {
  // checksum = 0X100;
  // for (int i = 0; i < len; i++) {
  //   write_message[i + 3] = data[i];
  //   checksum += data[i];
  // }
  // // ESP_LOGI("main_TAG", "%d", checksum);
  // write_message[len + 3] = (~checksum + 1) & 0XFF;
  // write_message[len + 4] = 0x7E;
  uart_write_bytes(UART_PORT_NUM, Write_Real_time_volume_t, sizeof(Write_Real_time_volume_t));
  ESP_LOGI(UART_TAG, "write0");
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
  uart_read_bytes(UART_PORT_NUM, read_message, sizeof(read_message), 1000 / portTICK_PERIOD_MS);
  esp_log_buffer_hex("main_TAG", read_message, sizeof(read_message));
  vTaskDelay(500 / portTICK_PERIOD_MS);
  if (data[1] == 0X4C) {
    uint8_t Read[9];
    for (int i = 0; i < 9; i++) {
      Read[i] = read_message[i + 5];
    }
    ESP_LOGI(UART_TAG, "4C");
    return *(double *)Read;
  } else {
    uint8_t Read[8];
    for (int i = 0; i < 8; i++) {
      Read[i] = read_message[i + 5];
    }
    ESP_LOGI(UART_TAG, "4B");
    return *(double *)Read;
  }
}
void uart_task(void *arg) {
  // void uart_task() {
  uart_config_t uart_config = {
      .baud_rate = UART_BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };
  int intr_alloc_flags = 0;

  ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
  ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

  while (1) {
    ESP_LOGI("main_TAG", "volume: %f", EMR3_read(Real_time_volume_t, sizeof(Real_time_volume_t)));
    ESP_LOGI("main_TAG", "totalize: %f", EMR3_read(Real_time_totalize_t, sizeof(Real_time_totalize_t)));
    // ESP_LOGI(UART_TAG, "Meter_net_totalizer: %f", EMR3_read(Meter_net_totalizer, sizeof(Meter_net_totalizer)));
    // ESP_LOGI(UART_TAG, "Meter_gross_totalizer: %f", EMR3_read(Meter_gross_totalizer, sizeof(Meter_gross_totalizer)));
    // ESP_LOGI(UART_TAG, "Gross_totalizer: %f", EMR3_read(Gross_totalizer, sizeof(Gross_totalizer)));
    // ESP_LOGI(UART_TAG, "Net_total_quantity_of_current_shift: %f", EMR3_read(Net_total_quantity_of_current_shift,
    // sizeof(Net_total_quantity_of_current_shift))); ESP_LOGI(UART_TAG, "Gross_total_quantity_of_current_shift: %f",
    // EMR3_read(Gross_total_quantity_of_current_shift, sizeof(Gross_total_quantity_of_current_shift)));
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}
