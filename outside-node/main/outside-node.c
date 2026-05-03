#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON   GPIO_NUM_23
#define SOLENOID GPIO_NUM_25

uint8_t inside_mac[] = {0x84, 0x1F, 0xE8, 0x07, 0xE1, 0xEC};

typedef struct {
    int cmd;
} msg_t;

static TaskHandle_t solenoid_task_handle = NULL;

static void send_cb(const uint8_t *mac, esp_now_send_status_t status)
{
    printf("ALARM SEND: %s\n",
           status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

static void recv_cb(const esp_now_recv_info_t *info,
                    const uint8_t *data, int len)
{
    if (len < (int)sizeof(msg_t)) return;
    msg_t *m = (msg_t *)data;
    if (m->cmd == 0) {
        printf("UNLOCK RECEIVED\n");
        xTaskNotifyGive(solenoid_task_handle);
    }
}

static void wifi_init(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    esp_wifi_init(&(wifi_init_config_t)WIFI_INIT_CONFIG_DEFAULT());
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_ps(WIFI_PS_NONE);

    vTaskDelay(500 / portTICK_PERIOD_MS);
}

static void espnow_init(void)
{
    esp_now_init();
    esp_now_register_send_cb(send_cb);
    esp_now_register_recv_cb(recv_cb);

    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    uint8_t ch; wifi_second_chan_t sec;
    esp_wifi_get_channel(&ch, &sec);
    printf("CHANNEL: %d\n", ch);

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, inside_mac, 6);
    peer.channel = 1;
    peer.ifidx   = ESP_IF_WIFI_STA;
    peer.encrypt = false;

    esp_err_t err = esp_now_add_peer(&peer);
    printf("PEER ADD: %s\n", esp_err_to_name(err));
}

void solenoid_task(void *p)
{
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printf("SOLENOID UNLOCKED\n");
        gpio_set_level(SOLENOID, 1);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        gpio_set_level(SOLENOID, 0);
        printf("SOLENOID LOCKED\n");
    }
}

void app_main(void)
{
    wifi_init();
    espnow_init();

    gpio_config_t sol = {
        .pin_bit_mask = (1ULL << SOLENOID),
        .mode         = GPIO_MODE_OUTPUT
    };
    gpio_config(&sol);
    gpio_set_level(SOLENOID, 0);  // make sure locked on boot

    gpio_config_t btn = {
        .pin_bit_mask = (1ULL << BUTTON),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&btn);

    xTaskCreate(solenoid_task, "sol", 2048, NULL, 5, &solenoid_task_handle);

    int last = 1;
    msg_t alarm = {.cmd = 1};

    while (1) {
        int state = gpio_get_level(BUTTON);

        if (state == 0 && last == 1) {
            esp_err_t res = esp_now_send(inside_mac,
                                         (uint8_t *)&alarm,
                                         sizeof(alarm));
            printf("ALARM SENT: %s\n", esp_err_to_name(res));
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }

        last = state;
        vTaskDelay(30 / portTICK_PERIOD_MS);
    }
}
