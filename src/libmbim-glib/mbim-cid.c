/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2013 - 2014 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2022 Intel Corporation
 */

#include "mbim-cid.h"
#include "mbim-uuid.h"
#include "mbim-enum-types.h"

typedef struct {
    gboolean set;
    gboolean query;
    gboolean notify;
} CidConfig;

#define NO_SET    FALSE
#define NO_QUERY  FALSE
#define NO_NOTIFY FALSE

#define SET    TRUE
#define QUERY  TRUE
#define NOTIFY TRUE

/* Note: index of the array is CID-1 */
#define MBIM_CID_BASIC_CONNECT_LAST MBIM_CID_BASIC_CONNECT_MULTICARRIER_PROVIDERS
static const CidConfig cid_basic_connect_config [MBIM_CID_BASIC_CONNECT_LAST] = {
    { NO_SET, QUERY,    NO_NOTIFY }, /* MBIM_CID_BASIC_CONNECT_DEVICE_CAPS */
    { NO_SET, QUERY,    NOTIFY    }, /* MBIM_CID_BASIC_CONNECT_SUBSCRIBER_READY_STATUS */
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_BASIC_CONNECT_RADIO_STATE */
    { SET,    QUERY,    NO_NOTIFY }, /* MBIM_CID_BASIC_CONNECT_PIN */
    { NO_SET, QUERY,    NO_NOTIFY }, /* MBIM_CID_BASIC_CONNECT_PIN_LIST */
    { SET,    QUERY,    NO_NOTIFY }, /* MBIM_CID_BASIC_CONNECT_HOME_PROVIDER */
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_BASIC_CONNECT_PREFERRED_PROVIDERS */
    { NO_SET, QUERY,    NO_NOTIFY }, /* MBIM_CID_BASIC_CONNECT_VISIBLE_PROVIDERS */
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_BASIC_CONNECT_REGISTER_STATE */
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_BASIC_CONNECT_PACKET_SERVICE */
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_BASIC_CONNECT_SIGNAL_STATE */
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_BASIC_CONNECT_CONNECT */
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_BASIC_CONNECT_PROVISIONED_CONTEXTS */
    { SET,    NO_QUERY, NO_NOTIFY }, /* MBIM_CID_BASIC_CONNECT_SERVICE_ACTIVATION */
    { NO_SET, QUERY,    NOTIFY    }, /* MBIM_CID_BASIC_CONNECT_IP_CONFIGURATION */
    { NO_SET, QUERY,    NO_NOTIFY }, /* MBIM_CID_BASIC_CONNECT_DEVICE_SERVICES */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* 17 reserved */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* 18 reserved */
    { SET,    NO_QUERY, NO_NOTIFY }, /* MBIM_CID_BASIC_CONNECT_DEVICE_SERVICE_SUBSCRIBE_LIST */
    { NO_SET, QUERY,    NO_NOTIFY }, /* MBIM_CID_BASIC_CONNECT_PACKET_STATISTICS */
    { SET,    QUERY,    NO_NOTIFY }, /* MBIM_CID_BASIC_CONNECT_NETWORK_IDLE_HINT */
    { NO_SET, QUERY,    NOTIFY    }, /* MBIM_CID_BASIC_CONNECT_EMERGENCY_MODE */
    { SET,    QUERY,    NO_NOTIFY }, /* MBIM_CID_BASIC_CONNECT_IP_PACKET_FILTERS */
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_BASIC_CONNECT_MULTICARRIER_PROVIDERS */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_SMS_LAST MBIM_CID_SMS_MESSAGE_STORE_STATUS
static const CidConfig cid_sms_config [MBIM_CID_SMS_LAST] = {
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_SMS_CONFIGURATION */
    { NO_SET, QUERY,    NOTIFY    }, /* MBIM_CID_SMS_READ */
    { SET,    NO_QUERY, NO_NOTIFY }, /* MBIM_CID_SMS_SEND */
    { SET,    NO_QUERY, NO_NOTIFY }, /* MBIM_CID_SMS_DELETE */
    { NO_SET, QUERY,    NOTIFY    }, /* MBIM_CID_SMS_MESSAGE_STORE_STATUS */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_USSD_LAST MBIM_CID_USSD
static const CidConfig cid_ussd_config [MBIM_CID_USSD_LAST] = {
    { SET, NO_QUERY, NOTIFY }, /* MBIM_CID_USSD */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_PHONEBOOK_LAST MBIM_CID_PHONEBOOK_WRITE
static const CidConfig cid_phonebook_config [MBIM_CID_PHONEBOOK_LAST] = {
    { NO_SET, QUERY,    NOTIFY    }, /* MBIM_CID_PHONEBOOK_CONFIGURATION */
    { NO_SET, QUERY,    NO_NOTIFY }, /* MBIM_CID_PHONEBOOK_READ */
    { SET,    NO_QUERY, NO_NOTIFY }, /* MBIM_CID_PHONEBOOK_DELETE */
    { SET,    NO_QUERY, NO_NOTIFY }, /* MBIM_CID_PHONEBOOK_WRITE */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_STK_LAST MBIM_CID_STK_ENVELOPE
static const CidConfig cid_stk_config [MBIM_CID_STK_LAST] = {
    { SET, QUERY,    NOTIFY    }, /* MBIM_CID_STK_PAC */
    { SET, NO_QUERY, NO_NOTIFY }, /* MBIM_CID_STK_TERMINAL_RESPONSE */
    { SET, QUERY,    NO_NOTIFY }, /* MBIM_CID_STK_ENVELOPE */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_AUTH_LAST MBIM_CID_AUTH_SIM
static const CidConfig cid_auth_config [MBIM_CID_AUTH_LAST] = {
    { NO_SET, QUERY, NO_NOTIFY }, /* MBIM_CID_AUTH_AKA */
    { NO_SET, QUERY, NO_NOTIFY }, /* MBIM_CID_AUTH_AKAP */
    { NO_SET, QUERY, NO_NOTIFY }, /* MBIM_CID_AUTH_SIM */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_DSS_LAST MBIM_CID_DSS_CONNECT
static const CidConfig cid_dss_config [MBIM_CID_DSS_LAST] = {
    { SET, NO_QUERY, NO_NOTIFY }, /* MBIM_CID_DSS_CONNECT */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_MS_FIRMWARE_ID_LAST MBIM_CID_MS_FIRMWARE_ID_GET
static const CidConfig cid_ms_firmware_id_config [MBIM_CID_MS_FIRMWARE_ID_LAST] = {
    { NO_SET, QUERY, NO_NOTIFY }, /* MBIM_CID_MS_FIRMWARE_ID_GET */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_MS_HOST_SHUTDOWN_LAST MBIM_CID_MS_HOST_SHUTDOWN_NOTIFY
static const CidConfig cid_ms_host_shutdown_config [MBIM_CID_MS_HOST_SHUTDOWN_LAST] = {
    { SET, NO_QUERY, NO_NOTIFY }, /* MBIM_CID_MS_HOST_SHUTDOWN_NOTIFY */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_MS_SAR_LAST MBIM_CID_MS_SAR_TRANSMISSION_STATUS
static const CidConfig cid_ms_sar_config [MBIM_CID_MS_SAR_LAST] = {
    { SET, QUERY, NOTIFY }, /* MBIM_CID_MS_SAR_CONFIG */
    { SET, QUERY, NOTIFY }, /* MBIM_CID_MS_SAR_TRANSMISSION_STATUS */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_PROXY_CONTROL_LAST MBIM_CID_PROXY_CONTROL_VERSION
static const CidConfig cid_proxy_control_config [MBIM_CID_PROXY_CONTROL_LAST] = {
    { SET,    NO_QUERY, NO_NOTIFY }, /* MBIM_CID_PROXY_CONTROL_CONFIGURATION */
    { NO_SET, NO_QUERY, NOTIFY    }, /* MBIM_CID_PROXY_CONTROL_VERSION */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_QMI_LAST MBIM_CID_QMI_MSG
static const CidConfig cid_qmi_config [MBIM_CID_QMI_LAST] = {
    { SET, NO_QUERY, NOTIFY }, /* MBIM_CID_QMI_MSG */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_ATDS_LAST MBIM_CID_ATDS_REGISTER_STATE
static const CidConfig cid_atds_config [MBIM_CID_ATDS_LAST] = {
    { NO_SET, QUERY, NO_NOTIFY }, /* MBIM_CID_ATDS_SIGNAL */
    { NO_SET, QUERY, NO_NOTIFY }, /* MBIM_CID_ATDS_LOCATION */
    { SET,    QUERY, NO_NOTIFY }, /* MBIM_CID_ATDS_OPERATORS */
    { SET,    QUERY, NO_NOTIFY }, /* MBIM_CID_ATDS_RAT */
    { NO_SET, QUERY, NO_NOTIFY }, /* MBIM_CID_ATDS_REGISTER_STATE */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_INTEL_FIRMWARE_UPDATE_LAST MBIM_CID_INTEL_FIRMWARE_UPDATE_MODEM_REBOOT
static const CidConfig cid_intel_firmware_update_config [MBIM_CID_INTEL_FIRMWARE_UPDATE_LAST] = {
    { SET, NO_QUERY, NO_NOTIFY }, /* MBIM_CID_INTEL_FIRMWARE_UPDATE_MODEM_REBOOT */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_LAST MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_REGISTRATION_PARAMETERS
static const CidConfig cid_ms_basic_connect_extensions_config [MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_LAST] = {
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_PROVISIONED_CONTEXTS */
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_NETWORK_DENYLIST */
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_LTE_ATTACH_CONFIG */
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_LTE_ATTACH_STATUS */
    { NO_SET, QUERY,    NO_NOTIFY }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_SYS_CAPS */
    { NO_SET, QUERY,    NO_NOTIFY }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_DEVICE_CAPS */
    { SET,    QUERY,    NO_NOTIFY }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_DEVICE_SLOT_MAPPINGS */
    { NO_SET, QUERY,    NOTIFY    }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_SLOT_INFO_STATUS */
    { NO_SET, QUERY,    NOTIFY    }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_PCO */
    { SET,    NO_QUERY, NO_NOTIFY }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_DEVICE_RESET */
    { NO_SET, QUERY,    NO_NOTIFY }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_BASE_STATIONS_INFO */
    { NO_SET, QUERY,    NOTIFY    }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_LOCATION_INFO_STATUS */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, QUERY,    NO_NOTIFY }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_VERSION */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_REGISTRATION_PARAMETERS */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_QDU_LAST MBIM_CID_QDU_FILE_WRITE
static const CidConfig cid_qdu_config [MBIM_CID_QDU_LAST] = {
    { SET,    QUERY,    NOTIFY    }, /* MBIM_CID_QDU_UPDATE_SESSION */
    { SET,    NO_QUERY, NO_NOTIFY }, /* MBIM_CID_QDU_FILE_OPEN */
    { SET,    NO_QUERY, NO_NOTIFY }, /* MBIM_CID_QDU_FILE_WRITE */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_LAST MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_READ_RECORD
static const CidConfig cid_ms_uicc_low_level_access_config [MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_LAST] = {
    { NO_SET,    QUERY,    NO_NOTIFY    }, /* MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_ATR */
    { SET,    NO_QUERY,    NO_NOTIFY    }, /* MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_OPEN_CHANNEL */
    { SET,    NO_QUERY,    NO_NOTIFY    }, /* MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_CLOSE_CHANNEL */
    { SET,    NO_QUERY,    NO_NOTIFY    }, /* MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_APDU */
    { SET,       QUERY,    NO_NOTIFY    }, /* MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_TERMINAL_CAPABILITY */
    { SET,       QUERY,    NO_NOTIFY    }, /* MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_RESET */
    { NO_SET,    QUERY,    NO_NOTIFY    }, /* MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_APPLICATION_LIST */
    { NO_SET,    QUERY,    NO_NOTIFY    }, /* MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_FILE_STATUS */
    { SET,       QUERY,    NO_NOTIFY    }, /* MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_READ_BINARY */
    { SET,       QUERY,    NO_NOTIFY    }, /* MBIM_CID_MS_UICC_LOW_LEVEL_ACCESS_READ_RECORD */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_QUECTEL_LAST MBIM_CID_QUECTEL_RADIO_STATE
static const CidConfig cid_quectel_config [MBIM_CID_QUECTEL_LAST] = {
    { NO_SET, QUERY, NO_NOTIFY }, /* MBIM_CID_QUECTEL_RADIO_STATE */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_INTEL_THERMAL_RF_LAST MBIM_CID_INTEL_THERMAL_RF_RFIM
static const CidConfig cid_intel_thermal_rf_config [MBIM_CID_INTEL_THERMAL_RF_LAST] = {
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { SET,       QUERY, NOTIFY    }, /* MBIM_CID_INTEL_THERMAL_RF_RFIM */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_MS_VOICE_EXTENSIONS_LAST MBIM_CID_MS_VOICE_EXTENSIONS_NITZ
static const CidConfig cid_ms_voice_extensions_config [MBIM_CID_MS_VOICE_EXTENSIONS_LAST] = {
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET,    QUERY,    NOTIFY }, /* MBIM_CID_MS_VOICE_EXTENSIONS_NITZ */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_INTEL_MUTUAL_AUTHENTICATION_LAST MBIM_CID_INTEL_MUTUAL_AUTHENTICATION_FCC_LOCK
static const CidConfig cid_intel_mutual_authentication_config [MBIM_CID_INTEL_MUTUAL_AUTHENTICATION_LAST] = {
    { SET, QUERY, NO_NOTIFY }, /* MBIM_CID_INTEL_MUTUAL_AUTHENTICATION_FCC_LOCK */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_INTEL_TOOLS_LAST MBIM_CID_INTEL_TOOLS_TRACE_CONFIG
static const CidConfig cid_intel_tools_config [MBIM_CID_INTEL_TOOLS_LAST] = {
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { NO_SET, NO_QUERY, NO_NOTIFY }, /* Unused */
    { SET   , QUERY   , NO_NOTIFY }, /* MBIM_CID_INTEL_TOOLS_TRACE_CONFIG */
};

/* Note: index of the array is CID-1 */
#define MBIM_CID_GOOGLE_LAST MBIM_CID_GOOGLE_CARRIER_LOCK
static const CidConfig cid_google_config [MBIM_CID_GOOGLE_LAST] = {
    { SET, QUERY, NOTIFY }, /* MBIM_CID_GOOGLE_CARRIER_LOCK */
};


gboolean
mbim_cid_can_set (MbimService service,
                  guint       cid)
{
    /* CID = 0 is never a valid command */
    g_return_val_if_fail (cid > 0, FALSE);
    /* Known service required */
    g_return_val_if_fail (service > MBIM_SERVICE_INVALID, FALSE);
    g_return_val_if_fail (service < MBIM_SERVICE_LAST, FALSE);

    switch (service) {
    case MBIM_SERVICE_BASIC_CONNECT:
        return cid_basic_connect_config[cid - 1].set;
    case MBIM_SERVICE_SMS:
        return cid_sms_config[cid - 1].set;
    case MBIM_SERVICE_USSD:
        return cid_ussd_config[cid - 1].set;
    case MBIM_SERVICE_PHONEBOOK:
        return cid_phonebook_config[cid - 1].set;
    case MBIM_SERVICE_STK:
        return cid_stk_config[cid - 1].set;
    case MBIM_SERVICE_AUTH:
        return cid_auth_config[cid - 1].set;
    case MBIM_SERVICE_DSS:
        return cid_dss_config[cid - 1].set;
    case MBIM_SERVICE_MS_FIRMWARE_ID:
        return cid_ms_firmware_id_config[cid - 1].set;
    case MBIM_SERVICE_MS_HOST_SHUTDOWN:
        return cid_ms_host_shutdown_config[cid - 1].set;
    case MBIM_SERVICE_MS_SAR:
        return cid_ms_sar_config[cid - 1].set;
    case MBIM_SERVICE_PROXY_CONTROL:
        return cid_proxy_control_config[cid - 1].set;
    case MBIM_SERVICE_QMI:
        return cid_qmi_config[cid - 1].set;
    case MBIM_SERVICE_ATDS:
        return cid_atds_config[cid - 1].set;
    case MBIM_SERVICE_INTEL_FIRMWARE_UPDATE:
        return cid_intel_firmware_update_config[cid - 1].set;
    case MBIM_SERVICE_QDU:
        return cid_qdu_config[cid - 1].set;
    case MBIM_SERVICE_MS_BASIC_CONNECT_EXTENSIONS:
        return cid_ms_basic_connect_extensions_config[cid - 1].set;
    case MBIM_SERVICE_MS_UICC_LOW_LEVEL_ACCESS:
        return cid_ms_uicc_low_level_access_config[cid - 1].set;
    case MBIM_SERVICE_QUECTEL:
        return cid_quectel_config[cid - 1].set;
    case MBIM_SERVICE_INTEL_THERMAL_RF:
        return cid_intel_thermal_rf_config[cid - 1].set;
    case MBIM_SERVICE_MS_VOICE_EXTENSIONS:
        return cid_ms_voice_extensions_config[cid - 1].set;
    case MBIM_SERVICE_INTEL_MUTUAL_AUTHENTICATION:
        return cid_intel_mutual_authentication_config[cid - 1].set;
    case MBIM_SERVICE_INTEL_TOOLS:
        return cid_intel_tools_config[cid - 1].set;
    case MBIM_SERVICE_GOOGLE:
        return cid_google_config[cid - 1].set;
    case MBIM_SERVICE_INVALID:
    case MBIM_SERVICE_LAST:
    default:
        g_assert_not_reached ();
        return FALSE;
    }
}

gboolean
mbim_cid_can_query (MbimService service,
                    guint       cid)
{
    /* CID = 0 is never a valid command */
    g_return_val_if_fail (cid > 0, FALSE);
    /* Known service required */
    g_return_val_if_fail (service > MBIM_SERVICE_INVALID, FALSE);
    g_return_val_if_fail (service < MBIM_SERVICE_LAST, FALSE);

    switch (service) {
    case MBIM_SERVICE_BASIC_CONNECT:
        return cid_basic_connect_config[cid - 1].query;
    case MBIM_SERVICE_SMS:
        return cid_sms_config[cid - 1].query;
    case MBIM_SERVICE_USSD:
        return cid_ussd_config[cid - 1].query;
    case MBIM_SERVICE_PHONEBOOK:
        return cid_phonebook_config[cid - 1].query;
    case MBIM_SERVICE_STK:
        return cid_stk_config[cid - 1].query;
    case MBIM_SERVICE_AUTH:
        return cid_auth_config[cid - 1].query;
    case MBIM_SERVICE_DSS:
        return cid_dss_config[cid - 1].query;
    case MBIM_SERVICE_MS_FIRMWARE_ID:
        return cid_ms_firmware_id_config[cid - 1].query;
    case MBIM_SERVICE_MS_HOST_SHUTDOWN:
        return cid_ms_host_shutdown_config[cid - 1].query;
    case MBIM_SERVICE_MS_SAR:
        return cid_ms_sar_config[cid - 1].query;
    case MBIM_SERVICE_PROXY_CONTROL:
        return cid_proxy_control_config[cid - 1].query;
    case MBIM_SERVICE_QMI:
        return cid_qmi_config[cid - 1].query;
    case MBIM_SERVICE_ATDS:
        return cid_atds_config[cid - 1].query;
    case MBIM_SERVICE_INTEL_FIRMWARE_UPDATE:
        return cid_intel_firmware_update_config[cid - 1].query;
    case MBIM_SERVICE_QDU:
        return cid_qdu_config[cid - 1].query;
    case MBIM_SERVICE_MS_BASIC_CONNECT_EXTENSIONS:
        return cid_ms_basic_connect_extensions_config[cid - 1].query;
    case MBIM_SERVICE_MS_UICC_LOW_LEVEL_ACCESS:
        return cid_ms_uicc_low_level_access_config[cid - 1].query;
    case MBIM_SERVICE_QUECTEL:
        return cid_quectel_config[cid - 1].query;
    case MBIM_SERVICE_INTEL_THERMAL_RF:
        return cid_intel_thermal_rf_config[cid - 1].query;
    case MBIM_SERVICE_MS_VOICE_EXTENSIONS:
        return cid_ms_voice_extensions_config[cid - 1].query;
    case MBIM_SERVICE_INTEL_MUTUAL_AUTHENTICATION:
        return cid_intel_mutual_authentication_config[cid - 1].query;
    case MBIM_SERVICE_INTEL_TOOLS:
        return cid_intel_tools_config[cid - 1].query;
    case MBIM_SERVICE_GOOGLE:
        return cid_google_config[cid - 1].query;
    case MBIM_SERVICE_INVALID:
    case MBIM_SERVICE_LAST:
    default:
        g_assert_not_reached ();
        return FALSE;
    }
}

gboolean
mbim_cid_can_notify (MbimService service,
                     guint       cid)
{
    /* CID = 0 is never a valid command */
    g_return_val_if_fail (cid > 0, FALSE);
    /* Known service required */
    g_return_val_if_fail (service > MBIM_SERVICE_INVALID, FALSE);
    g_return_val_if_fail (service < MBIM_SERVICE_LAST, FALSE);

    switch (service) {
    case MBIM_SERVICE_BASIC_CONNECT:
        return cid_basic_connect_config[cid - 1].notify;
    case MBIM_SERVICE_SMS:
        return cid_sms_config[cid - 1].notify;
    case MBIM_SERVICE_USSD:
        return cid_ussd_config[cid - 1].notify;
    case MBIM_SERVICE_PHONEBOOK:
        return cid_phonebook_config[cid - 1].notify;
    case MBIM_SERVICE_STK:
        return cid_stk_config[cid - 1].notify;
    case MBIM_SERVICE_AUTH:
        return cid_auth_config[cid - 1].notify;
    case MBIM_SERVICE_DSS:
        return cid_dss_config[cid - 1].notify;
    case MBIM_SERVICE_MS_FIRMWARE_ID:
        return cid_ms_firmware_id_config[cid - 1].notify;
    case MBIM_SERVICE_MS_HOST_SHUTDOWN:
        return cid_ms_host_shutdown_config[cid - 1].notify;
    case MBIM_SERVICE_MS_SAR:
        return cid_ms_sar_config[cid - 1].notify;
    case MBIM_SERVICE_PROXY_CONTROL:
        return cid_proxy_control_config[cid - 1].notify;
    case MBIM_SERVICE_QMI:
        return cid_qmi_config[cid - 1].notify;
    case MBIM_SERVICE_ATDS:
        return cid_atds_config[cid - 1].notify;
    case MBIM_SERVICE_INTEL_FIRMWARE_UPDATE:
        return cid_intel_firmware_update_config[cid - 1].notify;
    case MBIM_SERVICE_QDU:
        return cid_qdu_config[cid - 1].notify;
    case MBIM_SERVICE_MS_BASIC_CONNECT_EXTENSIONS:
        return cid_ms_basic_connect_extensions_config[cid - 1].notify;
    case MBIM_SERVICE_MS_UICC_LOW_LEVEL_ACCESS:
        return cid_ms_uicc_low_level_access_config[cid - 1].notify;
    case MBIM_SERVICE_QUECTEL:
        return cid_quectel_config[cid - 1].notify;
    case MBIM_SERVICE_INTEL_THERMAL_RF:
        return cid_intel_thermal_rf_config[cid - 1].notify;
    case MBIM_SERVICE_MS_VOICE_EXTENSIONS:
        return cid_ms_voice_extensions_config[cid - 1].notify;
    case MBIM_SERVICE_INTEL_MUTUAL_AUTHENTICATION:
        return cid_intel_mutual_authentication_config[cid - 1].notify;
    case MBIM_SERVICE_INTEL_TOOLS:
        return cid_intel_tools_config[cid - 1].notify;
    case MBIM_SERVICE_GOOGLE:
        return cid_google_config[cid - 1].notify;
    case MBIM_SERVICE_INVALID:
    case MBIM_SERVICE_LAST:
    default:
        g_assert_not_reached ();
        return FALSE;
    }
}

const gchar *
mbim_cid_get_printable (MbimService service,
                        guint       cid)
{
    /* CID = 0 is never a valid command */
    g_return_val_if_fail (cid > 0, NULL);
    /* Known service required */
    g_return_val_if_fail (service < MBIM_SERVICE_LAST, NULL);

    switch (service) {
    case MBIM_SERVICE_INVALID:
        return "invalid";
    case MBIM_SERVICE_BASIC_CONNECT:
        return mbim_cid_basic_connect_get_string (cid);
    case MBIM_SERVICE_SMS:
        return mbim_cid_sms_get_string (cid);
    case MBIM_SERVICE_USSD:
        return mbim_cid_ussd_get_string (cid);
    case MBIM_SERVICE_PHONEBOOK:
        return mbim_cid_phonebook_get_string (cid);
    case MBIM_SERVICE_STK:
        return mbim_cid_stk_get_string (cid);
    case MBIM_SERVICE_AUTH:
        return mbim_cid_auth_get_string (cid);
    case MBIM_SERVICE_DSS:
        return mbim_cid_dss_get_string (cid);
    case MBIM_SERVICE_MS_FIRMWARE_ID:
        return mbim_cid_ms_firmware_id_get_string (cid);
    case MBIM_SERVICE_MS_HOST_SHUTDOWN:
        return mbim_cid_ms_host_shutdown_get_string (cid);
    case MBIM_SERVICE_MS_SAR:
        return mbim_cid_ms_sar_get_string (cid);
    case MBIM_SERVICE_PROXY_CONTROL:
        return mbim_cid_proxy_control_get_string (cid);
    case MBIM_SERVICE_QMI:
        return mbim_cid_qmi_get_string (cid);
    case MBIM_SERVICE_ATDS:
        return mbim_cid_atds_get_string (cid);
    case MBIM_SERVICE_INTEL_FIRMWARE_UPDATE:
        return mbim_cid_intel_firmware_update_get_string (cid);
    case MBIM_SERVICE_QDU:
        return mbim_cid_qdu_get_string (cid);
    case MBIM_SERVICE_MS_BASIC_CONNECT_EXTENSIONS:
        return mbim_cid_ms_basic_connect_extensions_get_string (cid);
    case MBIM_SERVICE_MS_UICC_LOW_LEVEL_ACCESS:
        return mbim_cid_ms_uicc_low_level_access_get_string (cid);
    case MBIM_SERVICE_QUECTEL:
        return mbim_cid_quectel_get_string (cid);
    case MBIM_SERVICE_INTEL_THERMAL_RF:
        return mbim_cid_intel_thermal_rf_get_string (cid);
    case MBIM_SERVICE_MS_VOICE_EXTENSIONS:
        return mbim_cid_ms_voice_extensions_get_string (cid);
    case MBIM_SERVICE_INTEL_MUTUAL_AUTHENTICATION:
        return mbim_cid_intel_mutual_authentication_get_string (cid);
    case MBIM_SERVICE_INTEL_TOOLS:
        return mbim_cid_intel_tools_get_string (cid);
    case MBIM_SERVICE_GOOGLE:
        return mbim_cid_google_get_string (cid);
    case MBIM_SERVICE_LAST:
    default:
        g_assert_not_reached ();
        return NULL;
    }
}
