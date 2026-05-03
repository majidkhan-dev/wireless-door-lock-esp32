#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_i2c.h"

#define BUZZER  GPIO_NUM_27
#define BUTTON  GPIO_NUM_26

#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

uint8_t outside_mac[] = {0x84, 0x1F, 0xE8, 0x45, 0xE4, 0xF0};

typedef struct {
    int cmd;
} msg_t;

volatile int alarm_on  = 0;
volatile int countdown = 0;

static void i2c_init(void)
{
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_SDA,
        .scl_io_num       = I2C_SCL,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

static void send_cb(const uint8_t *mac, esp_now_send_status_t status)
{
    printf("UNLOCK SEND: %s\n",
           status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

static void recv_cb(const esp_now_recv_info_t *info,
                    const uint8_t *data, int len)
{
    if (len < (int)sizeof(msg_t)) return;
    msg_t *m = (msg_t *)data;
    if (m->cmd == 1) {
        printf("ALARM RECEIVED\n");
        alarm_on = 1;
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
    memcpy(peer.peer_addr, outside_mac, 6);
    peer.channel = 1;
    peer.ifidx   = ESP_IF_WIFI_STA;
    peer.encrypt = false;

    esp_err_t err = esp_now_add_peer(&peer);
    printf("PEER ADD: %s\n", esp_err_to_name(err));
}

void buzzer_task(void *p)
{
    while (1) {
        if (alarm_on) {
            gpio_set_level(BUZZER, 1);
            vTaskDelay(200 / portTICK_PERIOD_MS);
            gpio_set_level(BUZZER, 0);
            vTaskDelay(200 / portTICK_PERIOD_MS);
        } else {
            gpio_set_level(BUZZER, 0);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}

void lcd_task(void *p)
{
    lcd_init();

    TickType_t last_dec  = 0;
    int        prev_cd   = 0;

    while (1) {
        TickType_t now = xTaskGetTickCount();

        // reset timer when countdown is freshly set
        if (countdown > 0 && prev_cd == 0)
            last_dec = now;

        // decrement once per second
        if (countdown > 0 && (now - last_dec) >= pdMS_TO_TICKS(1000)) {
            countdown--;
            last_dec = now;
        }

        prev_cd = countdown;

        // line 1 — visitor status
        lcd_set_cursor(0, 0);
        if (alarm_on)
            lcd_print("    VISITOR!    ");
        else
            lcd_print("  DOOR SYSTEM   ");

        // line 2 — lock status
        lcd_set_cursor(1, 0);
        if (countdown > 0)
            lcd_print("  DOOR UNLOCKED ");
        else
            lcd_print("  DOOR LOCKED   ");

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    i2c_init();
    wifi_init();
    espnow_init();

    gpio_config_t buz = {
        .pin_bit_mask = (1ULL << BUZZER),
        .mode         = GPIO_MODE_OUTPUT
    };
    gpio_config(&buz);

    gpio_config_t btn = {
        .pin_bit_mask  = (1ULL << BUTTON),
        .mode          = GPIO_MODE_INPUT,
        .pull_up_en    = GPIO_PULLUP_ENABLE,
        .pull_down_en  = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&btn);

    xTaskCreate(buzzer_task, "buzz", 2048, NULL, 5, NULL);
    xTaskCreate(lcd_task,    "lcd",  4096, NULL, 4, NULL);

    int last = 1;
    msg_t unlock = {.cmd = 0};

    while (1) {
        int state = gpio_get_level(BUTTON);

        if (state == 0 && last == 1) {
            alarm_on  = 0;
            countdown = 5;
            printf("ALARM STOPPED\n");
            esp_err_t res = esp_now_send(outside_mac,
                                         (uint8_t *)&unlock,
                                         sizeof(unlock));
            printf("UNLOCK SENT: %s\n", esp_err_to_name(res));
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }

        last = state;
        vTaskDelay(30 / portTICK_PERIOD_MS);
    }
}
