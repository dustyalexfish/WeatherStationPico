//Non-PIO implementation of a rotary encoder system. 

#pragma once
#include <stdio.h>
#include "pico/stdlib.h"

#define ENCODER_CLK 2
#define ENCODER_DT  3
#define ENCODER_SW  4

#define ENCODER_NO_ACTIVITY 0
#define ENCODER_CCW 1
#define ENCODER_CW 2
#define ENCODER_SW_PUSHED 3
bool lastState;

void init_rotaryEncoder() {
    gpio_init(ENCODER_CLK);
    gpio_init(ENCODER_DT);
    gpio_init(ENCODER_SW);

    gpio_set_dir(ENCODER_CLK, GPIO_IN);
    gpio_set_dir(ENCODER_DT,  GPIO_IN);
    gpio_set_dir(ENCODER_SW,  GPIO_IN);

    gpio_pull_up(ENCODER_CLK);

    lastState = gpio_get(ENCODER_CLK);

    
}

int update_rotaryEncoder() {
    
    bool currentState = gpio_get(ENCODER_CLK);
    if(currentState != lastState && currentState == false) {
        lastState = currentState;
        if(gpio_get(ENCODER_DT)) {
            return ENCODER_CW;
        } else {
            return ENCODER_CCW;
        }
    }

    lastState = currentState;

    return ENCODER_NO_ACTIVITY;
}

bool getRotaryEncoderSWPushState() {
    return gpio_get(ENCODER_SW);
}
