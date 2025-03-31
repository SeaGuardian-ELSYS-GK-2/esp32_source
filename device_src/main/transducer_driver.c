#include "includes/transducer_driver.h"

#include "driver/ledc.h"

esp_err_t transducer_driver_init() {
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = WAVE_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {
        .gpio_num = PDM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = PWM_DUTY,
        .hpoint = 0
    };
    
    ledc_channel_config(&channel_config);
    return ledc_fade_func_install(0);
}

void transducer_start() {
    printf("Sending singal for %f ms\n", 100 * time_one_period);
    ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, PWM_DUTY, PWM_DUTY);
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, PWM_DUTY, 100 * time_one_period);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT); 
}

void transducer_stop() {
    ledc_fade_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, PWM_DUTY);
}
