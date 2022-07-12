/**********************************************
 *
 * Copyright (c) 2016-2021 IoTerop.
 * All rights reserved.
 *
 * This program and the accompanying materials
 * are made available under the terms of
 * IoTerop’s IOWA License (LICENSE.TXT) which
 * accompany this distribution.
 *
 **********************************************/

/**************************************************
 *
 * This is a very simple LwM2M Client demonstrating
 * IOWA ease-of-use over Nordic nrf9160DK
 *
 **************************************************/

#include "iowa_client.h"
#include "iowa_ipso.h"

#include <modem/lte_lc.h>
#include <net/socket.h>
#include <stdio.h>
#include <zephyr.h>

#include <modem/modem_key_mgmt.h>
#include <net/tls_credentials.h>

#if defined(EXIST_IN_KCONFIG)
// Theses settings are defined in KConfig file
// On iowa-server.ioterop.com, the secure port is on 5684 (non-secure: 5683)
#define CONFIG_IOWA_SERVER_URI "coap://iowa-server.ioterop.com"
#define CONFIG_IOWA_SERVER_PORT "5684"

#define CONFIG_IOWA_DEVICE_NAME "IOWA_nrf9160DK_Client"

#define CONFIG_IOWA_SERVER_SHORT_ID 1234
#define CONFIG_IOWA_SERVER_LIFETIME 50

#define CONFIG_IOWA_PSK_IDENTITY  "MyIdentity"
#define CONFIG_IOWA_PSK_KEY       "123456"
#define CONFIG_IOWA_BOARD_TLS_TAG 280234110

#endif


// LwM2M Server details
#define SERVER_SHORT_ID CONFIG_IOWA_SERVER_SHORT_ID //default: 1234
#define SERVER_LIFETIME CONFIG_IOWA_SERVER_LIFETIME //default: 50
#define SERVER_URI CONFIG_IOWA_SERVER_URI ":" CONFIG_IOWA_SERVER_PORT

// Change the name for non secure device
#if defined(CONFIG_MBEDTLS)
  #define ENDPOINT_NAME CONFIG_IOWA_DEVICE_NAME
#else
  #define ENDPOINT_NAME CONFIG_IOWA_DEVICE_NAME "_NoSec"  // add suffix to device name
#endif

static char client_identity[] = CONFIG_IOWA_PSK_IDENTITY ;
static char client_psk[] = CONFIG_IOWA_PSK_KEY;       //Not in base64 

// proto - client_platform.c
extern void *get_platform_data();

// a structure to store data for the measure task
typedef struct
{
    iowa_context_t iowaContext;
    iowa_sensor_t voltSensorId;
} measure_data_t;
measure_data_t measureP;

#define THREAD_STACK_SIZE KB(1)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1) //K_LOWEST_APPLICATION_THREAD_PRIO

static K_KERNEL_STACK_DEFINE(measure_thread_stack, THREAD_STACK_SIZE);
static struct k_thread measure_thread_data;
static k_tid_t measure_thread_id;

// notif. for cellular
K_SEM_DEFINE(lte_connected, 0, 1);

//#if defined(CONFIG_BSD_LIBRARY)
/* --------------------------------------------------------------- 
*/
static void lte_handler(const struct lte_lc_evt *const evt) {
    switch (evt->type) {
    case LTE_LC_EVT_NW_REG_STATUS:
        if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
            (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
            break;
        }

        printk("Network registration status: %s\n",
            evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "Connected - home network" : "Connected - roaming\n");
        k_sem_give(&lte_connected);
        break;
    case LTE_LC_EVT_PSM_UPDATE:
        printk("PSM parameter update: TAU: %d, Active time: %d\n",
            evt->psm_cfg.tau, evt->psm_cfg.active_time);
        break;
    case LTE_LC_EVT_EDRX_UPDATE: {
        char log_buf[60];
        ssize_t len;

        len = snprintf(log_buf, sizeof(log_buf),
            "eDRX parameter update: eDRX: %f, PTW: %f\n",
            evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
        if (len > 0) {
            printk("%s\n", log_buf);
        }
        break;
    }
    case LTE_LC_EVT_RRC_UPDATE:
        printk("RRC mode: %s\n",
            evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ? "Connected" : "Idle\n");
        break;
    case LTE_LC_EVT_CELL_UPDATE:
        printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
            evt->cell.id, evt->cell.tac);
        break;
    default:
        break;
    }
}

/* --------------------------------------------------------------- 
*/
static int configure_low_power(void) {
    int err;

#if defined(CONFIG_MODEM_PSM_ENABLE)
    /** Power Saving Mode */
    err = lte_lc_psm_req(true);
    if (err) {
        printk("lte_lc_psm_req, error: %d\n", err);
    }
#else
    err = lte_lc_psm_req(false);
    if (err) {
        printk("lte_lc_psm_req, error: %d\n", err);
    }
#endif

#if defined(CONFIG_MODEM_EDRX_ENABLE)
    /** enhanced Discontinuous Reception */
    err = lte_lc_edrx_req(true);
    if (err) {
        printk("lte_lc_edrx_req, error: %d\n", err);
    }
#else
    err = lte_lc_edrx_req(false);
    if (err) {
        printk("lte_lc_edrx_req, error: %d\n", err);
    }
#endif

#if defined(CONFIG_MODEM_RAI_ENABLE)
    /** Release Assistance Indication  */
    err = lte_lc_rai_req(true);
    if (err) {
        printk("lte_lc_rai_req, error: %d\n", err);
    }
#endif

    return err;
}

