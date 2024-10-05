#include <stdio.h>
#include "esp_mac.h"
#include "esp_err.h"
#include "esp_now.h"
#include "nvs_flash.h"

#include "msg.h"

#include <esp_wifi.h>

#include <string.h> //memcpy

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

struct_message myData;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  printf("\r\nLast Packet Send Status:\t%s\n",status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


uint8_t mac[6];
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    esp_wifi_stop();
	vTaskDelay(200 / portTICK_PERIOD_MS);

	esp_wifi_deinit();
	vTaskDelay(200 / portTICK_PERIOD_MS);

        ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    //esp_netif_t *sta_netif = 
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = 10,
            .threshold.authmode = WIFI_AUTH_OPEN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_now_init());
    esp_now_register_send_cb(OnDataSent);
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));

    while(1) {
        strcpy(myData.a, "THIS IS A CHAR");
        myData.b = random();
        myData.c = 1.2;
        myData.d = false;
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
        if (result == ESP_OK) {
            printf("Sent with success\n");
        }
        else {
            printf("Error sending the data\n");
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);

    }

}
