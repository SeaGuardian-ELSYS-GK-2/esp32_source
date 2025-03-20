#pragma once

#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/sigmadelta.h"
#include "driver/dac_types_legacy.h"
#include "esp_log.h"
#include "esp_err.h"

esp_err_t transducer_driver_init();
void transducer_start();
void transducer_stop();
