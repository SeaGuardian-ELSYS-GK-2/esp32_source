#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_err.h"

/**
 * @brief Initialize the ADC1 channel 3 for water sensor.
 */
esp_err_t water_sensor_init();

/**
 * @brief Read data from ADC -> outputs voltage
 * @param data      Pointer data element where data will be populated
 */
esp_err_t water_sensor_read(int* data);

/**
 * @brie Free up ADC
 */
esp_err_t water_sensor_deinit();
