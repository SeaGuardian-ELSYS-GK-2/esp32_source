#include "includes/water_sensor_reader.h"

static const char* TAG = "water_sensor_reader";

static bool should_calibrate = false;
static adc_oneshot_unit_handle_t adc_handle;
static adc_oneshot_unit_init_cfg_t init_config;
adc_cali_handle_t adc_cali_handle;

static bool water_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t* out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated) {
        ESP_LOGI(TAG, "Calibration scheme version %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
            calibrated = true;
    }

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "Not supoprted or calibration failed");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void water_adc_calibration_deinit(adc_cali_handle_t handle) {
    ESP_LOGI(TAG, "Deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
}

esp_err_t water_sensor_init() {
    // init
    init_config = (adc_oneshot_unit_init_cfg_t){
        .unit_id = ADC_UNIT_1
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // config
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12             // TODO: Look at what attenuation is best
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_3, &config));

    // calibration
    adc_cali_handle = NULL;
    should_calibrate = water_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_3, ADC_ATTEN_DB_0, &adc_cali_handle);

    return ESP_OK;
}

esp_err_t water_sensor_read(int* data) {
    int raw_data;
    adc_oneshot_read(adc_handle, ADC_CHANNEL_3, &raw_data);
    ESP_LOGI(TAG, "ADC1 Channel 3 Raw data: %d", raw_data);

    if (should_calibrate) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw_data, data));
        ESP_LOGI(TAG, "ADC1 Channel 3 Calibrated Voltage: %d mV", *data);
    }

    return ESP_OK;
}

esp_err_t water_sensor_deinit() {
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_handle));
    if (should_calibrate) {
        water_adc_calibration_deinit(adc_cali_handle);
    }

    return ESP_OK;
}
