#include "freertos/FreeRTOS.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "nvs_flash.h"

const static char *TAG = "sniffer";

esp_err_t event_handler(void *ctx, system_event_t *event) {
  return ESP_OK;
}

typedef struct {
  unsigned version:2;
  unsigned type:2;
  unsigned subtype:4;
  unsigned to_ds:1;
  unsigned from_ds:1;
  unsigned :6;
  uint16_t duration_id;
  uint8_t addr1[6];
  uint8_t addr2[6];
  uint8_t addr3[6];
  uint16_t sequence_control;
  uint8_t addr4[6];
} wifi_ieee80211_pkt_t;

void print_mac(uint8_t addr[6]) {
  for(int i = 0; i < 6; i++) {
    printf("%02x", addr[i]);
    if(i < 5) printf(":");
  }
}

void wifi_ieee80211_pkt_cb(wifi_ieee80211_pkt_t *data) {
  ESP_LOGI(TAG, "wifi_ieee80211_pkt_cb to_ds=%d, from_ds=%d", data->to_ds,
      data->from_ds);

  if(
    data->to_ds == 0 &&
    data->from_ds == 0
  ) {
    // addr1 == dst
    // addr2 == src
    // addr3 == bssid
    // addr4 == ?

    printf("dst=");
    print_mac(data->addr1);
    printf("\n");

    printf("src=");
    print_mac(data->addr2);
    printf("\n");

    printf("bssid=");
    print_mac(data->addr3);
    printf("\n");
  } else if(
    data->to_ds == 0 &&
    data->from_ds == 1
  ) {
    // addr1 == dst
    // addr2 == bssid
    // addr3 == src
    // addr4 == ?

    printf("dst=");
    print_mac(data->addr1);
    printf("\n");

    printf("bssid=");
    print_mac(data->addr2);
    printf("\n");

    printf("src=");
    print_mac(data->addr3);
    printf("\n");
  } else if(
    data->to_ds == 1 &&
    data->from_ds == 0
  ) {
    // addr1 == bssid
    // addr2 == src
    // addr3 == dst
    // addr4 == ?

    printf("bssid=");
    print_mac(data->addr1);
    printf("\n");

    printf("src=");
    print_mac(data->addr2);
    printf("\n");

    printf("dst=");
    print_mac(data->addr3);
    printf("\n");
  } else if(
    data->to_ds == 1 &&
    data->from_ds == 1
  ) {
    // addr1 == dst?
    // addr2 == src?
    // addr3 == dst
    // addr4 == src

    printf("dst=");
    print_mac(data->addr3);
    printf("\n");

    printf("src=");
    print_mac(data->addr4);
    printf("\n");
  }
}

void wifi_pkt_ctrl_cb(wifi_pkt_rx_ctrl_t *data) {
  ESP_LOGI(TAG, "wifi_pkt_ctrl_cb");
}

void wifi_pkt_mgmt_cb(wifi_promiscuous_pkt_t *data) {
  ESP_LOGI(TAG, "wifi_pkt_mgmt_cb len=%d", data->rx_ctrl.sig_len);
  wifi_ieee80211_pkt_cb((wifi_ieee80211_pkt_t *) data->payload);
}

void wifi_pkt_data_cb(wifi_promiscuous_pkt_t *data) {
  ESP_LOGI(TAG, "wifi_pkt_data_cb len=%d", data->rx_ctrl.sig_len);
  wifi_ieee80211_pkt_cb((wifi_ieee80211_pkt_t *) data->payload);
}

void wifi_pkt_misc_cb(wifi_promiscuous_pkt_t *data) {
  ESP_LOGI(TAG, "wifi_pkt_misc_cb len=%d", data->rx_ctrl.sig_len);
  wifi_ieee80211_pkt_cb((wifi_ieee80211_pkt_t *) data->payload);
}

void wifi_promiscuous_cb(void *buf, wifi_promiscuous_pkt_type_t type) {;
  ESP_LOGI(TAG, "wifi_promiscuous_cb");

  switch (type) {
    case WIFI_PKT_CTRL:
      wifi_pkt_ctrl_cb((wifi_pkt_rx_ctrl_t *) buf);
      break;
    case WIFI_PKT_MGMT:
      wifi_pkt_mgmt_cb((wifi_promiscuous_pkt_t *) buf);
      break;
    case WIFI_PKT_DATA:
      wifi_pkt_data_cb((wifi_promiscuous_pkt_t *) buf);
      break;
    case WIFI_PKT_MISC:
      wifi_pkt_misc_cb((wifi_promiscuous_pkt_t *) buf);
      break;
  }
}

void app_main(void) {
  nvs_flash_init();
  tcpip_adapter_init();

  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_promiscuous_cb));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
}
