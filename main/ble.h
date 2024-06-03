#ifndef BLE_H
#define BLE_H

#include <stdio.h>
#include <inttypes.h>
#include "esp_nimble_hci.h"
#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sensor.h"

#define MY_SERVICE_UUID         BLE_UUID16_DECLARE(0x1405)
#define MY_READ_CHR_UUID        BLE_UUID16_DECLARE(0x0508)
#define MY_WRITE_CHR_UUID       BLE_UUID16_DECLARE(0x0512)
#define MY_RTT_CHR_UUID         BLE_UUID16_DECLARE(0x0516)

#define RTT_MEASURE_INTERVAL_MS 1000

extern char *TAG;
extern uint8_t ble_addr_type;
extern uint16_t cccd_value;
extern float timestamp_value;
extern uint64_t notifyStartTime;
extern uint64_t notifyEndTime;

extern const ble_addr_t server_address;

extern uint16_t conn_handle;
extern uint16_t attr_handle;
extern uint16_t read_handle;
extern uint16_t write_handle;
extern bool writable_chr_discovered;
extern bool readable_chr_discovered;
extern bool service_discovery_in_progress;
extern bool device_discovered;
extern bool is_connected;
extern bool is_scanning;
extern bool is_triggered;
extern bool is_notified;
extern bool bleServiceStarted;
extern struct ble_hs_adv_fields fields;
extern struct ble_gap_conn_params conn_params;

bool is_server_address(const ble_addr_t *addr);

int on_read(uint16_t conn_handle, int status, struct ble_gatt_attr *attr, void *arg);

void ble_app_read_timestamp(uint16_t conn_handle, uint16_t read_handle);

void ble_app_write_timestamp(uint16_t conn_handle, uint16_t write_handle, float timestamp_value);

int on_disc_chr(uint16_t conn_handle, const struct ble_gatt_error *error,
                       const struct ble_gatt_chr *chr, void *arg);

int on_disc_svc(uint16_t conn_handle, const struct ble_gatt_error *error,
                       const struct ble_gatt_svc *service, void *arg);

int ble_gap_connect_cb(struct ble_gap_event *event, void *arg);

int ble_gap_event(struct ble_gap_event *event, void *arg);

void ble_app_scan(void);

void ble_app_on_sync(void);


#endif /* BLE_H */