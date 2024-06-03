#include "ble.h"

char *TAG = "BLE Client Scan";
uint8_t ble_addr_type;
uint16_t cccd_value = BLE_GATT_CHR_F_NOTIFY;

const ble_addr_t server_address = {
    .type = BLE_ADDR_PUBLIC,
    .val = {0x76, 0x89, 0x15, 0x79, 0xcf, 0x58},
};

uint64_t notifyStartTime = 0;
uint64_t notifyEndTime = 0;

bool is_connected = false;
bool is_scanning = false;
bool is_triggered = false;
bool is_notified = false;
bool bleServiceStarted = false;

uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
uint16_t attr_handle = 0;
uint16_t read_handle = 0;
uint16_t write_handle = 0;
bool writable_chr_discovered = false;
bool readable_chr_discovered = false;
bool service_discovery_in_progress = false;
bool device_discovered = false;
struct ble_hs_adv_fields fields;
struct ble_gap_conn_params conn_params;


/*
// status == 0 && 
int on_read(uint16_t conn_handle, int status, struct ble_gatt_attr *attr, void *arg)
{
    uint64_t read_value;
    if (attr != NULL)
    {
        if (attr->om->om_len == sizeof(uint64_t))
        {
            memcpy(&read_value, attr->om->om_data, sizeof(uint64_t));
            printf("Received timestamp value from the server: %llu microseconds.\n", read_value);
        }
    }
    else
    {
        printf("Read operation failed with status: %d\n", status);
    }

    server_timestamp_value = read_value;
    os_mbuf_free_chain(attr->om);

    return 0;
}
*/

// Client-side function to read the timestamp value from the server
void ble_app_read_timestamp(uint16_t conn_handle, uint16_t read_handle)
{
    int rc;

    rc = ble_gattc_read(conn_handle, read_handle, on_read, NULL);
    if (rc != 0)
    {
        printf("Error initiating read operation: %d\n", rc);
    }
}

void ble_app_write_timestamp(uint16_t conn_handle, uint16_t write_handle, float timestamp_value)
{
    uint8_t timestamp_buffer[sizeof(float)];
    memcpy(timestamp_buffer, &timestamp_value, sizeof(float));
    struct os_mbuf *om = ble_hs_mbuf_from_flat(timestamp_buffer, sizeof(float));
    ESP_LOGI("GAP", "6\n");
    if (om != NULL)
    {
        int rc = ble_gattc_write_flat(conn_handle, write_handle, om->om_data, om->om_len, NULL, NULL);
        if (rc != 0)
        {
            printf("Error writing timestamp value: %d\n", rc);
            ESP_LOGI("GAP", "7\n");
        }
        os_mbuf_free_chain(om);
    }
}

// error == NULL && 
// Characteristic discovery callback
int on_disc_chr(uint16_t conn_handle, const struct ble_gatt_error *error,
                       const struct ble_gatt_chr *chr, void *arg)
{
    if (chr != NULL)
    {
        if (ble_uuid_cmp(&chr->uuid.u, MY_READ_CHR_UUID) == 0)
        {
            read_handle = chr->val_handle;
            printf("Read handle: %u\n", read_handle);
            readable_chr_discovered = true;
        } else if (ble_uuid_cmp(&chr->uuid.u, MY_WRITE_CHR_UUID) == 0)
        {
            write_handle = chr->val_handle;
            printf("Write handle: %u\n", write_handle);
            writable_chr_discovered = true;
        }
    }
    return 0;
}

// error == NULL && 
// Service discovery callback
int on_disc_svc(uint16_t conn_handle, const struct ble_gatt_error *error,
                       const struct ble_gatt_svc *service, void *arg)
{
    ESP_LOGI(TAG, "Service discovery completed");
    if (service != NULL && service_discovery_in_progress == true) {
        if (ble_uuid_cmp(&service->uuid.u, MY_SERVICE_UUID) == 0) {
            printf("Found the target service! UUID: %u\n", service->uuid.u16.value);
            vTaskDelay(pdMS_TO_TICKS(1000));
            int rc = ble_gattc_disc_all_chrs(conn_handle, service->start_handle, service->end_handle, on_disc_chr, NULL);
            if (rc != 0) {
                printf("Failed to discover characteristics for service (error: %d)\n", rc);
            }
        } else {
            return -1;
        }
    }
    service_discovery_in_progress = false;
    return 0;
}

