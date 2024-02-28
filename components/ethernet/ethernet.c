#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
// #include "cJSON.h"


#include "ethernet.h"

#define TAG "ethernet"

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  uint8_t mac_addr[6] = {0};
  /* we can get the ethernet driver handle from event data */
  esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

  switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
      esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
      ESP_LOGI(TAG, "Ethernet Link Up");
      ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
      break;
    case ETHERNET_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "Ethernet Link Down");
      break;
    case ETHERNET_EVENT_START:
      ESP_LOGI(TAG, "Ethernet Started");
      break;
    case ETHERNET_EVENT_STOP:
      ESP_LOGI(TAG, "Ethernet Stopped");
      break;
    default:
      break;
  }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  const esp_netif_ip_info_t *ip_info = &event->ip_info;

  ESP_LOGI(TAG, "Ethernet Got IP Address");
  ESP_LOGI(TAG, "~~~~~~~~~~~");
  ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
  ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
  ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
  ESP_LOGI(TAG, "~~~~~~~~~~~");
}

void initEth(void) {
  // Initialize TCP/IP network interface (should be called only once in application)
  ESP_ERROR_CHECK(esp_netif_init());
  // Create default event loop that running in background
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
  esp_netif_config_t cfg_spi = {.base = &esp_netif_config, .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
  esp_netif_t *eth_netif_spi[1] = {NULL};
  char if_key_str[10];
  char if_desc_str[10];
  char num_str[3];
  //   for (int i = 0; i < 1; i++) {
  itoa(0, num_str, 10);
  strcat(strcpy(if_key_str, "ETH_SPI_"), num_str);
  strcat(strcpy(if_desc_str, "eth"), num_str);
  esp_netif_config.if_key = if_key_str;
  esp_netif_config.if_desc = if_desc_str;
  esp_netif_config.route_prio = 30 - 0;
  eth_netif_spi[0] = esp_netif_new(&cfg_spi);
  //   }

  // Init MAC and PHY configs to default
  eth_mac_config_t mac_config_spi = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_config_spi = ETH_PHY_DEFAULT_CONFIG();

  // Install GPIO ISR handler to be able to service SPI Eth modlues interrupts
  gpio_install_isr_service(0);

  // Init SPI bus
  spi_device_handle_t spi_handle[1] = {NULL};
  spi_bus_config_t buscfg = {
      .miso_io_num = 3,
      .mosi_io_num = 10,
      .sclk_io_num = 7,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(1, &buscfg, SPI_DMA_CH_AUTO));

  typedef struct {
    uint8_t spi_cs_gpio;
    uint8_t int_gpio;
    int8_t phy_reset_gpio;
    uint8_t phy_addr;
  } spi_eth_module_config_t;
  
  // Init specific SPI Ethernet module configuration from Kconfig (CS GPIO, Interrupt GPIO, etc.)
  spi_eth_module_config_t spi_eth_module_config[1];
  spi_eth_module_config[0].spi_cs_gpio = 9;
  spi_eth_module_config[0].int_gpio = 8;
  spi_eth_module_config[0].phy_reset_gpio = 6;
  spi_eth_module_config[0].phy_addr = 1;

  // Configure SPI interface and Ethernet driver for specific SPI module
  esp_eth_mac_t *mac_spi[1];
  esp_eth_phy_t *phy_spi[1];
  esp_eth_handle_t eth_handle_spi[1] = {NULL};

  spi_device_interface_config_t devcfg = {.command_bits = 1, .address_bits = 7, .mode = 0, .clock_speed_hz = 25 * 1000 * 1000, .queue_size = 20};

  // Set SPI module Chip Select GPIO
  devcfg.spics_io_num = spi_eth_module_config[0].spi_cs_gpio;

  ESP_ERROR_CHECK(spi_bus_add_device(1, &devcfg, &spi_handle[0]));
  // dm9051 ethernet driver is based on spi driver
  eth_dm9051_config_t dm9051_config = ETH_DM9051_DEFAULT_CONFIG(spi_handle[0]);

  // Set remaining GPIO numbers and configuration used by the SPI module
  dm9051_config.int_gpio_num = spi_eth_module_config[0].int_gpio;
  phy_config_spi.phy_addr = spi_eth_module_config[0].phy_addr;
  phy_config_spi.reset_gpio_num = spi_eth_module_config[0].phy_reset_gpio;

  mac_spi[0] = esp_eth_mac_new_dm9051(&dm9051_config, &mac_config_spi);
  phy_spi[0] = esp_eth_phy_new_dm9051(&phy_config_spi);

  esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac_spi[0], phy_spi[0]);
  ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config_spi, &eth_handle_spi[0]));

  /* The SPI Ethernet module might not have a burned factory MAC address, we cat to set it manually.
 02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
  */
  ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle_spi[0], ETH_CMD_S_MAC_ADDR, (uint8_t[]){0x02, 0x00, 0x00, 0x12, 0x34, 0x56 + 0}));

  // attach Ethernet driver to TCP/IP stack
  ESP_ERROR_CHECK(esp_netif_attach(eth_netif_spi[0], esp_eth_new_netif_glue(eth_handle_spi[0])));

  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

  ESP_ERROR_CHECK(esp_eth_start(eth_handle_spi[0]));
  ESP_LOGI(TAG, "finish init ethernet");
}
