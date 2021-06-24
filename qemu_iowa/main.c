/**********************************************
*
* Copyright (c) 2016-2021 IoTerop.
* All rights reserved.
*
**********************************************
* Sample IOWA code 
* - running on QEMU/ZephyrOS
*/

// IOWA headers
#include "iowa_client.h"
#include "iowa_ipso.h"

// Platform specific headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <random/rand32.h>

// From client_platform.h
void *get_platform_data();
void platform_data_set_iowa_context(void *userData, iowa_context_t contextP);
void free_platform_data(void *userData);

// LwM2M Server URIs
// WARNING: If you modify the secure URI, remember to change it also in function iowa_system_security_data() in client_1_1_platform.c
#define SERVER_URI "coap://192.0.2.2:5683"
// For secure connection: #define SERVER_SECURE_URI "coaps://192.0.2.2:5684"

#define SERVER_SHORT_ID 1234
#define ENDPOINT_NAME "Zephyr_IOWA_Client_1_0" // ! should be unique - please change
#define CLIENT_LIFETIME 300

// Application specific: a structure to store data for the measure task
typedef struct
{
    iowa_context_t iowaContext;
    iowa_sensor_t tempSensorId;
    iowa_sensor_t voltSensorId;
} measure_data_t;

#ifdef LWM2M_DATA_PUSH_SUPPORT // Available in the full licence version only
/* -------------------------------------------------------------
*/
static void prv_sendResultCb(uint32_t shortId,
                             iowa_dm_operation_t operation,
                             iowa_status_t status,
                             iowa_response_content_t *contentP,
                             void *userDataP,
                             iowa_context_t contextP)
{
    (void)shortId;
    (void)contextP;
    (void)contentP;
    (void)userDataP;
    if (operation == IOWA_DM_DATA_PUSH)
    {
        fprintf(stdout, "Send operation occurred: %u.%02u\r\n", (status & 0xFF) >> 5, (status & 0x1F));
    }
}
#endif

/* -------------------------------------------------------------
*/
void prv_measurement(measure_data_t *dataP)
{
    float newValue;
    iowa_status_t result;
#ifdef LWM2M_DATA_PUSH_SUPPORT // Available in the paid version only
    iowa_sensor_uri_t sensorUri;

    sensorUri.id = dataP->voltSensorId;
    sensorUri.resourceId = IOWA_LWM2M_ID_ALL;
#endif

    // Example: Update the sensor values with a random value
    newValue = 11.5 + ((float)(uint16_t)z_impl_sys_rand32_get()) / UINT16_MAX;
    result = iowa_client_IPSO_update_value(dataP->iowaContext,
                                           dataP->voltSensorId,
                                           newValue);
    if (result != IOWA_COAP_NO_ERROR)
    {
        fprintf(stderr, "Updating the voltage sensor value failed (%u.%02u).\r\n", (result & 0xFF) >> 5, (result & 0x1F));
    }
    fprintf(stdout, "Voltage sensor value changed to %f.\r\n", newValue);

#ifdef LWM2M_DATA_PUSH_SUPPORT // Available in the paid version only
    result = iowa_client_send_sensor_data(dataP->iowaContext, SERVER_SHORT_ID, &sensorUri, 1, prv_sendResultCb, NULL);
#endif

    newValue *= 2.0;
    result = iowa_client_IPSO_update_value(dataP->iowaContext,
                                           dataP->tempSensorId,
                                           newValue);
    if (result != IOWA_COAP_NO_ERROR)
    {
        fprintf(stderr, "Updating the temperature sensor value failed (%u.%02u).\r\n", (result & 0xFF) >> 5, (result & 0x1F));
    }
    fprintf(stdout, "Temperature sensor value changed to %f.\r\n", newValue);
}