/* --------------------------------------------------------------- 
*/
static int modem_configure(void) {
    int err;

    if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
        /* Do nothing, modem is already configured and LTE connected. */
    } else {
        err = lte_lc_init_and_connect_async(lte_handler);
        if (err) {
            printk("Modem configuration, error: %d\n", err);
            return err;
        }
    }
}
//#endif


/* ----------------------------------------------------
 * handle the result notification
*/
#ifndef CONFIG_IOWA_EVAL_VERSION 
static void prv_sendResultCb(uint32_t shortId,
    iowa_dm_operation_t operation,
    iowa_status_t status,
    iowa_response_content_t *contentP,
    void *userDataP,
    iowa_context_t contextP) {
    (void)shortId;
    (void)contextP;
    (void)contentP;
    (void)userDataP;

    if (operation == IOWA_DM_DATA_PUSH) {
        printk("\tprv_sendResultCb():Send operation occurred: %u.%02u\n", (status & 0xFF) >> 5, (status & 0x1F));
    }
}
#endif
/* ----------------------------------------------------
*/
static void send_measure_fn(void) {
    iowa_status_t result;
    iowa_sensor_uri_t sensorUri;
    
    sensorUri.id = measureP.voltSensorId;
    sensorUri.resourceId = IOWA_LWM2M_ID_ALL;

    while (1) {
        if (measureP.voltSensorId > 0) {
            // Example: Update the sensor values with a random value
            float newValue = (float)(rand() % 100);
            result = iowa_client_IPSO_update_value(measureP.iowaContext, measureP.voltSensorId, newValue);
            k_yield();
            if (result != IOWA_COAP_NO_ERROR) {
                printk("Updating the voltage sensor value failed (%u.%02u).\n", (result & 0xFF) >> 5, (result & 0x1F));
            }
            printk("\n===> Thread: Voltage sensor value changed to %d.\n", (int)newValue);
        }
        k_msleep(1000);
    }
}

/* --------------------------------------------------------------- 
*/
void main(void) {
    int err;
    iowa_context_t iowaH;
    iowa_status_t result;
    iowa_device_info_t devInfo;
    void *platformDataP;

    printk("**************************************\n");
    printk("** Iowa sample client for nrf9160DK **\n");
    printk("** (c)IoTerop 2021                  **\n");
    printk("**************************************\n");
    printk("Server:  %s\n", SERVER_URI);
    printk("Endpoint Name: %s\n", ENDPOINT_NAME);

    printk("Connecting celullar network...\n");
    measureP.voltSensorId = 0;

#if defined(CONFIG_BSD_LIBRARY)
    err = configure_low_power();
    if (err) {
        printk("Unable to set low power configuration, error: %d\n",
            err);
    }
#endif
    modem_configure();

    k_sem_take(&lte_connected, K_FOREVER);


    // Application specific : initialize abstraction layer functions
    printk("Start IOWA lwm2m stack\n");
#if defined(CONFIG_MBEDTLS)
    printk("MBEDTLS version\n");
#endif

    platformDataP = get_platform_data();
    if (platformDataP == NULL) {
        printk("Error initializing platform functions.\n");
        return;
    }

    // Initialize the IOWA stack.
    iowaH = iowa_init(platformDataP);
    if (iowaH == NULL) {
        printk("IOWA context initialization failed.\n");
        goto cleanup;
    }

    // Save iowa context
    measureP.iowaContext = iowaH;

    // Start "send measure" thread
    measure_thread_id = k_thread_create(&measure_thread_data,
        &measure_thread_stack[0],
        K_KERNEL_STACK_SIZEOF(measure_thread_stack),
        (k_thread_entry_t)send_measure_fn,
        NULL, NULL, NULL,
        /* Lowest priority cooperative thread */
        THREAD_PRIORITY, 0, K_NO_WAIT);

    // Configure the LwM2M Client
    memset(&devInfo, 0, sizeof(iowa_device_info_t));
    devInfo.manufacturer = "IoTerop";
    devInfo.deviceType = "IOWA nrf9160DK basic sample";
    devInfo.modelNumber = "nrf9160DK-001";
    result = iowa_client_configure(iowaH, ENDPOINT_NAME, &devInfo, NULL);

    if (result != IOWA_COAP_NO_ERROR) {
        printk("IOWA Client configuration failed (%u.%02u).\n", (result & 0xFF) >> 5, (result & 0x1F));
        goto cleanup;
    }

    // Add a LwM2M Server to connect to
    result = iowa_client_add_server(iowaH, SERVER_SHORT_ID, SERVER_URI, SERVER_LIFETIME, 0, IOWA_SEC_NONE);  
    if (result != IOWA_COAP_NO_ERROR) {
        printk("Adding a server failed (%u.%02u).\n", (result & 0xFF) >> 5, (result & 0x1F));
        goto cleanup;
    }

    // Add an IPSO voltage sensor
    result = iowa_client_IPSO_add_sensor(iowaH, IOWA_IPSO_VOLTAGE, 12.0, "V", "Test DC", 0.0, 24.0, &measureP.voltSensorId);
    if (result != IOWA_COAP_NO_ERROR) {
        printk("Adding a sensor failed (%u.%02u).\n", (result & 0xFF) >> 5, (result & 0x1F));
        goto cleanup;
    }

    //IOWA stack runs for 2 minutes
    result = iowa_step(iowaH, 240);
    if (result != IOWA_COAP_NO_ERROR) {
        printk("iowa_step failed with (%u.%02u).\n", (result & 0xFF) >> 5, (result & 0x1F));
        goto cleanup;
    }

cleanup:
    printk("Leaving IOWA.\n");

    k_thread_abort(measure_thread_id);

    iowa_client_remove_server(iowaH, SERVER_SHORT_ID);
    iowa_close(iowaH);
}