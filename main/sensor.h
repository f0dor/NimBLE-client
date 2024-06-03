#ifndef SENSOR_H
#define SENSOR_H

#include <stdio.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "rom/ets_sys.h"
#include "esp_attr.h"
#include "ble.h"


#define SENSOR_TRIGGER_PIN      GPIO_NUM_0
#define START_PIN               GPIO_NUM_1
#define ECHO_PIN                GPIO_NUM_3

extern const char *PBTN;

extern gptimer_handle_t gptimerEcho;
extern gptimer_config_t timer_config_echo;
extern gptimer_handle_t gptimerNotify;
extern gptimer_config_t timer_config_notify;
extern gpio_config_t io_conf_start;
extern gpio_config_t io_conf_sensor;
extern gpio_config_t io_conf_echo;

typedef enum {
    STATE_IDLE,
    STATE_BLE_STARTED
} AppState;

void push_button_task(void *parameters);
void sensor_task(void *parameters);
void IRAM_ATTR echoPinHandler(void *arg);

#endif /*SENSOR_H*/