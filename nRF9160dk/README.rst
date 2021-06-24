.. _iowa_client:

nRF9160: IOWA Client
############

.. contents::
   :local:
   :depth: 2

The IOWA sample demonstrates the behavior of the IOWA lwM2M stack running on a cellular device.
The sample uses the :ref:`nrfxlib:bsdlib` and :ref:`lte_lc_readme` library.
The IOWA library *must* be the full version.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160ns, nrf9160dk_nrf9160ns

Overview
********

The sample initiates a celullar connection, then initializes the IOWA stack.
A sample thread is started to simulate a data acquisition (and update). This thread sends a random value in the IPSO object *IOWA_IPSO_VOLTAGE* 

Functionality and Supported Technologies
========================================

 * UDP/IP
 * LwM2M IOWA stack


Configuration
*************

|config|

You can configure the following options:

* :option:`CONFIG_IOWA_SERVER_URI`
* :option:`CONFIG_IOWA_SERVER_SHORT_ID`
* :option:`CONFIG_IOWA_SERVER_LIFETIME`
* :option:`CONFIG_IOWA_DEVICE_NAME`
* :option:`CONFIG_MODEM_PSM_ENABLE`
* :option:`CONFIG_MODEM_EDRX_ENABLE`
* :option:`CONFIG_MODEM_RAI_ENABLE`


Configuration options
=====================

.. option:: CONFIG_IOWA_SERVER_URI - IOWA server address

This configuration option sets the server URI (e.g.: "coap://iowa-server.ioterop.com:5683").

.. option:: CONFIG_IOWA_SERVER_SHORT_ID - IOWA server short ID

This configuration option sets the IOWA server short ID to use.

.. option:: CONFIG_IOWA_SERVER_LIFETIME - IOWA server lifetime

This configuration option sets the IOWA server lifetime value.

.. option:: CONFIG_IOWA_DEVICE_NAME - IOWA device endpoint name

This configuration option sets the server address port number.

.. option:: CONFIG_MODEM_PSM_ENABLE - PSM mode configuration

This configuration option, if set, allows the sample to request PSM from the modem or cellular network.

.. option:: CONFIG_MODEM_EDRX_ENABLE - eDRX mode configuration

This configuration option, if set, allows the sample to request eDRX from the modem or cellular network.

.. option:: CONFIG_MODEM_RAI_ENABLE - RAI configuration

This configuration option, if set, allows the sample to request RAI for transmitted messages.

.. note::
   PSM, eDRX and RAI value or timers are set via the configurable options for the :ref:`lte_lc_readme` library.


Additional configuration
========================

Below configurations are recommended for low power behavior:

 * :option:`CONFIG_LTE_PSM_REQ_RPTAU`
 * :option:`CONFIG_LTE_PSM_REQ_RAT` set to 0.
 * :option:`CONFIG_SERIAL` disabled in ``prj.conf`` and ``spm.conf``.
 * :option:`CONFIG_MODEM_EDRX_ENABLE` set to false.
 * :option:`CONFIG_MODEM_RAI_ENABLE` set to true for NB-IoT. It is not supported for LTE-M.

PSM and eDRX timers are set with binary strings that signify a time duration in seconds.
See `Power saving mode setting section in AT commands reference document`_ for a conversion chart of these timer values.

.. note::
   The availability of power saving features or timers is entirely dependent on the cellular network.
   The above recommendations may not be the most current efficient if the network does not support the respective feature.

Configuration files
===================

The sample provides predefined configuration files for the following development kits:

* ``prj.conf`` : For nRF9160 DK and Thingy:91


Building and running
********************

.. |sample path| replace:: :file:`iowa_nrf9160dk_client`

.. include:: /includes/build_and_run.txt

.. include:: /includes/spm.txt

Testing
=======

After programming the sample to your device, test it by performing the following steps:

1. |connect_kit|
#. |connect_terminal|
#. Observe that the sample shows the :ref:`UART output <uart_output>` from the device.
   Note that this is an example and the output need not be identical to your observed output.

.. note::
   Logging output is disabled by default in this sample in order to produce the lowest possible amount of current consumption.
   To enable logging, set the :option:`CONFIG_SERIAL` option in the ``prj.conf`` and ``spm.conf`` configuration files.

.. _uart_output:

Sample output
=============

The following serial UART output is displayed in the terminal emulator:

.. code-block:: console

      *** Booting Zephyr OS build v2.4.0-ncs2  ***
      **************************************
      ** Iowa sample client for nrf9160DK **
      ** (c)IoTerop 2021                  **
      **************************************
      Server:  coap://iowa-server.ioterop.com:5683
      Endpoint Name: IOWA_Nrf9160DK_Client
      Connecting celullar network...
      +CEREG: 2,"6602","0118C901",7,0,0,"11100000","11100000"
      LTE cell changed: Cell ID: 18401537, Tracking area: 26114
      PSM parameter update: TAU: -1, Active time: -1
      +CSCON: 1
      RRC mode: Connected
      +CEREG: 5,"6602","0118C901",7,,,"11100000","11100000"
      Network registration status: Connected - roaming

      Start IOWA lwm2m stack

      ===> Thread: Voltage sensor value changed to 33.
         Send update to server...
         prv_sendResultCb():Send operation occurred: 4.00

      ===> Thread: Voltage sensor value changed to 43.
         Send update to server...
         prv_sendResultCb():Send operation occurred: 4.00

      ===> Thread: Voltage sensor value changed to 62.
         Send update to server...
         prv_sendResultCb():Send operation occurred: 4.00


