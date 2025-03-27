#include "includes/transducer_driver.h"

#define PDM_GPIO GPIO_NUM_2
#define SAMPLE_RATE 8000000
#define WAVE_FREQ 40000

static TaskHandle_t transducer_update_task;
static const char* TAG = "transducer_driver";

#define WAVE_SAMPLES (SAMPLE_RATE/WAVE_FREQ)
int8_t sine_wave[WAVE_SAMPLES];

static void generate_sine_wave() {
    // float dc_offset = 0.7;
    // float dc_offset_normalized = dc_offset / 3.3; // Assuming 3.3V supply
    // int8_t dc_offset_step = dc_offset_normalized * 256;
    // float new_signal_amplitude = 127.0f - (float)(dc_offset_step);

    for (int i = 0; i < WAVE_SAMPLES; i++) {
        sine_wave[i] = (int8_t)(127.0f * sinf((float)(2.0 * M_PI * i) / (float)WAVE_SAMPLES));
    }

    // bool high = false;
    // int half_period = (SAMPLE_RATE/WAVE_FREQ) / 2;
    // printf("Half period: %d\n", half_period);
    // for (int i = 0; i < WAVE_SAMPLES; i++) {
    //     if (i % half_period == 0)
    //         high = !high;
    //     sine_wave[i] = (int)(high);
    //     printf("%d: %d\n", i, sine_wave[i]);
    // }
}

static void transducer_value_update(void* arg) {
    for (int num_times = 0; num_times < 10; num_times++) {
        for (int i = 0; i < WAVE_SAMPLES; i++) {
            sigmadelta_set_duty(SIGMADELTA_CHANNEL_0, sine_wave[i]);
            vTaskDelay(pdMS_TO_TICKS(1000 / SAMPLE_RATE));
        }
    }
    sigmadelta_set_duty(SIGMADELTA_CHANNEL_0, -128);
    vTaskDelete(NULL);
}

esp_err_t transducer_driver_init() {
    generate_sine_wave();

    size_t prescale = 80000000 / SAMPLE_RATE;   // 80 MHz APP_clk -> PDM rate = 80 MHz / prescale
    sigmadelta_config_t pdm_config = {
        .channel = SIGMADELTA_CHANNEL_0,
        .sigmadelta_prescale = prescale,
        .sigmadelta_gpio = PDM_GPIO,
        .sigmadelta_duty = -128    // Initial
    };

    esp_err_t err = sigmadelta_config(&pdm_config);
    return err;
}

void transducer_start() {
    xTaskCreate(transducer_value_update, "waveform_task", 2048, NULL, 10, &transducer_update_task);
}

void transducer_stop() {
    vTaskDelete(transducer_update_task);
}
