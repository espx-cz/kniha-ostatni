#include <stdio.h>
#include <stdlib.h>
#include <string.h> //Requires by memset
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include <esp_http_server.h>
#include "esp_spiffs.h"
#include "driver/gpio.h"
#include "rom/gpio.h"

#include "connect_wifi.h"

#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"


#define LED_PIN 21
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      21

#define EXAMPLE_LED_NUMBERS         1
#define EXAMPLE_CHASE_SPEED_MS      10

static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

    rmt_channel_handle_t led_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
rmt_encoder_handle_t led_encoder = NULL;
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };

static const char *TAG = "espressif"; // TAG for debug
int led_state = 0;

#define INDEX_HTML_PATH "/spiffs/index.html"
char index_html[4096];
char response_data[4096];

static void initi_web_page_buffer(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    memset((void *)index_html, 0, sizeof(index_html));
    struct stat st;
    if (stat(INDEX_HTML_PATH, &st))
    {
        ESP_LOGE(TAG, "index.html not found");
        return;
    }

    FILE *fp = fopen(INDEX_HTML_PATH, "r");
    if (fread(index_html, st.st_size, 1, fp) == 0)
    {
        ESP_LOGE(TAG, "fread failed");
    }
    fclose(fp);
}

esp_err_t send_web_page(httpd_req_t *req)
{
    int response;
    if(led_state)
    {
        sprintf(response_data, index_html, "ON");
    }
    else
    {
        sprintf(response_data, index_html, "OFF");
    }
    response = httpd_resp_send(req, response_data, HTTPD_RESP_USE_STRLEN);
    return response;
}
esp_err_t get_req_handler(httpd_req_t *req)
{
    return send_web_page(req);
}

esp_err_t led_on_handler(httpd_req_t *req)
{
    led_strip_pixels[0] = 0xFF;
    led_strip_pixels[1] = 0xFF;
    led_strip_pixels[2] = 0xff;
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

    led_state = 1;
    return send_web_page(req);
}


esp_err_t led_r_handler(httpd_req_t *req)
{
    led_strip_pixels[0] = 0x00;
    led_strip_pixels[1] = 0xFF;
    led_strip_pixels[2] = 0x00;
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

    led_state = 1;
    return send_web_page(req);
}

esp_err_t led_g_handler(httpd_req_t *req)
{
    led_strip_pixels[0] = 0xFF;
    led_strip_pixels[1] = 0x00;
    led_strip_pixels[2] = 0x00;
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

    led_state = 1;
    return send_web_page(req);
}

esp_err_t led_b_handler(httpd_req_t *req)
{
    led_strip_pixels[0] = 0x00;
    led_strip_pixels[1] = 0x00;
    led_strip_pixels[2] = 0xFF;
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

    led_state = 1;
    return send_web_page(req);
}


esp_err_t led_off_handler(httpd_req_t *req)
{
    led_strip_pixels[0] = 0x00;
    led_strip_pixels[1] = 0x00;
    led_strip_pixels[2] = 0x00;
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

    led_state = 0;
    return send_web_page(req);
}

httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_req_handler,
    .user_ctx = NULL};

httpd_uri_t uri_on = {
    .uri = "/led2on",
    .method = HTTP_GET,
    .handler = led_on_handler,
    .user_ctx = NULL};

httpd_uri_t uri_off = {
    .uri = "/led2off",
    .method = HTTP_GET,
    .handler = led_off_handler,
    .user_ctx = NULL};

httpd_uri_t uri_r = {
    .uri = "/led2r",
    .method = HTTP_GET,
    .handler = led_r_handler,
    .user_ctx = NULL};
httpd_uri_t uri_g = {
    .uri = "/led2g",
    .method = HTTP_GET,
    .handler = led_g_handler,
    .user_ctx = NULL};
httpd_uri_t uri_b = {
    .uri = "/led2b",
    .method = HTTP_GET,
    .handler = led_b_handler,
    .user_ctx = NULL};


httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_on);
        httpd_register_uri_handler(server, &uri_off);
        httpd_register_uri_handler(server, &uri_r);
        httpd_register_uri_handler(server, &uri_g);
        httpd_register_uri_handler(server, &uri_b);
    }

    return server;
}

void app_main()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    connect_wifi();
    // GPIO initialization

    ESP_LOGI(TAG, "Create RMT TX channel");
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");
    
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    ESP_LOGI(TAG, "Start LED rainbow chase");


    led_strip_pixels[0] = 0x00;
    led_strip_pixels[1] = 0x00;
    led_strip_pixels[2] = 0x80;
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

    if (wifi_connect_status)
    {
        //gpio_pad_select_gpio(LED_PIN);
        //gpio_config()
        //gpio_reset_pin(LED_PIN);
        //gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
        //gpio_set_level(LED_PIN, 1);

        led_state = 1;
        ESP_LOGI(TAG, "LED Control SPIFFS Web Server is running ... ...\n");
        initi_web_page_buffer();
        setup_server();
    }
}