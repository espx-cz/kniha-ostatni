/*
https://github.com/UncleRus/esp-idf-lib/blob/master/components/i2cdev/i2cdev.c
https://github.com/UncleRus/esp-idf-lib/tree/master/components/scd4x
https://github.com/UncleRus/esp-idf-lib/blob/master/README.md
BSD-3-Clause License
https://esp-idf-lib.readthedocs.io/en/latest/index.html

see also:
https://github.com/khoek/esp-sensirion/blob/master/src/esp-sensirion/io.c

*/


#include "esp_err.h"
#include "scd4x.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TAG "MAIN"

static uint8_t crc8(const uint8_t *data, size_t count)
{
    uint8_t res = 0xff;

    for (size_t i = 0; i < count; ++i)
    {
        res ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            if (res & 0x80)
                res = (res << 1) ^ 0x31;
            else
                res = (res << 1);
        }
    }
    return res;
}

static inline uint16_t swap(uint16_t v)
{
    return (v << 8) | (v >> 8);
}

static esp_err_t send_cmd(i2c_master_dev_handle_t dev, uint16_t cmd, uint16_t *data, size_t words)
{
    uint8_t buf[2 + words * 3];
    // add command
    *(uint16_t *)buf = swap(cmd);
    if (data && words)
        // add arguments
        for (size_t i = 0; i < words; i++)
        {
            uint8_t *p = buf + 2 + i * 3;
            *(uint16_t *)p = swap(data[i]);
            *(p + 2) = crc8(p, 2);
        }

    ESP_LOGV(TAG, "Sending buffer:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf, sizeof(buf), ESP_LOG_VERBOSE);

    return i2c_master_transmit(dev, buf, sizeof(buf), -1);

    //return i2c_dev_write(dev, NULL, 0, buf, sizeof(buf));
}

static esp_err_t read_resp(i2c_master_dev_handle_t dev, uint16_t *data, size_t words)
{
    uint8_t buf[words * 3];
    CHECK(i2c_master_receive(dev, buf, sizeof(buf), -1));

    ESP_LOGV(TAG, "Received buffer:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf, sizeof(buf), ESP_LOG_VERBOSE);

    for (size_t i = 0; i < words; i++)
    {
        uint8_t *p = buf + i * 3;
        uint8_t crc = crc8(p, 2);
        if (crc != *(p + 2))
        {
            ESP_LOGE(TAG, "Invalid CRC 0x%02x, expected 0x%02x", crc, *(p + 2));
            return ESP_OK;
            return ESP_ERR_INVALID_CRC;
        }
        data[i] = swap(*(uint16_t *)p);
    }
    return ESP_OK;
}

static esp_err_t execute_cmd(i2c_master_dev_handle_t dev, uint16_t cmd, uint32_t timeout_ms,
        uint16_t *out_data, size_t out_words, uint16_t *in_data, size_t in_words)
{
    CHECK_ARG(dev);

    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev,send_cmd(dev, cmd, out_data, out_words));
    if (timeout_ms)
    {
        if (timeout_ms > 10)
            vTaskDelay(pdMS_TO_TICKS(timeout_ms));
        //else
            //ets_delay_us(timeout_ms * 1000);
    }
    if (in_data && in_words)
    I2C_DEV_CHECK(dev,read_resp(dev, in_data, in_words));
    I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

esp_err_t scd4x_read_measurement_ticks(i2c_master_dev_handle_t dev, uint16_t *co2, uint16_t *temperature, uint16_t *humidity)
{
    CHECK_ARG(co2 || temperature || humidity);

    uint16_t buf[3];
    CHECK(execute_cmd(dev, CMD_READ_MEASUREMENT, 1, NULL, 0, buf, 3));
    if (co2)
        *co2 = buf[0];
    if (temperature)
        *temperature = buf[1];
    if (humidity)
        *humidity = buf[2];

    return ESP_OK;
}

esp_err_t scd4x_read_measurement(i2c_master_dev_handle_t dev, uint16_t *co2, float *temperature, float *humidity)
{
    CHECK_ARG(co2 || temperature || humidity);
    uint16_t t_raw, h_raw;

    CHECK(scd4x_read_measurement_ticks(dev, co2, &t_raw, &h_raw));
    if (temperature)
        *temperature = (float)t_raw * 175.0f / 65536.0f - 45.0f;
    if (humidity)
        *humidity = (float)h_raw * 100.0f / 65536.0f;

    return ESP_OK;
}


esp_err_t scd4x_stop_periodic_measurement(i2c_master_dev_handle_t dev)
{
    return execute_cmd(dev, CMD_STOP_PERIODIC_MEASUREMENT, 500, NULL, 0, NULL, 0);
}

esp_err_t scd4x_get_serial_number(i2c_master_dev_handle_t dev, uint16_t *serial0, uint16_t *serial1, uint16_t *serial2)
{
    CHECK_ARG(serial0 && serial1 && serial2);

    uint16_t buf[3];
    CHECK(execute_cmd(dev, CMD_GET_SERIAL_NUMBER, 1, NULL, 0, buf, 3));
    *serial0 = buf[0];
    *serial1 = buf[1];
    *serial2 = buf[2];

    return ESP_OK;
}

esp_err_t scd4x_start_periodic_measurement(i2c_master_dev_handle_t dev)
{
    return execute_cmd(dev, CMD_START_PERIODIC_MEASUREMENT, 1, NULL, 0, NULL, 0);
}

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SCD4X_I2C_ADDR,
        .scl_speed_hz = 100000,
    };

esp_err_t scd4x_new_device(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, dev_handle));
    return i2c_dev_create_mutex(*dev_handle);
}


///// mutrexes

//variable for mutex
static SemaphoreHandle_t i2c_dev_mutex = NULL;

esp_err_t i2c_dev_create_mutex(i2c_master_dev_handle_t dev)
{
#if !CONFIG_I2CDEV_NOLOCK
    if (!dev) return ESP_ERR_INVALID_ARG;

    ESP_LOGV(TAG, "creating mutex");

    i2c_dev_mutex = xSemaphoreCreateMutex();
    if (!i2c_dev_mutex)
    {
        ESP_LOGE(TAG, "Could not create device mutex");
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}

esp_err_t i2c_dev_take_mutex(i2c_master_dev_handle_t dev)
{
#if !CONFIG_I2CDEV_NOLOCK
    if (!dev) return ESP_ERR_INVALID_ARG;

    ESP_LOGV(TAG, "taking mutex");

    if (!xSemaphoreTake(i2c_dev_mutex, pdMS_TO_TICKS(1000)))
    {
        ESP_LOGE(TAG, "Could not take device mutex");
        return ESP_ERR_TIMEOUT;
    }
#endif
    return ESP_OK;
}

esp_err_t i2c_dev_give_mutex(i2c_master_dev_handle_t dev)
{
#if !CONFIG_I2CDEV_NOLOCK
    if (!dev) return ESP_ERR_INVALID_ARG;

    ESP_LOGV(TAG, "giving mutex");

    if (!xSemaphoreGive(i2c_dev_mutex))
    {
        ESP_LOGE(TAG, "Could not give device mutex");
        return ESP_FAIL;
    }
#endif
    return ESP_OK;
}