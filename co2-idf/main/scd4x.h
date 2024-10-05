#include "driver/i2c_master.h"

#define SCD4X_I2C_ADDR 0x62

#define CMD_START_PERIODIC_MEASUREMENT             (0x21B1)
#define CMD_READ_MEASUREMENT                       (0xEC05)
#define CMD_STOP_PERIODIC_MEASUREMENT              (0x3F86)
#define CMD_SET_TEMPERATURE_OFFSET                 (0x241D)
#define CMD_GET_TEMPERATURE_OFFSET                 (0x2318)
#define CMD_SET_SENSOR_ALTITUDE                    (0x2427)
#define CMD_GET_SENSOR_ALTITUDE                    (0x2322)
#define CMD_SET_AMBIENT_PRESSURE                   (0xE000)
#define CMD_PERFORM_FORCED_RECALIBRATION           (0x362F)
#define CMD_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED (0x2416)
#define CMD_GET_AUTOMATIC_SELF_CALIBRATION_ENABLED (0x2313)
#define CMD_START_LOW_POWER_PERIODIC_MEASUREMENT   (0x21AC)
#define CMD_GET_DATA_READY_STATUS                  (0xE4B8)
#define CMD_PERSIST_SETTINGS                       (0x3615)
#define CMD_GET_SERIAL_NUMBER                      (0x3682)
#define CMD_PERFORM_SELF_TEST                      (0x3639)
#define CMD_PERFORM_FACTORY_RESET                  (0x3632)
#define CMD_REINIT                                 (0x3646)
#define CMD_MEASURE_SINGLE_SHOT                    (0x219D)
#define CMD_MEASURE_SINGLE_SHOT_RHT_ONLY           (0x2196)
#define CMD_POWER_DOWN                             (0x36E0)
#define CMD_WAKE_UP                                (0x36F6)

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

esp_err_t i2c_dev_create_mutex(i2c_master_dev_handle_t dev);
esp_err_t i2c_dev_take_mutex(i2c_master_dev_handle_t dev);
esp_err_t i2c_dev_give_mutex(i2c_master_dev_handle_t dev);

#define I2C_DEV_TAKE_MUTEX(dev) do { \
        esp_err_t __ = i2c_dev_take_mutex(dev); \
        if (__ != ESP_OK) return __;\
    } while (0)

#define I2C_DEV_GIVE_MUTEX(dev) do { \
        esp_err_t __ = i2c_dev_give_mutex(dev); \
        if (__ != ESP_OK) return __;\
    } while (0)

#define I2C_DEV_CHECK(dev, X) do { \
        esp_err_t ___ = X; \
        if (___ != ESP_OK) { \
            I2C_DEV_GIVE_MUTEX(dev); \
            return ___; \
        } \
    } while (0)


esp_err_t scd4x_read_measurement(i2c_master_dev_handle_t dev, uint16_t *co2, float *temperature, float *humidity);
esp_err_t scd4x_stop_periodic_measurement(i2c_master_dev_handle_t dev);
esp_err_t scd4x_get_serial_number(i2c_master_dev_handle_t dev, uint16_t *serial0, uint16_t *serial1, uint16_t *serial2);
esp_err_t scd4x_start_periodic_measurement(i2c_master_dev_handle_t dev);
esp_err_t scd4x_new_device(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t *dev_handle);
