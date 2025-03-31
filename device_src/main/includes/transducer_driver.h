#pragma once

#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/dac_types_legacy.h"
#include "esp_log.h"
#include "esp_err.h"

#define PDM_GPIO GPIO_NUM_2
#define WAVE_FREQ 400      // 40 kHz square wave
#define PWM_RESOLUTION LEDC_TIMER_10_BIT // 10-bit resolution
#define PWM_DUTY (1 << 9) // 50% duty cycle (512/1024)
#define time_one_period (1.0f/WAVE_FREQ) * 1000

esp_err_t transducer_driver_init();
void transducer_start();
void transducer_stop();
