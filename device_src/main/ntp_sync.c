#include "includes/ntp_sync.h"

// FreeRTOS event group to signal when we are connected
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care bout two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries
*/
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char* TAG = "device_wifi_ntp_sync";
static int s_retry_num = 0;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t ntp_sync_wifi_connect(char* wifi_ssid, char* wifi_pass) {
    // Create FreeRTOS event group
    s_wifi_event_group = xEventGroupCreate();

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Create network interface handle
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_any_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_ip));

    wifi_config_t wifi_config = {
        .sta = {
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };

    // Set ssid and password
    memcpy(wifi_config.sta.ssid, wifi_ssid, 32);
    memcpy(wifi_config.sta.password, wifi_pass, 64);

    printf("Connecting to %s with password: %s\n", wifi_config.sta.ssid, wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.\nStarting connection with SSID:%s", wifi_config.sta.ssid);

    // Waiting until either the connection is stablished (WIFI_CONNECTED BIT) or connection failed for the maximum
    // number of tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    // xEventGroupWaitBits() returns the bits before the call returened, hence we can test which event actually happened
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t ntp_sync(char* ntp_server_ip) {
    // Create config
    esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG(ntp_server_ip);
    esp_netif_sntp_init(&sntp_config);

    // Wait for NTP server response
    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(100000)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update system time within 100s timeout");
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Got good NTP update");
    }

    // Takes some time before RTC is updated
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

        // Get high-resolution time from ESP clock
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);

    // Debug log with microsecond precision
    ESP_LOGI(TAG, "Synced to NTP. Time now: %ld.%06ld", (long int)tv_now.tv_sec, (long int)tv_now.tv_usec);

    // // Ensure RTC is updated with full precision
    // settimeofday(&tv_now, NULL);

    return ESP_OK;
}

static int sync_test_gpio_pin = 0;
static void ntp_sync_test_proc(void* arg) {
    // Get milliseconds to next top of second
    struct timeval tv;
    gettimeofday(&tv, NULL);

    long long ms_until_next_second = 1000 - (tv.tv_usec / 1000);
    ESP_LOGI(TAG, "Ms until second: %lld", (long long)ms_until_next_second);

    // TickType_t xLastWakeTime = xTaskGetTickCount();
    // vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(ms_until_next_second));
    vTaskDelay(pdMS_TO_TICKS(ms_until_next_second));

    int output_val = 0;
    while (true) {
        output_val = !output_val;
        gpio_set_level(sync_test_gpio_pin, output_val);

        ESP_LOGI(TAG, "Setting signal: %s", output_val ? "High" : "Low");

        vTaskDelay(pdMS_TO_TICKS(10));

        // Get next time
        gettimeofday(&tv, NULL);
        ms_until_next_second = 1000 - (tv.tv_usec / 1000);
        vTaskDelay(pdMS_TO_TICKS(ms_until_next_second));
    }

    vTaskDelete(NULL);
}

esp_err_t ntp_sync_test(uint32_t GPIO_pin) {
    // Set GPIO pin
    sync_test_gpio_pin = GPIO_pin;

    // Initialize GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << GPIO_pin,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };
    gpio_config(&io_conf);

    // Create task
    return xTaskCreate(ntp_sync_test_proc, "ntp_sync_test_proc", 4096, NULL, 10, NULL);
}