/* -------------------------------------------------------------
*/
int main(int argc,
         char *argv[])
{
    iowa_context_t iowaH;
    iowa_status_t result;
    iowa_device_info_t devInfo;
    measure_data_t data;
    int64_t time_stamp;
    void *platformDataP;

    (void)argc;
    (void)argv;

    /******************
     * Initialization
     */
    printf("This a simple Zephyr LwM2M 1.0 Client (UDP) featuring an IPSO Voltage Sensor and an IPSO Temperature Sensor.\r\n\n");

    // Application specific : initialize abstraction layer functions
    platformDataP = get_platform_data();
    if (platformDataP == NULL)
    {
        fprintf(stderr, "Error initializing platform functions.\r\n");
        exit(1);
    }

    // Initialize the IOWA stack.
    iowaH = iowa_init(platformDataP);
    if (iowaH == NULL)
    {
        fprintf(stderr, "IOWA context initialization failed.\r\n");
        goto cleanup;
    }

    platform_data_set_iowa_context(platformDataP, iowaH);

    // Configure the LwM2M Client
    memset(&devInfo, 0, sizeof(iowa_device_info_t));
    devInfo.manufacturer = "IoTerop";
    result = iowa_client_configure(iowaH, ENDPOINT_NAME, &devInfo, NULL);
    if (result != IOWA_COAP_NO_ERROR)
    {
        fprintf(stderr, "IOWA Client configuration failed (%u.%02u).\r\n", (result & 0xFF) >> 5, (result & 0x1F));
        goto cleanup;
    }

    // Add the information of a LwM2M Server to connect to
    result = iowa_client_add_server(iowaH, SERVER_SHORT_ID, SERVER_URI, CLIENT_LIFETIME, 0, IOWA_SEC_NONE);
    // or if you want to use a secure connection:
    // result = iowa_client_add_server(iowaH, SERVER_SHORT_ID, SERVER_SECURE_URI, 300, 0, IOWA_SEC_PRE_SHARED_KEY);
    if (result != IOWA_COAP_NO_ERROR)
    {
        fprintf(stderr, "Adding a server failed (%u.%02u).\r\n", (result & 0xFF) >> 5, (result & 0x1F));
        goto cleanup;
    }

    // store the IOWA context to use it in the measure task
    data.iowaContext = iowaH;

    // Add an IPSO voltage sensor
    result = iowa_client_IPSO_add_sensor(iowaH, IOWA_IPSO_VOLTAGE, 12.0, "V", "Test DC", 0.0, 24.0, &(data.voltSensorId));
    if (result != IOWA_COAP_NO_ERROR)
    {
        fprintf(stderr, "Adding a sensor failed (%u.%02u).\r\n", (result & 0xFF) >> 5, (result & 0x1F));
        goto cleanup;
    }

    // Add an IPSO temperature sensor
    result = iowa_client_IPSO_add_sensor(iowaH, IOWA_IPSO_TEMPERATURE, 24, "Cel", "Test Temperature", -20.0, 50.0, &(data.tempSensorId));
    if (result != IOWA_COAP_NO_ERROR)
    {
        fprintf(stderr, "Adding a second sensor failed (%u.%02u).\r\n", (result & 0xFF) >> 5, (result & 0x1F));
        goto cleanup;
    }

    printf("Starting a Client. Connecting to the LWM2M server.\r\n\n");

    // Record initial timestamp
    time_stamp = k_uptime_get();

    // Let IOWA run indefinitely
    while (iowa_step(iowaH, 1) == IOWA_COAP_NO_ERROR)
    {
        int64_t base_ts = time_stamp;

        // If 3s or more elapsed since last time we refreshed our timestamp
        if (k_uptime_delta(&base_ts) >= 3000)
        {
            // Read sensors and report updated values
            prv_measurement(&data);

            // Refresh timestamp
            time_stamp = k_uptime_get();
        }
    }

    fprintf(stderr, "IOWA step function failed (%u.%02u).\r\n", (result & 0xFF) >> 5, (result & 0x1F));

cleanup:
    iowa_client_IPSO_remove_sensor(iowaH, data.tempSensorId);
    iowa_client_IPSO_remove_sensor(iowaH, data.voltSensorId);
    iowa_client_remove_server(iowaH, SERVER_SHORT_ID);
    iowa_close(iowaH);

    // Application specific: clean up
    free_platform_data(&data);

    return 0;
}
