#include <stdio.h>

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bmx280.h"
#include <string.h>
#include "wifiConnect.h"
#include "mqtt_client.h"
#include "esp_sleep.h"

//--------------------------------------------------
extern const char cert_pem_start[] asm("_binary_cert_pem_start");
extern const char cert_pem_end[]   asm("_binary_cert_pem_end");
//--------------------------------------------------

#define TAG "MAIN"

//#define SDA_GPIO 8
//#define SCL_GPIO 10

#define SDA_GPIO 19
#define SCL_GPIO 18

bmx280_t* bme280;

void bme_init() {
    i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_GPIO,
        .scl_io_num = SCL_GPIO,
        .sda_pullup_en = false,
        .scl_pullup_en = false,

        .master = {
            .clk_speed = 100000
        }
    };

    i2c_param_config(I2C_NUM_0, &i2c_cfg);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);

    bme280 = bmx280_create(I2C_NUM_0);

    bmx280_init(bme280);

    bmx280_config_t bmx_cfg = BMX280_DEFAULT_CONFIG;

    bmx280_configure(bme280, &bmx_cfg);


}

#define MQTT_SERVER "k83ff1f3.ala.eu-central-1.emqxsl.com"
#define MQTT_PORT "8883"
#define MQTT_USERNAME CONFIG_ESP_MQTT_USERNAME
#define MQTT_PW CONFIG_ESP_MQTT_PW
#define MQTT_URI "mqtts://" MQTT_SERVER ":" MQTT_PORT
#define BME_ID "myhome"

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}
esp_mqtt_client_handle_t client;
static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_URI,
            .verification.certificate = (const char *)cert_pem_start,

        },
        .credentials = {
            .username = MQTT_USERNAME,
            .authentication.password = MQTT_PW,
        },
    };

    ESP_LOGI(TAG, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}


void app_main()

{

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    connect_wifi();

    mqtt_app_start();

    bme_init();
    float pressure, temperature, humidity;
    char payload[100];

    const int wakeup_time_sec = 20;


    while (1) {
        bmx280_setMode(bme280, BMX280_MODE_FORCE);

        do {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        } while(bmx280_isSampling(bme280));

        bmx280_readoutFloat(bme280, &temperature, &pressure, &humidity);

        printf("%.2f Pa, %.2f C, %.2f %%\n", pressure, temperature, humidity);
        //sprintf(payload, "{\"temperature\": %.2f, \"pressure\": %.2f, \"humidity\": %.2f}", temperature, pressure, humidity);
        //esp_mqtt_client_publish(client, "bme/temp", payload, 0, 0, 0);

        sprintf(payload, "%.2f", temperature);
        esp_mqtt_client_publish(client, "bme/" BME_ID "/temperature", payload, 0, 0, 0);

        sprintf(payload, "%.2f", pressure);
        esp_mqtt_client_publish(client, "bme/" BME_ID "/pressure", payload, 0, 0, 0);

        sprintf(payload, "%.2f", humidity);
        esp_mqtt_client_publish(client, "bme/" BME_ID "/humidity", payload, 0, 0, 0);

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));

        printf("Going to sleep now\n");
        esp_deep_sleep_start();
    }
}