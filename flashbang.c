#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gpiod.h>       // libgpiod v2
#include <MQTTClient.h>
#include <cjson/cJSON.h> // Fixed cJSON path

// --- CONFIGURATION ---
// Based on PB5 -> Chip 1, Line 5
#define GPIO_CHIP_PATH   "/dev/gpiochip1" 
#define GPIO_OFFSET      5                

#define BROKER_URI       "tcp://mosquitoserver:1883"
#define CLIENT_ID        "Cubie_CS2_v2"
#define TARGET_TOPIC     "cs2mqtt/xxxxxxxxxxxxxxxxx/player-state"
#define MQTT_USER        "xxxxx"
#define MQTT_PASS        "xxxxxxxxxxxxxx"

// Logic: Active Low Relay
// 1 (High) = Relay OFF
// 0 (Low)  = Relay ON
#define VAL_RELAY_OFF    GPIOD_LINE_VALUE_ACTIVE   // High
#define VAL_RELAY_ON     GPIOD_LINE_VALUE_INACTIVE // Low

// Global Handles
struct gpiod_chip *chip = NULL;
struct gpiod_line_request *request = NULL;

void cleanup_gpio() {
    if (request) gpiod_line_request_release(request);
    if (chip) gpiod_chip_close(chip);
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    // 1. Parse JSON
    cJSON *json = cJSON_ParseWithLength((char*)message->payload, message->payloadlen);
    
    if (json) {
        cJSON *flashed_item = cJSON_GetObjectItemCaseSensitive(json, "flashed");

        if (cJSON_IsNumber(flashed_item)) {
            int flash_val = flashed_item->valueint;

            // 2. Determine State
            // If flashed > 0 -> Turn ON (Low/0). Else OFF (High/1).
            enum gpiod_line_value target = (flash_val > 0) ? VAL_RELAY_ON : VAL_RELAY_OFF;

            // 3. Set GPIO (Zero Latency)
            gpiod_line_request_set_value(request, GPIO_OFFSET, target);
        }
        cJSON_Delete(json);
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost. Cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    // --- 1. GPIO Setup (libgpiod v2 style) ---
    chip = gpiod_chip_open(GPIO_CHIP_PATH);
    if (!chip) {
        perror("Failed to open GPIO chip");
        return 1;
    }

    // Prepare settings: Output, Start HIGH (OFF)
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, VAL_RELAY_OFF);

    // Configure line
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offset = GPIO_OFFSET;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    // Prepare request
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "cs2_mqtt_listener");

    // Request the line
    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    // Cleanup config objects (no longer needed after request)
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);

    if (!request) {
        perror("Failed to request GPIO line (Pin busy?)");
        gpiod_chip_close(chip);
        return 1;
    }

    // --- 2. MQTT Setup ---
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    MQTTClient_create(&client, BROKER_URI, CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = MQTT_USER;
    conn_opts.password = MQTT_PASS;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, NULL);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("MQTT Connect failed, code %d\n", rc);
        cleanup_gpio();
        exit(EXIT_FAILURE);
    }

    printf("Connected! Listening for flashes on %s Line %d...\n", GPIO_CHIP_PATH, GPIO_OFFSET);
    MQTTClient_subscribe(client, TARGET_TOPIC, 0);

    // --- 3. Wait Loop ---
    printf("Press Q to quit\n");
    int ch;
    do {
        ch = getchar();
    } while (ch != 'Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    cleanup_gpio();
    return 0;
}
