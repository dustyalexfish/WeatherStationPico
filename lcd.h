#pragma once

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c1
#define I2C_SDA 6
#define I2C_SCL 7

static int address = 0x27;



void blinkLED() {
    gpio_put(25, 1);
    busy_wait_ms(100);
    gpio_put(25, 0);
}

const uint8_t init_cmd  = 0b00100000;
const uint8_t blank     = 0b00000000;
const uint8_t backlight = 0b00001000;
const uint8_t RS_CHAR   = 0b00000001;
bool lcdBacklight = true;

void lcd_backlight_on() {
    lcdBacklight = true;
    i2c_write_blocking(I2C_PORT, address, &backlight, 1, false);
}
void lcd_backlight_off() {
    lcdBacklight = false;
    i2c_write_blocking(I2C_PORT, address, &blank, 1, false);
}
bool isBacklightOn() {
    return lcdBacklight;
}
void lcd_write_command_raw(uint8_t val) {
    uint32_t ints = save_and_disable_interrupts();
    if(lcdBacklight) {
        uint8_t val_mod = val | 0b00000100 | backlight;
        uint8_t val_backlight = val | backlight;
        i2c_write_blocking(I2C_PORT, address, &val_backlight, 1, false);
        busy_wait_us(10);
        i2c_write_blocking(I2C_PORT, address, &val_mod, 1, false);
        busy_wait_us(20);
        i2c_write_blocking(I2C_PORT, address, &val_backlight, 1, false);
        busy_wait_us(10);
    } else {
        uint8_t val_mod = val | 0b00000100;
        i2c_write_blocking(I2C_PORT, address, &val, 1, false);
        busy_wait_us(10);
        i2c_write_blocking(I2C_PORT, address, &val_mod, 1, false);
        busy_wait_us(20);
        i2c_write_blocking(I2C_PORT, address, &val, 1, false);
        busy_wait_us(10);
    }
    restore_interrupts(ints);
}

void lcd_write_char(char character) {
    lcd_write_command_raw((character & 0b11110000) | RS_CHAR);
    lcd_write_command_raw(((character & 0b00001111) << 4) | RS_CHAR);
}
void lcd_write_chars(char* chars, int length) {
    for(int i = 0; i < length; ++i) {
        lcd_write_command_raw((chars[i] & 0b11110000) | RS_CHAR);
        lcd_write_command_raw(((chars[i] & 0b00001111) << 4) | RS_CHAR);
    }
}
void lcd_clear_screen() {
    lcd_write_command_raw(blank);
    lcd_write_command_raw(0b00010000);
}

void lcd_home() {
    lcd_write_command_raw(blank);
    lcd_write_command_raw(0b00100000);
}
void shiftCursor(int numberOfTimes) {
    for(int i = 0; i < numberOfTimes; ++i) {
        lcd_write_command_raw(0b00010000);
        lcd_write_command_raw(0b01000000);
    }
}

void init_lcd() {
    
    busy_wait_ms(1000);


    lcd_write_command_raw(init_cmd);

    //                      7654BEWR
    lcd_write_command_raw(0b00000000);

    lcd_write_command_raw(0b00100000);
    
    lcd_write_command_raw(0b00000000);
    lcd_write_command_raw(0b11000000);

    lcd_write_chars("Hello World!", 12);

    gpio_put(25, 1);
}
