#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/pwm.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#define GPIO_OUPUT_LED 12
#define GPIO_OUPUT_BUZZER 14

static const char *TAG = "pwm_example";

#define PWM_0_OUT_IO_NUM 14

// PWM period 500us(2Khz), same as depth
#define PWM_PERIOD 500

// pwm pin number
const uint32_t pin_num[] = {
    PWM_0_OUT_IO_NUM};

// dutys table, (duty/PERIOD)*depth
uint32_t duties[] = {
    250};

uint32_t duties_off[] = {
    0,
};

// phase table, (phase/180)*depth
int16_t phase[] = {
    0,
};

/******************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void app_main(void)
{
    printf("SDK version:%s\n", esp_get_idf_version());

    gpio_config_t c;
    c.mode = GPIO_MODE_OUTPUT;
    c.pin_bit_mask = GPIO_Pin_12 | GPIO_Pin_14;
    c.intr_type = GPIO_INTR_DISABLE;
    c.pull_down_en = c.pull_up_en = 0;
    gpio_config(&c);

    int cnt = 0;

    pwm_init(PWM_PERIOD, duties, 1, pin_num);
    pwm_set_channel_invert(0);
    pwm_set_phases(phase);
    pwm_start();

    while (1)
    {
        printf("cnt: %d\n", cnt++);
        gpio_set_level(GPIO_OUPUT_LED, cnt % 2);

        if (cnt % 2 == 0)
        {
            printf("buzzzzzz\n");
            pwm_set_duties(duties_off);
            pwm_start();
        }
        else
        {
            printf("pas buzzzzzz\n");
            pwm_set_duties(duties);
            pwm_start();
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
