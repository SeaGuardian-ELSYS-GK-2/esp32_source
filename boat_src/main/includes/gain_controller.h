#pragma once

#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_types.h"
#include "driver/i2c.h"

#define CONFIG_I2C_MASTER_SCL 0 // TODO
#define CONFIG_I2C_MASTER_SDA 1 // TODO

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define SENSOR_ADDR                 0x68        /*!< Slave address of the MPU9250 sensor */
#define WHO_AM_I_REG_ADDR           0x75        /*!< Register addresses of the "who am I" register */

#define GAIN_REGISTER_ADDR          0x6B        /*!< Register addresses of the power managment register */
#define RESET_BIT                   7

esp_err_t gain_init();
esp_err_t set_gain(uint8_t gain);
