#include "includes/gain_controller.h"

/**
 * @brief Read a sequence of bytes from the sensor
 */
static esp_err_t register_read(uint8_t reg_addr, uint8_t* data, size_t len) {
    return i2c_master_write_read_device(I2C_MASTER_NUM, SENSOR_ADDR, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS/portTICK_PERIOD_MS);
}

/**
 * @brief Write a sequence of bytes to the sensor
 */
static esp_err_t register_write_byte(uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, SENSOR_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS/portTICK_PERIOD_MS);
}

esp_err_t gain_init() {
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

esp_err_t set_gain(uint8_t gain) {
    return register_write_byte(GAIN_REGISTER_ADDR, gain);
}
