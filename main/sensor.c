#include "sensor.h"
#include "ble.h"

const char *PBTN = "Push_Button_Task";
const char *SNSR = "Sensor_Task";

volatile uint64_t echoStartTime = 0;
volatile uint64_t echoEndTime = 0;
volatile bool echoPulseStarted = false;

gpio_config_t io_conf_start = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_INPUT,
    .pin_bit_mask = (1ULL << START_PIN),
    .pull_down_en = 0,
    .pull_up_en = 1
};

gpio_config_t io_conf_sensor = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = (1ULL << SENSOR_TRIGGER_PIN),
    .pull_down_en = 0,
    .pull_up_en = 0
};

gpio_config_t io_conf_echo = {
    .intr_type = GPIO_INTR_ANYEDGE,
    .mode = GPIO_MODE_INPUT,
    .pin_bit_mask = (1ULL << ECHO_PIN),
    .pull_down_en = 1,
    .pull_up_en = 0
};

//______________________________________________________//

gptimer_handle_t gptimerEcho = NULL;
gptimer_handle_t gptimerNotify = NULL;
gptimer_config_t timer_config_echo = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1000000
};
gptimer_config_t timer_config_notify = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1000000
};

void IRAM_ATTR echoPinHandler(void *arg) {
    if (gpio_get_level(ECHO_PIN) == 1) {
        gptimer_start(gptimerEcho);
        echoPulseStarted = true;
    } else {
        gptimer_get_raw_count(gptimerEcho, &echoEndTime);
        gptimer_stop(gptimerEcho);
        echoPulseStarted = false;
    }
}

void sensor_task(void *parameters) {
    float distance = 0.0;
    float ticks_duration = 0.0;
    uint32_t ticks_num = 0;
    gpio_config(&io_conf_sensor);
    gpio_set_intr_type(ECHO_PIN, GPIO_INTR_ANYEDGE);
    
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config_echo, &gptimerEcho));
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config_notify, &gptimerNotify));
    gpio_install_isr_service(0);
    gptimer_enable(gptimerEcho);
    gptimer_enable(gptimerNotify);
    gpio_isr_handler_add(ECHO_PIN, echoPinHandler, NULL);

    printf("Started sensor task!\n");

    while(true) {
        if(echoPulseStarted == false && is_notified == true) {
            gpio_set_level(SENSOR_TRIGGER_PIN, 1);
            ets_delay_us(15);
            gpio_set_level(SENSOR_TRIGGER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(25));

            ticks_num = echoEndTime - echoStartTime;
            echoStartTime = echoEndTime;
            ticks_duration = (float)ticks_num / 1.0;
            distance = ((ticks_duration / 1000000.0) * 343.0) / 2.0;
            printf("Distance: %2f\n", distance);
            if(distance > 0.0 && distance <= 0.7) {
                gptimer_get_raw_count(gptimerNotify, &notifyEndTime);
                gptimer_stop(gptimerNotify);
                ticks_num = notifyEndTime - notifyStartTime;
                notifyStartTime = notifyEndTime;
                ticks_duration = (float)ticks_num / 1000000.0;
                printf("It took %.3f seconds!\n", ticks_duration);
                ble_app_write_timestamp(conn_handle, write_handle, ticks_duration);
                is_notified = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void push_button_task(void *parameters) {
    int button_state = 0;
    AppState app_state = STATE_IDLE;

    gpio_config(&io_conf_start);

    while (true) {
        button_state = gpio_get_level(START_PIN);
        switch (app_state) {
            case STATE_IDLE:
                if (button_state == 0) {
                    vTaskDelay(pdMS_TO_TICKS(500));
                    printf("BLE service started.\n");
                    ble_app_scan();
                    is_scanning = true;
                    app_state = STATE_BLE_STARTED;
                }
                break;

            case STATE_BLE_STARTED:
                if (button_state == 0) {
                    vTaskDelay(pdMS_TO_TICKS(500));
                    if (is_connected) {
                        ble_gap_terminate(conn_handle, 0);
                    }
                    is_connected = false;
                    printf("BLE service ended.\n");
                    app_state = STATE_IDLE;
                }
                break;

            default:
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(60));
    }
}