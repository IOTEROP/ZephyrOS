#
# Copyright (c) 2021 IoTerop
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.5)

#For evaluation version (from github) 
set(IOWA_SDK_BASE iowa-sdk/iowa)

#
set(spm_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/spm.conf)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(iowa_udp)

# NORDIC SDK APP START
target_sources( app PRIVATE src/main.c
src/client_platform.c
${IOWA_SDK_BASE}/src/coap/iowa_block.c
${IOWA_SDK_BASE}/src/coap/iowa_coap.c
${IOWA_SDK_BASE}/src/coap/iowa_coap_lorawan.c
${IOWA_SDK_BASE}/src/coap/iowa_coap_sms.c
${IOWA_SDK_BASE}/src/coap/iowa_coap_tcp.c
${IOWA_SDK_BASE}/src/coap/iowa_coap_udp.c
${IOWA_SDK_BASE}/src/coap/iowa_coap_utils.c
${IOWA_SDK_BASE}/src/coap/iowa_message.c
${IOWA_SDK_BASE}/src/coap/iowa_option.c
${IOWA_SDK_BASE}/src/coap/iowa_peer.c
${IOWA_SDK_BASE}/src/coap/iowa_transaction.c
${IOWA_SDK_BASE}/src/comm/iowa_comm.c
${IOWA_SDK_BASE}/src/core/iowa_base.c
${IOWA_SDK_BASE}/src/core/iowa_buffer.c
${IOWA_SDK_BASE}/src/core/iowa_client.c
${IOWA_SDK_BASE}/src/core/iowa_context.c
${IOWA_SDK_BASE}/src/core/iowa_server.c
${IOWA_SDK_BASE}/src/core/iowa_timer.c
${IOWA_SDK_BASE}/src/data/iowa_cbor.c
${IOWA_SDK_BASE}/src/data/iowa_data.c
${IOWA_SDK_BASE}/src/data/iowa_data_utils.c
${IOWA_SDK_BASE}/src/data/iowa_json.c
${IOWA_SDK_BASE}/src/data/iowa_senml_cbor.c
${IOWA_SDK_BASE}/src/data/iowa_senml_json.c
${IOWA_SDK_BASE}/src/data/iowa_text_opaque.c
${IOWA_SDK_BASE}/src/data/iowa_tlv.c
${IOWA_SDK_BASE}/src/logger/iowa_logger.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_acl.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_attributes.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_bootstrap.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_core_link.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_lwm2m.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_lwm2m_utils.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_management.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_objects.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_observe.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_packet.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_registration.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_send.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_uri.c
${IOWA_SDK_BASE}/src/lwm2m/iowa_value.c
${IOWA_SDK_BASE}/src/misc/iowa_list.c
${IOWA_SDK_BASE}/src/misc/iowa_utils.c
${IOWA_SDK_BASE}/src/objects/iowa_object_accelerometer.c
${IOWA_SDK_BASE}/src/objects/iowa_object_acl.c
${IOWA_SDK_BASE}/src/objects/iowa_object_apn_connection_profile.c
${IOWA_SDK_BASE}/src/objects/iowa_object_at_command.c
${IOWA_SDK_BASE}/src/objects/iowa_object_bearer_selection.c
${IOWA_SDK_BASE}/src/objects/iowa_object_cellular_connectivity.c
${IOWA_SDK_BASE}/src/objects/iowa_object_connectivity_monitoring.c
${IOWA_SDK_BASE}/src/objects/iowa_object_connectivity_stats.c
${IOWA_SDK_BASE}/src/objects/iowa_object_device.c
${IOWA_SDK_BASE}/src/objects/iowa_object_digital_output.c

${IOWA_SDK_BASE}/src/objects/iowa_object_firmware.c
${IOWA_SDK_BASE}/src/objects/iowa_object_gps.c
${IOWA_SDK_BASE}/src/objects/iowa_object_gyrometer.c
${IOWA_SDK_BASE}/src/objects/iowa_object_internals.c
${IOWA_SDK_BASE}/src/objects/iowa_object_ipso.c
${IOWA_SDK_BASE}/src/objects/iowa_object_light_control.c
${IOWA_SDK_BASE}/src/objects/iowa_object_location.c
${IOWA_SDK_BASE}/src/objects/iowa_object_magnetometer.c
${IOWA_SDK_BASE}/src/objects/iowa_object_oscore.c
${IOWA_SDK_BASE}/src/objects/iowa_object_security.c
${IOWA_SDK_BASE}/src/objects/iowa_object_server.c
${IOWA_SDK_BASE}/src/objects/iowa_object_software_component.c
${IOWA_SDK_BASE}/src/objects/iowa_object_software_management.c
${IOWA_SDK_BASE}/src/oscore/iowa_cose.c
${IOWA_SDK_BASE}/src/oscore/iowa_oscore.c
${IOWA_SDK_BASE}/src/security/iowa_layer_mbedtls.c
${IOWA_SDK_BASE}/src/security/iowa_layer_tinydtls.c
${IOWA_SDK_BASE}/src/security/iowa_security.c
${IOWA_SDK_BASE}/src/security/iowa_security_helper.c

${IOWA_SDK_BASE}/src/core/iowa_comms.c      #Specific Eval version

## Not on EVAL ${IOWA_SDK_BASE}/src/objects/iowa_object_dimmer.c
## Not on EVAL ${IOWA_SDK_BASE}/src/objects/iowa_object_mqtt_broker.c
## Not on EVAL ${IOWA_SDK_BASE}/src/objects/iowa_object_mqtt_publication.c
)

# NORDIC SDK APP END

zephyr_include_directories(src
${IOWA_SDK_BASE}/include
${IOWA_SDK_BASE}/include/objects
${IOWA_SDK_BASE}/src/coap
${IOWA_SDK_BASE}/src/comm
${IOWA_SDK_BASE}/src/core
${IOWA_SDK_BASE}/src/data
${IOWA_SDK_BASE}/src/logger
${IOWA_SDK_BASE}/src/lwm2m
${IOWA_SDK_BASE}/src/misc
${IOWA_SDK_BASE}/src/objects
${IOWA_SDK_BASE}/src/oscore
${IOWA_SDK_BASE}/src/security
)