// BLE connection event handling callback
int ble_gap_connect_cb(struct ble_gap_event *event, void *arg)
{
    int rc = 0;
    uint64_t received_data;
    struct os_mbuf *om;
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI("GAP", "GAP EVENT CONNECTED");
            conn_handle = event->connect.conn_handle;
            is_connected = true;
            is_scanning = false;
            printf("Connected with %.*s\n", fields.name_len, fields.name);

            vTaskDelay(pdMS_TO_TICKS(1000));

            if (service_discovery_in_progress == false) {
                service_discovery_in_progress = true;
                rc = ble_gattc_disc_svc_by_uuid(event->connect.conn_handle, MY_SERVICE_UUID, on_disc_svc, NULL);
                if (rc != 0) {
                    printf("Failed to initiate service discovery (error: %d)\n", rc);
                    service_discovery_in_progress = false;
                }
            } else {
                printf("Service discovery already in progress, skipping.\n");
            }
            vTaskDelay(pdMS_TO_TICKS(500));
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI("GAP", "GAP EVENT DISCONNECTED");
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            writable_chr_discovered = false;
            service_discovery_in_progress = false;
            bleServiceStarted = false;
            is_scanning = false;
            is_connected = false;
            write_handle = 0;
            read_handle = 0;
            break;
        case BLE_GAP_EVENT_NOTIFY_RX:
            gptimer_start(gptimerNotify);
            gptimer_get_raw_count(gptimerNotify, &notifyStartTime);
            ESP_LOGI("GAP", "Received Notification");   
            is_notified = true;
            printf("Recieved notification, started counter!\n");
            struct os_mbuf *om = event->notify_rx.om;
            memcpy(&received_data, om->om_data, sizeof(received_data));
            ESP_LOGI(TAG, "Received Data: %llu", received_data);
            os_mbuf_free_chain(om);
            break;
        default:
            break;
    }
    return 0;
}

bool is_server_address(const ble_addr_t *addr) {
    for (int i = 0; i < 6; i++) { // Only compare the first 6 bytes
        if (addr->val[i] != server_address.val[i]) {
            return false;
        }
    }
    return true;
}


// BLE event handling
int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    int rc = 0;
    switch (event->type)
    {
    case BLE_GAP_EVENT_DISC:
        ESP_LOGI("GAP", "GAP EVENT DISCOVERY");
        ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        if(fields.name_len > 0)
        {
            printf("Discovered %.*s\n", fields.name_len, fields.name);
        }
        ble_gap_disc_cancel();

        memset(&conn_params, 0, sizeof(conn_params));
        conn_params.scan_itvl = BLE_GAP_SCAN_FAST_INTERVAL_MAX;
        conn_params.scan_window = BLE_GAP_SCAN_FAST_WINDOW;
        conn_params.itvl_min = BLE_GAP_INITIAL_CONN_ITVL_MIN;
        conn_params.itvl_max = BLE_GAP_INITIAL_CONN_ITVL_MAX;
        conn_params.latency = BLE_GAP_INITIAL_CONN_LATENCY;
        conn_params.supervision_timeout = BLE_GAP_INITIAL_SUPERVISION_TIMEOUT;
        conn_params.min_ce_len = BLE_GAP_INITIAL_CONN_MIN_CE_LEN;
        conn_params.max_ce_len = BLE_GAP_INITIAL_CONN_MAX_CE_LEN;

        printf("Event Address: ");
        for (int i = 0; i < sizeof(ble_addr_t); i++) {
            printf("%02x ", event->disc.addr.val[i]);
        }
        printf("\n");

        if(is_server_address(&event->disc.addr) == true)
        {
            rc = ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &event->disc.addr, 1000, &conn_params, ble_gap_connect_cb, NULL);
            vTaskDelay(pdMS_TO_TICKS(400));
            if (rc != 0)
            {
                printf("Failed to initiate connection (error: %d)\n", rc);
                ble_app_scan();
            }
        }
        else
        {
            printf("Server address does not match, skipping connection attempt.\n");
            ble_app_scan();
        }
        break;
    default:
        break;

    }
    return 0;
}

void ble_app_scan(void)
{
    struct ble_gap_disc_params disc_params;

    printf("Start scanning ...\n");

    disc_params.filter_duplicates = 1;
    disc_params.passive = 0;
    disc_params.itvl = 128;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    ble_gap_disc(ble_addr_type, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
}

// The application
void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    vTaskDelay(pdMS_TO_TICKS(1000));                     
}