#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "driver/pwm.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "rom/ets_sys.h"
#include "nvs_flash.h"

#define GPIO_OUPUT_LED 12
#define GPIO_OUPUT_BUZZER 14
#define GPIO_INPUT_BUTTON 05

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

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_MODE_AP TRUE //TRUE:AP FALSE:STA
#define EXAMPLE_ESP_WIFI_SSID "MY_SUPER_WIFI"
#define EXAMPLE_ESP_WIFI_PASS ""
#define EXAMPLE_MAX_STA_CONN 1

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "simple wifi";

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR " join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR "leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_softap()
{
    wifi_event_group = xEventGroupCreate();

    printf("INIT WIFI");
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void wifi_init_sta()
{
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS},
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

/******************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void app_main(void)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();

    printf("SDK version:%s\n", esp_get_idf_version());

    gpio_config_t c;
    c.mode = GPIO_MODE_OUTPUT;
    c.pin_bit_mask = GPIO_Pin_12 | GPIO_Pin_14;
    c.intr_type = GPIO_INTR_DISABLE;
    c.pull_down_en = c.pull_up_en = 0;
    gpio_config(&c);

    gpio_config_t c_output;
    c_output.mode = GPIO_MODE_INPUT;
    c_output.pin_bit_mask = GPIO_Pin_4;
    c_output.intr_type = GPIO_INTR_DISABLE;
    c_output.pull_down_en = c.pull_up_en = 0;
    gpio_config(&c_output);

    int cnt = 0;

    pwm_init(PWM_PERIOD, duties, 1, pin_num);
    pwm_set_channel_invert(0);
    pwm_set_phases(phase);
    pwm_start();

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    int button_pressed = 0;
    while (1)
    {
        printf("cnt: %d\n", cnt++);

        button_pressed = gpio_get_level(GPIO_INPUT_BUTTON);
        printf("BUTTON %d\n", button_pressed);

        gpio_set_level(GPIO_OUPUT_LED, button_pressed % 2);
        if (button_pressed % 2 == 0)
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
