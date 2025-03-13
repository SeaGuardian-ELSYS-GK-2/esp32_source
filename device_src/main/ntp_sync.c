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

esp_err_t ntp_sync_wifi_connect(uint8_t* wifi_ssid, uint8_t* wifi_pass) {
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
            .ssid = {*wifi_ssid},
            .password = {*wifi_pass},
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

    printf("Connecting to %s with password: %s\n", wifi_ssid, wifi_pass);


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.\nStarting connection with SSID:%s", wifi_ssid);

    // Waiting until either the connection is stablished (WIFI_CONNECTED BIT) or connection failed for the maximum
    // number of tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    // xEventGroupWaitBits() returns the bits before the call returened, hence we can test which event actually happened
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s, password:%s", wifi_ssid, wifi_pass);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID:%s, password:%s", wifi_ssid, wifi_pass);
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
    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update system time within 10s timeout");
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Got good NTP update");
    }

    // Takes some time before RTC is updated
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    // Get new time (mostly for debug)
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "Synced to NTP server. Time now: %s", asctime(&timeinfo));

    // Write to RTC? Not sure if necessery
    struct timeval tv = { .tv_sec = now, .tv_usec = 0 };
    settimeofday(&tv, NULL);

    return ESP_OK;
}
