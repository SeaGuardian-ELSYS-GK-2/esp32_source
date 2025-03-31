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
// #include "includes/ntp_sync.h"
#include "includes/transducer_driver.h"
// #include "includes/water_sensor_reader.h"

// Config
// #define SYNC_TIME_ENABLE
// #define SYNC_TEST_ENABLE
#define SEND_SIGNAL_ENABLE
// #define WATER_SENSOR_ENABLE

#define ESP_WIFI_SSID      "morten_iphone"
#define ESP_WIFI_PASS      "aPwIWF&24ot"

#define NTP_SYNC_TEST_GPIO 12

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

    esp_err_t err;

#ifdef SYNC_TIME_ENABLE
    // Sync clocks
    err = ntp_sync_wifi_connect((char*)ESP_WIFI_SSID, (char*)ESP_WIFI_PASS);
    if (err == ESP_FAIL)
        return;


    char* ntp_server = "172.20.10.4";
    err = ntp_sync(ntp_server);
    if (err == ESP_FAIL) {
        ESP_LOGE(TAG, "Failed to sync time");
        return;
    } else {
        ESP_LOGI(TAG, "Sucessfully synced time to NTP server: %s", ntp_server);
    }
#endif

#ifdef SYNC_TEST_ENABLE
    // Test NTP sync
    ESP_LOGI(TAG, "Starting NTP sync test...");
    ntp_sync_test(NTP_SYNC_TEST_GPIO);
#endif

#ifdef SEND_SIGNAL_ENABLE
    // Initialize Transduser Send
    err = transducer_driver_init();        
    if (err == ESP_FAIL) {
        ESP_LOGE(TAG, "Could not initialize transducer driver");
        return;
    }


    while (true) {
        ESP_LOGI(TAG, "Sending signal...");
        transducer_start();
        vTaskDelay(pdMS_TO_TICKS(100 * time_one_period));
        transducer_stop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#endif

#ifdef WATER_SENSOR_ENABLE
    water_sensor_init();

    int data;
    while (true) {
        water_sensor_read(&data);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    water_sensor_deinit();
#endif
}
