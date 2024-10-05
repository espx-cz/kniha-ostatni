#include <stdio.h>

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bmx280.h"
#include <string.h>

#define SDA_GPIO 8
#define SCL_GPIO 10


void app_main()

{
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

    bmx280_t* bmx280 = bmx280_create(I2C_NUM_0);

    bmx280_init(bmx280);

    bmx280_config_t bmx_cfg = BMX280_DEFAULT_CONFIG;

    bmx280_configure(bmx280, &bmx_cfg);

    float pressure, temperature, humidity;

    while (1)

    {

        bmx280_setMode(bmx280, BMX280_MODE_FORCE);

        do {
            vTaskDelay(pdMS_TO_TICKS(100));
        } while(bmx280_isSampling(bmx280));

        bmx280_readoutFloat(bmx280, &temperature, &pressure, &humidity);

        

            printf("%.2f Pa, %.2f C, %.2f %%\n", pressure, temperature, humidity);

        

    }

}