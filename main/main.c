// NimBLE Client

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "sensor.h"
#include "ble.h"

// The infinite task
void host_task(void *param)
{
    nimble_port_run();
}

void app_main()
{
    nvs_flash_init();                               // 1 - Initialize NVS flash using
    esp_nimble_hci_init();                          // 2 - Initialize ESP controller
    nimble_port_init();                             // 3 - Initialize the controller stack
    esp_log_level_set("*", ESP_LOG_INFO);           // Initialize Logging System with log level INFO
    ble_svc_gap_device_name_set("BLE-Client");      // 4 - Set device name characteristic
    ble_hs_cfg.sync_cb = ble_app_on_sync;           // 4 - Set application
    ble_svc_gap_init();                             // 5 - Initialize GAP service
    nimble_port_freertos_init(host_task);           // 6 - Set infinite task
    xTaskCreate(push_button_task, "push_button_task", 2048, NULL, 5, NULL);
    xTaskCreate(sensor_task, "sensor_task", 2048, NULL, 5, NULL);
}