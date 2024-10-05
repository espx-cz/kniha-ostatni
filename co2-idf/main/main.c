#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_log.h"
#include "esp_err.h"
#include "scd4x.h"

#define TAG "MAIN"

//c3 mini

//#define SDA_GPIO 8
//#define SCL_GPIO 10

//c3 meteokit
#define SDA_GPIO 19
#define SCL_GPIO 18







void app_main(void)
{

i2c_master_bus_config_t i2c_cfg = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = -1,
    .sda_io_num = SDA_GPIO,
    .scl_io_num = SCL_GPIO,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
    //    .sda_pullup_en = false,
    //    .scl_pullup_en = false,


    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_cfg, &bus_handle));



    i2c_master_dev_handle_t dev_handle;


    ESP_ERROR_CHECK(scd4x_new_device(bus_handle, &dev_handle));

    ESP_ERROR_CHECK(scd4x_stop_periodic_measurement(dev_handle));

    uint16_t buf[3];
    ESP_ERROR_CHECK(scd4x_get_serial_number(dev_handle, &buf[0], &buf[1], &buf[2]));
    ESP_LOGI(TAG, "Sensor serial number: 0x%04x%04x%04x", buf[0], buf[1], buf[2]);

    ESP_ERROR_CHECK(scd4x_start_periodic_measurement(dev_handle));

    ESP_LOGI(TAG, "Waiting for data ready...");

    uint16_t co2;
    float temperature, humidity;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_err_t res = scd4x_read_measurement(dev_handle, &co2, &temperature, &humidity);
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG, "Error reading results %d (%s)", res, esp_err_to_name(res));
            continue;
        }

        if (co2 == 0)
        {
            ESP_LOGW(TAG, "Invalid sample detected, skipping");
            continue;
        }

        ESP_LOGI(TAG, "CO2: %u ppm", co2);
        ESP_LOGI(TAG, "Temperature: %.2f °C", temperature);
        ESP_LOGI(TAG, "Humidity: %.2f %%", humidity);
    }


}
