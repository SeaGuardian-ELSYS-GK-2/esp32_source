#include "includes/gain_controller.h"

static const char* TAG = "gain_controller";

/**
 * @brief Read a sequence of bytes from the sensor
 */
static esp_err_t register_read(uint8_t* data) {
    return i2c_master_read_from_device(I2C_MASTER_NUM, SENSOR_ADDR, data, 1, I2C_MASTER_TIMEOUT_MS/portTICK_PERIOD_MS);
}

/**
 * @brief Write a sequence of bytes to the sensor
 */
static esp_err_t register_write_byte(uint8_t data) {
    uint8_t send_data[1] = {data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, SENSOR_ADDR, send_data, 1, I2C_MASTER_TIMEOUT_MS/portTICK_PERIOD_MS);
}

esp_err_t gain_init(uint8_t sda_pin, uint8_t scl_pin) {
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

esp_err_t set_gain(uint8_t gain) {
    if (gain > 127) {
        ESP_LOGW(TAG, "Tried to set gain higher than max allowed value (127). Setting value to 127");
        gain = 127;
    }

    return register_write_byte(gain);
}

esp_err_t read_gain(uint8_t* gain) {
    return register_read(gain);
}
