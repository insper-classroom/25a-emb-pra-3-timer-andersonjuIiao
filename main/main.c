/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/rtc.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "hardware/gpio.h"

const int TRIG_PIN = 28;
const int ECHO_PIN = 27;

volatile int time_init = 0;
volatile int time_end = 0;
volatile int timer_fired = 0;

void gpio_callback(uint gpio, uint32_t events) {
    if (events == 0x4) {         // fall edge
        time_end = to_us_since_boot(get_absolute_time());
    } else if (events == 0x8) {  // rise edge
        time_init = to_us_since_boot(get_absolute_time());
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    timer_fired = 1;
    return 0;
}

void disparar_sensor() {
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);
}

void processar_leitura() {
    datetime_t current_time;
    rtc_get_datetime(&current_time);

    if (timer_fired) {
        printf("%02d:%02d:%02d - Falha\n", current_time.hour, current_time.min, current_time.sec);
    } else {
        int tempo_pulso = time_end - time_init;
        float distancia = (tempo_pulso * 0.0343f) / 2.0f;
        printf("%02d:%02d:%02d - %.1f cm\n", current_time.hour, current_time.min, current_time.sec, distancia);
    }
}

int main() {
    stdio_init_all();

    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN, 0);


    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(ECHO_PIN, 
                                        GPIO_IRQ_EDGE_RISE | 
                                        GPIO_IRQ_EDGE_FALL, 
                                        true,
                                        &gpio_callback);
    

    datetime_t t = {
        .year  = 2025,
        .month = 03,
        .day   = 14,
        .dotw  = 6,
        .hour  = 11,
        .min   = 50,
        .sec   = 00
    };

    rtc_init();
    rtc_set_datetime(&t);

    bool leitura_ativa = false;

    printf("Digite 's' para Start ou 'p' para Stop:\n");


    while (true) {

        if (getchar_timeout_us(100000) == 's' || getchar_timeout_us(100000) == 'S') {
            leitura_ativa = true;
            printf("Iniciando medições...\n");
        } else if (getchar_timeout_us(100000) == 'p' || getchar_timeout_us(100000) == 'P') {
            leitura_ativa = false;
            printf("Parando medições...\n");
        }
        
        if (leitura_ativa) {

            time_init = 0;
            time_end = 0;
            timer_fired = 0;
            
            alarm_id_t alarm_id = add_alarm_in_ms(30, alarm_callback, NULL, false);

            disparar_sensor();
            
            while (time_end == 0 && !timer_fired) {
                tight_loop_contents();
            }
            
            if (time_end != 0) {
                cancel_alarm(alarm_id);
            }
            
            processar_leitura();
            
            sleep_ms(1000);
        }
    }
}