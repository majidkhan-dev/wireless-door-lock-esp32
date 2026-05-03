#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_rom_sys.h"
#include "lcd_i2c.h"

#define LCD_ADDR      0x27   // try 0x3F if display stays blank
#define LCD_BACKLIGHT 0x08
#define LCD_EN        0x04
#define LCD_RS        0x01
#define I2C_NUM       I2C_NUM_0
#define I2C_TIMEOUT   pdMS_TO_TICKS(100)

static void lcd_write_byte(uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LCD_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM, cmd, I2C_TIMEOUT);
    i2c_cmd_link_delete(cmd);
}

static void lcd_pulse_en(uint8_t data)
{
    lcd_write_byte(data | LCD_EN | LCD_BACKLIGHT);
    esp_rom_delay_us(1);
    lcd_write_byte((data & ~LCD_EN) | LCD_BACKLIGHT);
    esp_rom_delay_us(50);
}

static void lcd_send_nibble(uint8_t nibble, uint8_t mode)
{
    lcd_pulse_en((nibble << 4) | mode);
}

static void lcd_send_byte(uint8_t byte, uint8_t mode)
{
    lcd_send_nibble(byte >> 4,   mode);
    lcd_send_nibble(byte & 0x0F, mode);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_send_byte(cmd, 0);
}

static void lcd_data(uint8_t data)
{
    lcd_send_byte(data, LCD_RS);
}

void lcd_init(void)
{
    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_send_nibble(0x03, 0); vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_nibble(0x03, 0); vTaskDelay(pdMS_TO_TICKS(2));
    lcd_send_nibble(0x03, 0); vTaskDelay(pdMS_TO_TICKS(2));
    lcd_send_nibble(0x02, 0); vTaskDelay(pdMS_TO_TICKS(2));

    lcd_cmd(0x28);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(5));
}

void lcd_clear(void)
{
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(5));
}

void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t row_offsets[] = {0x00, 0x40};
    lcd_cmd(0x80 | (row_offsets[row] + col));
}

void lcd_print(const char *str)
{
    while (*str)
        lcd_data((uint8_t)*str++);
}
