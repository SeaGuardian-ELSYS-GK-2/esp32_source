/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "includes/ntp_sync.h"
#include "includes/gain_controller.h"

// Config
#define SYNC_TIME_ENABLE false
#define SET_GAIN_ENABLE  true

#define ESP_WIFI_SSID      "morten_iphone"
#define ESP_WIFI_PASS      "aPwIWF&24ot"

#define CONFIG_I2C_MASTER_SCL 8
#define CONFIG_I2C_MASTER_SDA 9

static const char* TAG = "main";

void app_main(void)
{
    printf("Startup...\n");
    esp_err_t err;

    // Initialize NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Sync clocks
    if (SYNC_TIME_ENABLE) {
        err = ntp_sync_wifi_connect((char*)ESP_WIFI_SSID, (char*)ESP_WIFI_PASS);
        if (err == ESP_FAIL)
            return;
        
        char* ntp_server = "pool.ntp.org";
        err = ntp_sync(ntp_server);
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to sync time");
            return;
        } else {
            ESP_LOGI(TAG, "Sucessfully synced time to NTP server: %s", ntp_server);
        }
    }

    // Set gain
    if (SET_GAIN_ENABLE) {
        err = gain_init(CONFIG_I2C_MASTER_SDA, CONFIG_I2C_MASTER_SCL);
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to initialize gain I2C bus");
            return;
        } else {
            ESP_LOGI(TAG, "Successfully initialized gain I2C bus");
        }

        uint8_t gain = 1;
        set_gain(gain);

        // while (true) {
        //     gain++;
        //     set_gain(gain);
        //     vTaskDelay(20 / portTICK_PERIOD_MS);
        //
        //     if (gain >= 127)
        //         gain = 0;
        //
        //     ESP_LOGI(TAG, "Gain: %d\n", gain);
        // }
    }
}
