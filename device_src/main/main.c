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

#define ESP_WIFI_SSID      "morten_iphone"
#define ESP_WIFI_PASS      "aPwIWF&24ot"

static const char* TAG = "main";

void app_main(void)
{
    printf("Startup...\n");

    // Initialize NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_err_t err = ntp_sync_wifi_connect((uint8_t*)ESP_WIFI_SSID, (uint8_t*)ESP_WIFI_PASS);
    if (err == ESP_FAIL)
        return;


    err = ntp_sync("pool.ntp.org");
    if (err == ESP_FAIL)
        ESP_LOGI(TAG, "Failed to sync time");
    else
        ESP_LOGI(TAG, "Great success!");
}
