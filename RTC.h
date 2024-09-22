#pragma once

// This code will work until December 31, 2099, however, Unix time will break after 2038

#include <stdio.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/sync.h"
#include "RTC.h"

#define I2C_SDA 6
#define I2C_SCL 7
#define I2C_PORT i2c1

typedef unsigned short ushort;
typedef unsigned long ulong;

int RTC_ADDR = 0x68;

uint8_t lastSecond = 0;
uint8_t lastMin = 0;
uint8_t lastHour = 0;
uint8_t lastDay = 0;
uint8_t lastDayOfMonth = 0;
uint8_t lastMonth = 0;
uint8_t lastYear = 24;

uint8_t getRTCVal(uint8_t val) {
    return ((val / 10) << 4) + ((val%10) & 0b00001111);

}
uint8_t i2c_getRTCdata(uint8_t REGISTER) {
    uint8_t result[1];
    //uint32_t interrupt = save_and_disable_interrupts();
    i2c_write_blocking(I2C_PORT, RTC_ADDR, &REGISTER, 1, true);
    sleep_us(10);
    i2c_read_blocking(I2C_PORT, RTC_ADDR, result, 1, false);
    return result[0];
}
void i2c_writeRTCdata(uint8_t REGISTER, uint8_t data) {
    uint8_t reg_and_data[2];
    reg_and_data[0] = REGISTER;
    reg_and_data[1] = data;
    
    //uint32_t interrupt = save_and_disable_interrupts();
    i2c_write_blocking(I2C_PORT, RTC_ADDR, reg_and_data, 2, false);
    sleep_us(10);
    
}
void setTime(int secs, int mins, int hours, int days, int month, int year) {
    gpio_put(25, 1);
    sleep_ms(100);
    i2c_writeRTCdata(0, getRTCVal(secs));
    i2c_writeRTCdata(1, getRTCVal(mins));
    i2c_writeRTCdata(2, getRTCVal(hours));
    i2c_writeRTCdata(4, getRTCVal(days));
    i2c_writeRTCdata(5, getRTCVal(month));
    i2c_writeRTCdata(6, getRTCVal(year));
    sleep_ms(500);
    gpio_put(25, 0);
}

uint8_t getSeconds() {
    uint8_t rtc_seconds = i2c_getRTCdata(0);
    uint8_t secs =     (rtc_seconds & 0b00001111);
    uint8_t secs_ten = ((rtc_seconds & 0b01110000) >> 4)*10;
    lastSecond = secs+secs_ten;
    return secs+secs_ten;
}
uint8_t getMinutes() {
    uint8_t rtc_mins = i2c_getRTCdata(1);
    uint8_t mins =     (rtc_mins & 0b00001111);
    uint8_t mins_ten = ((rtc_mins & 0b01110000) >> 4)*10;
    lastMin = mins+mins_ten;
    return mins+mins_ten;
}
uint8_t getHour() {
    uint8_t rtc_hours = i2c_getRTCdata(2);
    uint8_t hours =     (rtc_hours & 0b00001111);
    uint8_t hours_ten = ((rtc_hours & 0b01110000) >> 4)*10;
    lastHour = hours+hours_ten;
    return hours+hours_ten;
}
uint8_t getDay() {
    uint8_t rtc_days = i2c_getRTCdata(3);
    lastDay = (rtc_days & 0b00000111);
    return (rtc_days & 0b00000111);
}
uint8_t getDayOfMonth() {
    uint8_t rtc_date = i2c_getRTCdata(4);
    uint8_t date =     (rtc_date & 0b00001111);
    uint8_t date_ten = ((rtc_date & 0b01110000) >> 4)*10;
    lastDayOfMonth = date+date_ten;
    return date+date_ten;
}
uint8_t getMonth() {
    uint8_t rtc_month = i2c_getRTCdata(5);
    uint8_t month =     (rtc_month & 0b00001111);
    uint8_t month_ten = ((rtc_month & 0b00010000) >> 4)*10;
    lastDayOfMonth = month+month_ten;
    return month+month_ten;
}
uint8_t getYear() {
    uint8_t rtc_year = i2c_getRTCdata(6);
    if(rtc_year == (uint8_t)255) {
        gpio_put(25, 1);
        return 24; // The year this code was written
    }
    uint8_t year =     (rtc_year & 0b00001111);
    uint8_t year_ten = ((rtc_year & 0b11110000) >> 4)*10;
    lastYear = year;
    return year+year_ten;
}
ulong getUnixTime() {
    long secs = getSeconds();
    long mins = getMinutes();
    long hours = getHour();
    long days = getDayOfMonth();
    long month = getMonth();
    long year = getYear()+2000;
    
    struct tm t;
    time_t t_of_day;
    t.tm_year = year-1900;  // Year - 1900
    t.tm_mon = getMonth()-1;           // Month, where 0 = jan
    t.tm_mday = getDayOfMonth();          // Day of the month
    t.tm_hour = getHour();
    t.tm_min = getMinutes();
    t.tm_sec = getSeconds();
    t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
    t_of_day = mktime(&t);

    return (long) t_of_day;
}
void printRTCData() {
    printf("Second: %d\n",getSeconds());
    printf("Minute: %d\n",getMinutes());
    printf("Hour:   %d\n",getHour());
    printf("Weekday:%d\n",getDay());
    printf("Day:    %d\n",getDayOfMonth());
    printf("Month:  %d\n",getMonth());
    printf("Year:   %d\n",getYear());
    printf("Unix Time:   %d\n",getUnixTime());
}
