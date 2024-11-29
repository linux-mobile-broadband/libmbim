/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2013 - 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <gio/gio.h>

#include <libmbim-glib.h>

#include "mbim-common.h"
#include "mbimcli.h"
#include "mbimcli-helpers.h"

/* Context */
typedef struct {
    MbimDevice *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean  query_device_caps_flag;
static gboolean  query_subscriber_ready_status_flag;
static gboolean  query_radio_state_flag;
static gchar    *set_radio_state_str;
static gboolean  query_device_services_flag;
static gboolean  query_pin_flag;
static gchar    *set_pin_enter_str;
static gchar    *set_pin_change_str;
static gchar    *set_pin_enable_str;
static gchar    *set_pin_disable_str;
static gchar    *set_pin_enter_puk_str;
static gboolean  query_pin_list_flag;
static gboolean  query_home_provider_flag;
static gboolean  query_preferred_providers_flag;
static gboolean  query_visible_providers_flag;
static gboolean  query_register_state_flag;
static gboolean  set_register_state_automatic_flag;
static gboolean  query_signal_state_flag;
static gboolean  query_packet_service_flag;
static gboolean  set_packet_service_attach_flag;
static gboolean  set_packet_service_detach_flag;
static gchar    *query_connect_str;
static gchar    *set_connect_activate_str;
static gchar    *query_ip_configuration_str;
static gchar    *set_connect_deactivate_str;
static gboolean  query_packet_statistics_flag;
static gchar    *query_ip_packet_filters_str;
static gchar    *set_ip_packet_filters_str;
static gboolean  query_provisioned_contexts_flag;
static gchar    *set_provisioned_contexts_str;
static gchar    *set_signal_state_str;
static gchar    *set_network_idle_hint_str;
static gboolean  query_network_idle_hint_flag;
static gchar    *set_emergency_mode_str;
static gboolean  query_emergency_mode_flag;
static gchar    *set_service_activation_str;

static gboolean query_connection_state_arg_parse (const char *option_name,
                                                  const char *value,
                                                  gpointer user_data,
                                                  GError **error);

static gboolean query_ip_configuration_arg_parse (const char *option_name,
                                                  const char *value,
                                                  gpointer user_data,
                                                  GError **error);

static gboolean disconnect_arg_parse (const char *option_name,
                                      const char *value,
                                      gpointer user_data,
                                      GError **error);

static gboolean query_ip_packet_filters_arg_parse (const char *option_name,
                                                   const char *value,
                                                   gpointer user_data,
                                                   GError **error);

static GOptionEntry entries[] = {
    { "query-device-caps", 0, 0, G_OPTION_ARG_NONE, &query_device_caps_flag,
      "Query device capabilities",
      NULL
    },
    { "query-subscriber-ready-status", 0, 0, G_OPTION_ARG_NONE, &query_subscriber_ready_status_flag,
      "Query subscriber ready status",
      NULL
    },
    { "query-radio-state", 0, 0, G_OPTION_ARG_NONE, &query_radio_state_flag,
      "Query radio state",
      NULL
    },
    { "set-radio-state", 0, 0, G_OPTION_ARG_STRING, &set_radio_state_str,
      "Set radio state",
      "[(on|off)]"
    },
    { "query-device-services", 0, 0, G_OPTION_ARG_NONE, &query_device_services_flag,
      "Query device services",
      NULL
    },
    { "query-pin-state", 0, 0, G_OPTION_ARG_NONE, &query_pin_flag,
      "Query PIN state",
      NULL
    },
    { "enter-pin", 0, 0, G_OPTION_ARG_STRING, &set_pin_enter_str,
      "Enter PIN (PIN type is optional, defaults to PIN1, allowed options: "
        "(pin1,network-pin,network-subset-pin,service-provider-pin,corporate-pin)",
      "[(PIN type),(current PIN)]"
    },
    { "change-pin", 0, 0, G_OPTION_ARG_STRING, &set_pin_change_str,
      "Change PIN",
      "[(current PIN),(new PIN)]"
    },
    { "enable-pin", 0, 0, G_OPTION_ARG_STRING, &set_pin_enable_str,
      "Enable PIN",
      "[(current PIN)]"
    },
    { "disable-pin", 0, 0, G_OPTION_ARG_STRING, &set_pin_disable_str,
      "Disable PIN (PIN type is optional, see enter-pin for details)",
      "[(PIN type),(current PIN)]"
    },
    { "enter-puk", 0, 0, G_OPTION_ARG_STRING, &set_pin_enter_puk_str,
      "Enter PUK (PUK type is optional, defaults to PUK1, allowed options: "
        "(puk1,network-puk,network-subset-puk,service-provider-puk,corporate-puk)",
      "[(PUK type),(PUK),(new PIN)]"
    },
    { "query-pin-list", 0, 0, G_OPTION_ARG_NONE, &query_pin_list_flag,
      "Query PIN list",
      NULL
    },
    { "query-home-provider", 0, 0, G_OPTION_ARG_NONE, &query_home_provider_flag,
      "Query home provider",
      NULL
    },
    { "query-preferred-providers", 0, 0, G_OPTION_ARG_NONE, &query_preferred_providers_flag,
      "Query preferred providers",
      NULL
    },
    { "query-visible-providers", 0, 0, G_OPTION_ARG_NONE, &query_visible_providers_flag,
      "Query visible providers",
      NULL
    },
    { "query-registration-state", 0, 0, G_OPTION_ARG_NONE, &query_register_state_flag,
      "Query registration state",
      NULL
    },
    { "register-automatic", 0, 0, G_OPTION_ARG_NONE, &set_register_state_automatic_flag,
      "Launch automatic registration",
      NULL
    },
    { "query-signal-state", 0, 0, G_OPTION_ARG_NONE, &query_signal_state_flag,
      "Query signal state",
      NULL
    },
    { "query-packet-service-state", 0, 0, G_OPTION_ARG_NONE, &query_packet_service_flag,
      "Query packet service state",
      NULL
    },
    { "attach-packet-service", 0, 0, G_OPTION_ARG_NONE, &set_packet_service_attach_flag,
      "Attach to the packet service",
      NULL
    },
    { "detach-packet-service", 0, 0, G_OPTION_ARG_NONE, &set_packet_service_detach_flag,
      "Detach from the packet service",
      NULL
    },
    { "query-connection-state", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, G_CALLBACK (query_connection_state_arg_parse),
      "Query connection state (SessionID is optional, defaults to 0)",
      "[SessionID]"
    },
    { "connect", 0, 0, G_OPTION_ARG_STRING, &set_connect_activate_str,
      "Connect (allowed keys: session-id, access-string, ip-type, auth, username, password, compression, context-type)",
      "[\"key=value,...\"]"
    },
    { "query-ip-configuration", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, G_CALLBACK (query_ip_configuration_arg_parse),
      "Query IP configuration (SessionID is optional, defaults to 0)",
      "[SessionID]"
    },
    { "disconnect", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, G_CALLBACK (disconnect_arg_parse),
      "Disconnect (SessionID is optional, defaults to 0)",
      "[SessionID]"
    },
    { "query-packet-statistics", 0, 0, G_OPTION_ARG_NONE, &query_packet_statistics_flag,
      "Query packet statistics",
      NULL
    },
    { "query-ip-packet-filters", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, G_CALLBACK (query_ip_packet_filters_arg_parse),
      "Query IP packet filters (SessionID is optional, defaults to 0)",
      "[SessionID]"
    },
    { "set-ip-packet-filters", 0, 0, G_OPTION_ARG_STRING, &set_ip_packet_filters_str,
      "Set IP packet filters (allowed keys: session-id, packet-filter, packet-mask, filter-id)",
      "[\"key=value,...\"]"
    },
    { "query-provisioned-contexts", 0, 0, G_OPTION_ARG_NONE, &query_provisioned_contexts_flag,
      "Query provisioned contexts",
      NULL
    },
    { "set-provisioned-contexts", 0, 0, G_OPTION_ARG_STRING, &set_provisioned_contexts_str,
      "Set provisioned contexts (allowed keys: context-id, context-type, auth, compression, username, password, access-string, provider-id)",
      "[\"key=value,...\"]"
    },
    { "set-signal-state", 0, 0, G_OPTION_ARG_STRING, &set_signal_state_str,
      "Set signal state (allowed keys: signal-strength-interval, rssi-threshold, error-rate-threshold)",
      "[\"key=value,...\"]"
    },
    { "set-network-idle-hint", 0, 0, G_OPTION_ARG_STRING, &set_network_idle_hint_str,
      "Set network idle hint",
      "[(enabled|disabled)]"
    },
    { "query-network-idle-hint", 0, 0, G_OPTION_ARG_NONE, &query_network_idle_hint_flag,
      "Query network idle hint",
      NULL
    },
    { "set-emergency-mode", 0, 0, G_OPTION_ARG_STRING, &set_emergency_mode_str,
      "Set emergency mode",
      "[(on|off)]"
    },
    { "query-emergency-mode", 0, 0, G_OPTION_ARG_NONE, &query_emergency_mode_flag,
      "Query emergency mode",
      NULL
    },
    { "set-service-activation", 0, 0, G_OPTION_ARG_STRING, &set_service_activation_str,
      "Set service activation",
      "[Data]"
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

GOptionGroup *
mbimcli_basic_connect_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("basic-connect",
                                "Basic Connect options:",
                                "Show Basic Connect Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

static gboolean
query_connection_state_arg_parse (const char *option_name,
                                  const char *value,
                                  gpointer user_data,
                                  GError **error)
{
    query_connect_str = g_strdup (value ? value : "0");
    return TRUE;
}

static gboolean
query_ip_configuration_arg_parse (const char *option_name,
                                  const char *value,
                                  gpointer user_data,
                                  GError **error)
{
    query_ip_configuration_str = g_strdup (value ? value : "0");
    return TRUE;
}

static gboolean
disconnect_arg_parse (const char *option_name,
                      const char *value,
                      gpointer user_data,
                      GError **error)
{
    set_connect_deactivate_str = g_strdup (value ? value : "0");
    return TRUE;
}

static gboolean
query_ip_packet_filters_arg_parse (const char *option_name,
                                   const char *value,
                                   gpointer user_data,
                                   GError **error)
{
    query_ip_packet_filters_str = g_strdup (value ? value : "0");
    return TRUE;
}

gboolean
mbimcli_basic_connect_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (query_device_caps_flag +
                 query_subscriber_ready_status_flag +
                 query_radio_state_flag +
                 !!set_radio_state_str +
                 query_device_services_flag +
                 query_pin_flag +
                 !!set_pin_enter_str +
                 !!set_pin_change_str +
                 !!set_pin_enable_str +
                 !!set_pin_disable_str +
                 !!set_pin_enter_puk_str +
                 query_pin_list_flag +
                 query_register_state_flag +
                 query_home_provider_flag +
                 query_preferred_providers_flag +
                 query_visible_providers_flag +
                 set_register_state_automatic_flag +
                 query_signal_state_flag +
                 query_packet_service_flag +
                 set_packet_service_attach_flag +
                 set_packet_service_detach_flag +
                 !!query_connect_str +
                 !!set_connect_activate_str +
                 !!query_ip_configuration_str +
                 !!set_connect_deactivate_str +
                 query_packet_statistics_flag +
                 !!query_ip_packet_filters_str +
                 !!set_ip_packet_filters_str +
                 query_provisioned_contexts_flag +
                 !!set_provisioned_contexts_str +
                 !!set_signal_state_str +
                 !!set_network_idle_hint_str +
                 query_network_idle_hint_flag +
                 !!set_emergency_mode_str +
                 query_emergency_mode_flag +
                 !!set_service_activation_str);

    if (n_actions > 1) {
        g_printerr ("error: too many Basic Connect actions requested\n");
        exit (EXIT_FAILURE);
    }

    checked = TRUE;
    return !!n_actions;
}

static void
context_free (Context *context)
{
    if (!context)
        return;

    if (context->cancellable)
        g_object_unref (context->cancellable);
    if (context->device)
        g_object_unref (context->device);
    g_slice_free (Context, context);
}

static void
shutdown (gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    context_free (ctx);
    mbimcli_async_operation_done (operation_status);
}

static void
query_device_caps_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    MbimDeviceType          device_type;
    const gchar            *device_type_str;
    MbimVoiceClass          voice_class;
    const gchar            *voice_class_str;
    MbimCellularClass       cellular_class;
    g_autofree gchar       *cellular_class_str = NULL;
    MbimSimClass            sim_class;
    g_autofree gchar       *sim_class_str = NULL;
    MbimDataClass           data_class;
    g_autofree gchar       *data_class_str = NULL;
    MbimSmsCaps             sms_caps;
    g_autofree gchar       *sms_caps_str = NULL;
    MbimCtrlCaps            ctrl_caps;
    g_autofree gchar       *ctrl_caps_str = NULL;
    guint32                 max_sessions;
    g_autofree gchar       *custom_data_class = NULL;
    g_autofree gchar       *device_id = NULL;
    g_autofree gchar       *firmware_info = NULL;
    g_autofree gchar       *hardware_info = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_device_caps_response_parse (
            response,
            &device_type,
            &cellular_class,
            &voice_class,
            &sim_class,
            &data_class,
            &sms_caps,
            &ctrl_caps,
            &max_sessions,
            &custom_data_class,
            &device_id,
            &firmware_info,
            &hardware_info,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    device_type_str    = mbim_device_type_get_string (device_type);
    cellular_class_str = mbim_cellular_class_build_string_from_mask (cellular_class);
    voice_class_str    = mbim_voice_class_get_string (voice_class);
    sim_class_str      = mbim_sim_class_build_string_from_mask (sim_class);
    data_class_str     = mbim_data_class_build_string_from_mask (data_class);
    sms_caps_str       = mbim_sms_caps_build_string_from_mask (sms_caps);
    ctrl_caps_str      = mbim_ctrl_caps_build_string_from_mask (ctrl_caps);

    g_print ("[%s] Device capabilities retrieved:\n"
             "\t      Device type: '%s'\n"
             "\t   Cellular class: '%s'\n"
             "\t      Voice class: '%s'\n"
             "\t        SIM class: '%s'\n"
             "\t       Data class: '%s'\n"
             "\t         SMS caps: '%s'\n"
             "\t        Ctrl caps: '%s'\n"
             "\t     Max sessions: '%u'\n"
             "\tCustom data class: '%s'\n"
             "\t        Device ID: '%s'\n"
             "\t    Firmware info: '%s'\n"
             "\t    Hardware info: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (device_type_str),
             VALIDATE_UNKNOWN (cellular_class_str),
             VALIDATE_UNKNOWN (voice_class_str),
             VALIDATE_UNKNOWN (sim_class_str),
             VALIDATE_UNKNOWN (data_class_str),
             VALIDATE_UNKNOWN (sms_caps_str),
             VALIDATE_UNKNOWN (ctrl_caps_str),
             max_sessions,
             VALIDATE_UNKNOWN (custom_data_class),
             VALIDATE_UNKNOWN (device_id),
             VALIDATE_UNKNOWN (firmware_info),
             VALIDATE_UNKNOWN (hardware_info));

    shutdown (TRUE);
}

static void
query_subscriber_ready_status_ready (MbimDevice   *device,
                                     GAsyncResult *res)
{
    g_autoptr (MbimMessage)        response = NULL;
    g_autoptr (GError)             error = NULL;
    MbimSubscriberReadyState       ready_state;
    const gchar                   *ready_state_str;
    g_autofree gchar              *subscriber_id = NULL;
    g_autofree gchar              *sim_iccid = NULL;
    MbimReadyInfoFlag              ready_info;
    g_autofree gchar              *ready_info_str = NULL;
    guint32                        telephone_numbers_count;
    g_auto(GStrv)                  telephone_numbers = NULL;
    g_autofree gchar              *telephone_numbers_str = NULL;
    MbimSubscriberReadyStatusFlag  flags;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    /* MBIMEx 3.0 support */
    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        if (!mbim_message_ms_basic_connect_v3_subscriber_ready_status_response_parse (response,
                                                                                      &ready_state,
                                                                                      &flags,
                                                                                      &subscriber_id,
                                                                                      &sim_iccid,
                                                                                      &ready_info,
                                                                                      &telephone_numbers_count,
                                                                                      &telephone_numbers,
                                                                                      &error)) {

            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        g_debug ("Successfully parsed response as MBIMEx 3.0 Subscriber State");
    }
    /* MBIM 1.0 support */
    else {
        if (!mbim_message_subscriber_ready_status_response_parse (response,
                                                                  &ready_state,
                                                                  &subscriber_id,
                                                                  &sim_iccid,
                                                                  &ready_info,
                                                                  &telephone_numbers_count,
                                                                  &telephone_numbers,
                                                                  &error)) {

            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
         }
        g_debug ("Successfully parsed response as MBIM 1.0 Subscriber State");
    }

    telephone_numbers_str = (telephone_numbers ? g_strjoinv (", ", telephone_numbers) : NULL);
    ready_state_str = mbim_subscriber_ready_state_get_string (ready_state);
    ready_info_str = mbim_ready_info_flag_build_string_from_mask (ready_info);

    g_print ("[%s] Subscriber ready status retrieved:\n"
             "\t      Ready state: '%s'\n"
             "\t    Subscriber ID: '%s'\n"
             "\t        SIM ICCID: '%s'\n"
             "\t       Ready info: '%s'\n"
             "\tTelephone numbers: (%u) '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (ready_state_str),
             VALIDATE_UNKNOWN (subscriber_id),
             VALIDATE_UNKNOWN (sim_iccid),
             VALIDATE_UNKNOWN (ready_info_str),
             telephone_numbers_count, VALIDATE_UNKNOWN (telephone_numbers_str));

    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        g_autofree gchar *flags_str = NULL;

        flags_str = mbim_subscriber_ready_status_flag_build_string_from_mask (flags);
        g_print ("\t            Flags: '%s'\n",
                 VALIDATE_UNKNOWN (flags_str));
    }

    shutdown (TRUE);
}

static void
query_radio_state_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    MbimRadioSwitchState    hardware_radio_state;
    const gchar            *hardware_radio_state_str;
    MbimRadioSwitchState    software_radio_state;
    const gchar            *software_radio_state_str;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_radio_state_response_parse (
            response,
            &hardware_radio_state,
            &software_radio_state,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    hardware_radio_state_str = mbim_radio_switch_state_get_string (hardware_radio_state);
    software_radio_state_str = mbim_radio_switch_state_get_string (software_radio_state);

    g_print ("[%s] Radio state retrieved:\n"
             "\t     Hardware radio state: '%s'\n"
             "\t     Software radio state: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (hardware_radio_state_str),
             VALIDATE_UNKNOWN (software_radio_state_str));

    shutdown (TRUE);
}

static void
query_device_services_ready (MbimDevice   *device,
                             GAsyncResult *res)
{
    g_autoptr(MbimMessage)                   response = NULL;
    g_autoptr(GError)                        error = NULL;
    g_autoptr(MbimDeviceServiceElementArray) device_services = NULL;
    guint32                                  device_services_count;
    guint32                                  max_dss_sessions;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_device_services_response_parse (
            response,
            &device_services_count,
            &max_dss_sessions,
            &device_services,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        return;
    }

    g_print ("[%s] Device services retrieved:\n"
             "\tMax DSS sessions: '%u'\n",
             mbim_device_get_path_display (device),
             max_dss_sessions);
    if (device_services_count == 0)
        g_print ("\t        Services: None\n");
    else {
        guint32 i;

        g_print ("\t        Services: (%u)\n", device_services_count);
        for (i = 0; i < device_services_count; i++) {
            MbimService         service;
            g_autofree gchar   *uuid_str = NULL;
            g_autoptr(GString)  cids = NULL;
            guint32             j;

            service = mbim_uuid_to_service (&device_services[i]->device_service_id);
            uuid_str = mbim_uuid_get_printable (&device_services[i]->device_service_id);

            cids = g_string_new ("");
            for (j = 0; j < device_services[i]->cids_count; j++) {
                if (service == MBIM_SERVICE_INVALID) {
                    g_string_append_printf (cids, "%u", device_services[i]->cids[j]);
                    if (j < device_services[i]->cids_count - 1)
                        g_string_append (cids, ", ");
                } else {
                    g_string_append_printf (cids, "%s%s (%u)",
                                            j == 0 ? "" : "\t\t                   ",
                                            VALIDATE_UNKNOWN (mbim_cid_get_printable (service, device_services[i]->cids[j])),
                                            device_services[i]->cids[j]);
                    if (j < device_services[i]->cids_count - 1)
                        g_string_append (cids, ",\n");
                }
            }

            g_print ("\n"
                     "\t\t          Service: '%s'\n"
                     "\t\t             UUID: [%s]:\n"
                     "\t\t      DSS payload: %u\n"
                     "\t\tMax DSS instances: %u\n"
                     "\t\t             CIDs: %s\n",
                     service == MBIM_SERVICE_INVALID ? "unknown" : mbim_service_get_string (service),
                     uuid_str,
                     device_services[i]->dss_payload,
                     device_services[i]->max_dss_instances,
                     cids->str);
        }
    }

    shutdown (TRUE);
}

static void
pin_ready (MbimDevice   *device,
           GAsyncResult *res,
           gpointer      user_data)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    MbimPinType             pin_type;
    MbimPinState            pin_state;
    const gchar            *pin_state_str;
    guint32                 remaining_attempts;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_pin_response_parse (
            response,
            &pin_type,
            &pin_state,
            &remaining_attempts,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (GPOINTER_TO_UINT (user_data))
        g_print ("[%s] PIN operation successful\n\n",
                 mbim_device_get_path_display (device));

    pin_state_str = mbim_pin_state_get_string (pin_state);

    g_print ("[%s] PIN info:\n"
             "\t         PIN state: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (pin_state_str));
    if (pin_type != MBIM_PIN_TYPE_UNKNOWN) {
        const gchar *pin_type_str;

        pin_type_str = mbim_pin_type_get_string (pin_type);
        g_print ("\t          PIN type: '%s'\n"
                 "\tRemaining attempts: '%u'\n",
                 VALIDATE_UNKNOWN (pin_type_str),
                 remaining_attempts);
    }

    shutdown (TRUE);
}

static gboolean
set_pin_input_parse (const gchar  *str,
                     gchar       **pin,
                     gchar       **new_pin,
                     MbimPinType  *pin_type)
{
    g_auto(GStrv)     split = NULL;
    guint             n_min, n_max, n = 0;
    MbimPinType       new_pin_type;
    g_autoptr(GError) error = NULL;

    g_assert (pin != NULL);
    n_min = (new_pin ? 2 : 1);
    n_max = n_min + (pin_type ? 1 : 0);

    /* Format of the string is:
     *    "[(current PIN)]"
     * or:
     *    "[(PIN name),(current PIN)]"
     * or:
     *    "[(current PIN),(new PIN)]"
     * or:
     *    "[(PUK name),(PUK),(new PIN)]"
     */
    split = g_strsplit (str, ",", -1);

    if (g_strv_length (split) > n_max) {
        g_printerr ("error: couldn't parse input string, too many arguments\n");
        return FALSE;
    }

    if (g_strv_length (split) < n_min) {
        g_printerr ("error: couldn't parse input string, missing arguments\n");
        return FALSE;
    }

    if (pin_type && (g_strv_length (split) == n_max)) {
        const gchar *pin_type_str;

        pin_type_str = split[n++];
        if (!mbimcli_read_pin_type_from_string (pin_type_str, &new_pin_type, &error)) {
            g_printerr ("error: couldn't parse input pin-type: %s\n", error->message);
            return FALSE;
        }
        if (new_pin_type == MBIM_PIN_TYPE_UNKNOWN ||
            (*pin_type == MBIM_PIN_TYPE_PIN1 && new_pin_type >= MBIM_PIN_TYPE_PUK1) ||
            (*pin_type == MBIM_PIN_TYPE_PUK1 && new_pin_type < MBIM_PIN_TYPE_PUK1)) {
            g_printerr ("error: couldn't parse input string, invalid PIN type\n");
            return FALSE;
        }
        *pin_type = new_pin_type;
    }

    *pin = g_strdup (split[n++]);
    if (new_pin)
        *new_pin = g_strdup (split[n]);

    return TRUE;
}

enum {
    CONNECTION_STATUS,
    CONNECT,
    DISCONNECT
};

static void
print_pin_desc (const gchar       *pin_name,
                const MbimPinDesc *pin_desc)
{
    g_print ("\t%s:\n"
             "\t\t      Mode: '%s'\n"
             "\t\t    Format: '%s'\n"
             "\t\tMin length: '%d'\n"
             "\t\tMax length: '%d'\n"
             "\n",
             pin_name,
             VALIDATE_UNKNOWN (mbim_pin_mode_get_string (pin_desc->pin_mode)),
             VALIDATE_UNKNOWN (mbim_pin_format_get_string (pin_desc->pin_format)),
             pin_desc->pin_length_min,
             pin_desc->pin_length_max);
}

static void
pin_list_ready (MbimDevice   *device,
                GAsyncResult *res,
                gpointer      user_data)
{
    g_autoptr(MbimMessage) response = NULL;
    g_autoptr(GError)      error = NULL;
    g_autoptr(MbimPinDesc) pin_desc_pin1 = NULL;
    g_autoptr(MbimPinDesc) pin_desc_pin2 = NULL;
    g_autoptr(MbimPinDesc) pin_desc_device_sim_pin = NULL;
    g_autoptr(MbimPinDesc) pin_desc_device_first_sim_pin = NULL;
    g_autoptr(MbimPinDesc) pin_desc_network_pin = NULL;
    g_autoptr(MbimPinDesc) pin_desc_network_subset_pin = NULL;
    g_autoptr(MbimPinDesc) pin_desc_service_provider_pin = NULL;
    g_autoptr(MbimPinDesc) pin_desc_corporate_pin = NULL;
    g_autoptr(MbimPinDesc) pin_desc_subsidy_lock = NULL;
    g_autoptr(MbimPinDesc) pin_desc_custom = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_pin_list_response_parse (
            response,
            &pin_desc_pin1,
            &pin_desc_pin2,
            &pin_desc_device_sim_pin,
            &pin_desc_device_first_sim_pin,
            &pin_desc_network_pin,
            &pin_desc_network_subset_pin,
            &pin_desc_service_provider_pin,
            &pin_desc_corporate_pin,
            &pin_desc_subsidy_lock,
            &pin_desc_custom,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN list:\n\n",
             mbim_device_get_path_display (device));

    print_pin_desc ("PIN1", pin_desc_pin1);
    print_pin_desc ("PIN2", pin_desc_pin2);
    print_pin_desc ("Device SIM PIN", pin_desc_device_sim_pin);
    print_pin_desc ("Device first SIM PIN", pin_desc_device_first_sim_pin);
    print_pin_desc ("Network PIN", pin_desc_network_pin);
    print_pin_desc ("Network subset PIN", pin_desc_network_subset_pin);
    print_pin_desc ("Service provider PIN", pin_desc_service_provider_pin);
    print_pin_desc ("Corporate PIN", pin_desc_corporate_pin);
    print_pin_desc ("Subsidy lock", pin_desc_subsidy_lock);
    print_pin_desc ("Custom", pin_desc_custom);

    shutdown (TRUE);
}

static void
ip_configuration_query_ready (MbimDevice   *device,
                              GAsyncResult *res)
{
    g_autoptr(GError)      error = NULL;
    g_autoptr(MbimMessage) response;
    gboolean               success = FALSE;

    response = mbim_device_command_finish (device, res, &error);
    if (!response ||
        !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: couldn't get IP configuration response message: %s\n", error->message);
    } else {
        success = mbimcli_print_ip_config (device, response, &error);
        if (!success)
            g_printerr ("error: couldn't parse IP configuration response message: %s\n", error->message);
    }

    shutdown (success);
}

static void
ip_configuration_query (MbimDevice   *device,
                        GCancellable *cancellable,
                        guint32       session_id)
{
    g_autoptr(MbimMessage) message = NULL;
    g_autoptr(GError)      error = NULL;

    message = (mbim_message_ip_configuration_query_new (
                   session_id,
                   MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_NONE, /* ipv4configurationavailable */
                   MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_NONE, /* ipv6configurationavailable */
                   0, /* ipv4addresscount */
                   NULL, /* ipv4address */
                   0, /* ipv6addresscount */
                   NULL, /* ipv6address */
                   NULL, /* ipv4gateway */
                   NULL, /* ipv6gateway */
                   0, /* ipv4dnsservercount */
                   NULL, /* ipv4dnsserver */
                   0, /* ipv6dnsservercount */
                   NULL, /* ipv6dnsserver */
                   0, /* ipv4mtu */
                   0, /* ipv6mtu */
                   &error));
    if (!message) {
        g_printerr ("error: couldn't create IP config request: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    mbim_device_command (device,
                         message,
                         60,
                         cancellable,
                         (GAsyncReadyCallback)ip_configuration_query_ready,
                         NULL);
}

static void
connect_ready (MbimDevice   *device,
               GAsyncResult *res,
               gpointer      user_data)
{
    g_autoptr(MbimMessage)   response = NULL;
    g_autoptr(GError)        error = NULL;
    guint32                  session_id;
    MbimActivationState      activation_state;
    MbimVoiceCallState       voice_call_state;
    MbimContextIpType        ip_type;
    MbimAccessMediaType      media_type = MBIM_ACCESS_MEDIA_TYPE_UNKNOWN;
    g_autofree gchar        *access_string = NULL;
    const MbimUuid          *context_type;
    guint32                  nw_error;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        if (!mbim_message_ms_basic_connect_v3_connect_response_parse (
                response,
                &session_id,
                &activation_state,
                &voice_call_state,
                &ip_type,
                &context_type,
                &nw_error,
                &media_type,
                &access_string,
                NULL, /* unnamed IEs ignored */
                &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
    } else {
        if (!mbim_message_connect_response_parse (
                response,
                &session_id,
                &activation_state,
                &voice_call_state,
                &ip_type,
                &context_type,
                &nw_error,
                &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
    }

    switch (GPOINTER_TO_UINT (user_data)) {
    case CONNECT:
        g_print ("[%s] Successfully connected\n\n",
                 mbim_device_get_path_display (device));
        break;
    case DISCONNECT:
        g_print ("[%s] Successfully disconnected\n\n",
                 mbim_device_get_path_display (device));
        break;
    default:
        break;
    }

    g_print ("[%s] Connection status:\n"
             "\t      Session ID: '%u'\n"
             "\tActivation state: '%s'\n"
             "\tVoice call state: '%s'\n"
             "\t         IP type: '%s'\n"
             "\t    Context type: '%s'\n"
             "\t   Network error: '%s'\n",
             mbim_device_get_path_display (device),
             session_id,
             VALIDATE_UNKNOWN (mbim_activation_state_get_string (activation_state)),
             VALIDATE_UNKNOWN (mbim_voice_call_state_get_string (voice_call_state)),
             VALIDATE_UNKNOWN (mbim_context_ip_type_get_string (ip_type)),
             VALIDATE_UNKNOWN (mbim_context_type_get_string (mbim_uuid_to_context_type (context_type))),
             VALIDATE_UNKNOWN (mbim_nw_error_get_string (nw_error)));

    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        g_print ("\tAccess media type: '%s'\n"
                 "\t    Access string: '%s'\n",
                 VALIDATE_UNKNOWN (mbim_access_media_type_get_string (media_type)),
                 access_string);
    }

    if (GPOINTER_TO_UINT (user_data) == CONNECT) {
        ip_configuration_query (device, NULL, session_id);
        return;
    }

    shutdown (TRUE);
}

static void
ip_packet_filters_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    g_autoptr(MbimMessage)             response = NULL;
    g_autoptr(GError)                  error = NULL;
    g_autoptr(MbimPacketFilterArray)   filters = NULL;
    g_autoptr(MbimPacketFilterV3Array) filters_v3 = NULL;
    guint32                            filters_count;
    guint32                            i;

    response = mbim_device_command_finish (device, res, &error);
    if (!response ||
        !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    /* MBIMEx 3.0 support */
    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        if (!mbim_message_ms_basic_connect_v3_ip_packet_filters_response_parse (
                response,
                NULL, /* sessionid */
                &filters_count,
                &filters_v3,
                &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        g_debug ("Successfully parsed response as MBIMEx 3.0 IP Packet Filters");
    }
    /* MBIM 1.0 support */
    else {
        if (!mbim_message_ip_packet_filters_response_parse (
                response,
                NULL, /* sessionid */
                &filters_count,
                &filters,
                &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        g_debug ("Successfully parsed response as MBIM 1.0 IP Packet Filters");
    }

    g_print ("[%s] IP packet filters: (%u)\n", mbim_device_get_path_display (device), filters_count);

    for (i = 0; i < filters_count; i++) {
        g_autofree gchar *packet_filter = NULL;
        g_autofree gchar *packet_mask   = NULL;
        guint32           filter_size = 0;

        if (filters_v3) {
            filter_size   = filters_v3[i]->filter_size;
            packet_filter = mbim_common_str_hex (filters_v3[i]->packet_filter, filter_size, ' ');
            packet_mask   = mbim_common_str_hex (filters_v3[i]->packet_mask, filter_size, ' ');
        } else if (filters) {
            filter_size   = filters[i]->filter_size;
            packet_filter = mbim_common_str_hex (filters[i]->packet_filter, filter_size, ' ');
            packet_mask   = mbim_common_str_hex (filters[i]->packet_mask, filter_size, ' ');
        } else
            g_assert_not_reached ();

        g_print ("Filter %u:\n", i);
        g_print ("\tFilter size   : %u\n", filter_size);
        g_print ("\tPacket filter : %s\n", VALIDATE_UNKNOWN (packet_filter));
        g_print ("\tPacket mask   : %s\n", VALIDATE_UNKNOWN (packet_mask));
        if (filters_v3)
            g_print ("\tFilter ID     : %u\n", filters_v3[i]->filter_id);
    }

    shutdown (TRUE);
}

static gboolean
connect_session_id_parse (const gchar  *str,
                          gboolean      allow_empty,
                          guint32      *session_id,
                          GError      **error)
{
    gchar *endptr = NULL;
    gint64 n;

    g_assert (str != NULL);
    g_assert (session_id != NULL);

    if (!str[0]) {
        if (allow_empty) {
            *session_id = 0;
            return TRUE;
        }
        g_set_error_literal (error,
                             MBIM_CORE_ERROR,
                             MBIM_CORE_ERROR_FAILED,
                             "missing session ID (must be 0 - 255)");
        return FALSE;
    }

    errno = 0;
    n = g_ascii_strtoll (str, &endptr, 10);
    if (errno || n < 0 || n > 255 || ((size_t)(endptr - str) < strlen (str))) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_FAILED,
                     "couldn't parse session ID '%s' (must be 0 - 255)",
                     str);
        return FALSE;
    }
    *session_id = (guint32) n;

    return TRUE;
}

typedef struct {
    gboolean   v3;
    guint32    session_id;
    GPtrArray *array;
    gchar     *tmp_packet_filter;
    gchar     *tmp_packet_mask;
    gchar     *tmp_filter_id;
} SetIpPacketFiltersProperties;

static void
mbim_packet_filter_v3_free (MbimPacketFilterV3 *var)
{
    if (!var)
        return;

    g_free (var->packet_filter);
    g_free (var->packet_mask);
    g_free (var);
}

static void
mbim_packet_filter_free (MbimPacketFilter *var)
{
    if (!var)
        return;

    g_free (var->packet_filter);
    g_free (var->packet_mask);
    g_free (var);
}

static void
set_ip_packet_filters_properties_clear (SetIpPacketFiltersProperties *props)
{
    g_free (props->tmp_packet_filter);
    g_free (props->tmp_packet_mask);
    g_free (props->tmp_filter_id);
    g_ptr_array_unref (props->array);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(SetIpPacketFiltersProperties, set_ip_packet_filters_properties_clear);

static gboolean
check_filter_add (SetIpPacketFiltersProperties  *props,
                  GError                       **error)
{
    g_autofree guint8 *packet_filter = NULL;
    gsize              packet_filter_size = 0;
    g_autofree guint8 *packet_mask = NULL;
    gsize              packet_mask_size = 0;

    if (!props->tmp_packet_filter) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Option 'packet-filter' is missing");
        return FALSE;
    }

    if (!props->tmp_packet_mask) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Option 'packet-mask' is missing");
        return FALSE;
    }

    if (!props->v3 && props->tmp_filter_id) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Option 'filter-id' is specific to MBIMEx v3.0");
        return FALSE;
    }

    if (props->v3 && !props->tmp_filter_id) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Option 'filter-id' is missing");
        return FALSE;
    }

    packet_filter = mbimcli_read_buffer_from_string (props->tmp_packet_filter, -1, &packet_filter_size, error);
    if (!packet_filter)
        return FALSE;

    packet_mask = mbimcli_read_buffer_from_string (props->tmp_packet_mask, -1, &packet_mask_size, error);
    if (!packet_mask)
        return FALSE;

    if (packet_filter_size != packet_mask_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Option 'packet-filter' and 'packet-mask' must have same size");
        return FALSE;
    }

    if (props->v3) {
        MbimPacketFilterV3 *filter;
        guint               filter_id;

        if (!mbimcli_read_uint_from_string (props->tmp_filter_id, &filter_id)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                         "Failed to parse 'filter-id' field as an integer");
            return FALSE;
        }

        filter = g_new0 (MbimPacketFilterV3, 1);
        filter->filter_size = packet_filter_size;
        filter->packet_filter = g_steal_pointer (&packet_filter);
        filter->packet_mask = g_steal_pointer (&packet_mask);
        filter->filter_id = filter_id;
        g_ptr_array_add (props->array, filter);
    } else {
        MbimPacketFilter *filter;

        filter = g_new0 (MbimPacketFilter, 1);
        filter->filter_size = packet_filter_size;
        filter->packet_filter = g_steal_pointer (&packet_filter);
        filter->packet_mask = g_steal_pointer (&packet_mask);
        g_ptr_array_add (props->array, filter);
    }

    g_clear_pointer (&props->tmp_filter_id, g_free);
    g_clear_pointer (&props->tmp_packet_filter, g_free);
    g_clear_pointer (&props->tmp_packet_mask, g_free);

    return TRUE;
}

static gboolean
set_ip_packet_filters_properties_handle (const gchar  *key,
                                         const gchar  *value,
                                         GError      **error,
                                         gpointer      user_data)
{
    SetIpPacketFiltersProperties *props = user_data;

    if (g_ascii_strcasecmp (key, "session-id") == 0) {
        if (!connect_session_id_parse (value, FALSE, &props->session_id, error))
            return FALSE;
    } else if (g_ascii_strcasecmp (key, "packet-filter") == 0) {
        if (props->tmp_packet_filter) {
            if (!check_filter_add (props, error))
                return FALSE;
            g_clear_pointer (&props->tmp_packet_filter, g_free);
        }
        props->tmp_packet_filter = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "packet-mask") == 0) {
        if (props->tmp_packet_mask) {
            if (!check_filter_add (props, error))
                return FALSE;
            g_clear_pointer (&props->tmp_packet_mask, g_free);
        }
        props->tmp_packet_mask = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "filter-id") == 0) {
        if (props->tmp_filter_id) {
            if (!check_filter_add (props, error))
                return FALSE;
            g_clear_pointer (&props->tmp_filter_id, g_free);
        }
        props->tmp_filter_id = g_strdup (value);
    } else {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "unrecognized option '%s'", key);
        return FALSE;
    }

    return TRUE;
}

static gboolean
set_ip_packet_filters_parse (const gchar                  *str,
                             SetIpPacketFiltersProperties *props,
                             MbimDevice                   *device)
{
    g_autoptr(GError) error = NULL;

    if (!mbimcli_parse_key_value_string (str,
                                         &error,
                                         set_ip_packet_filters_properties_handle,
                                         props)) {
        g_printerr ("error: couldn't parse input string: %s\n", error->message);
        return FALSE;
    }

    if ((props->tmp_packet_filter || props->tmp_packet_mask) &&
        !check_filter_add (props, &error)) {
        g_printerr ("error: failed to add last packet filter item: %s\n", error->message);
        return FALSE;
    }

    return TRUE;
}

typedef struct {
    guint32             session_id;
    gchar              *access_string;
    MbimAuthProtocol    auth_protocol;
    gchar              *username;
    gchar              *password;
    MbimContextIpType   ip_type;
    MbimCompression     compression;
    MbimContextType     context_type;
    MbimAccessMediaType media_type;
} ConnectActivateProperties;

static void
connect_activate_properties_clear (ConnectActivateProperties *props)
{
    g_free (props->access_string);
    g_free (props->username);
    g_free (props->password);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(ConnectActivateProperties, connect_activate_properties_clear);

static gboolean
connect_activate_properties_handle (const gchar  *key,
                                    const gchar  *value,
                                    GError      **error,
                                    gpointer      user_data)
{
    ConnectActivateProperties *props = user_data;

    /* access-string/apn may be empty */
    if ((g_ascii_strcasecmp (key, "access-string") != 0) &&
        (g_ascii_strcasecmp (key, "apn") != 0) &&
        (!value || !value[0])) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "key '%s' required a value", key);
        return FALSE;
    }

    if (g_ascii_strcasecmp (key, "session-id") == 0) {
        if (!connect_session_id_parse (value, FALSE, &props->session_id, error))
            return FALSE;
    } else if (g_ascii_strcasecmp (key, "apn") == 0) {
        g_printerr ("warning: key 'apn' is deprecated, use 'access-string' instead\n");
        g_free (props->access_string);
        props->access_string = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "access-string") == 0) {
        g_free (props->access_string);
        props->access_string = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "auth") == 0) {
        if (!mbimcli_read_auth_protocol_from_string (value, &props->auth_protocol, error)) {
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "username") == 0) {
        g_free (props->username);
        props->username = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "password") == 0) {
        g_free (props->password);
        props->password = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "ip-type") == 0) {
        if (!mbimcli_read_context_ip_type_from_string (value, &props->ip_type, error)) {
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "compression") == 0) {
        if (!mbimcli_read_compression_from_string (value, &props->compression, error)) {
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "context-type") == 0) {
        if (!mbimcli_read_context_type_from_string (value, &props->context_type, error)) {
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "media-type") == 0) {
        if (!mbimcli_read_access_media_type_from_string (value, &props->media_type, error)) {
            return FALSE;
        }
    } else {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "unrecognized option '%s'", key);
        return FALSE;
    }

    return TRUE;
}

static gboolean
set_connect_activate_parse (const gchar               *str,
                            ConnectActivateProperties *props,
                            MbimDevice                *device)
{
    g_auto(GStrv)     split = NULL;
    g_autoptr(GError) error = NULL;

    if (strchr (str, '=')) {
        /* New key=value format */
        if (!mbimcli_parse_key_value_string (str,
                                             &error,
                                             connect_activate_properties_handle,
                                             props)) {
            g_printerr ("error: couldn't parse input string: %s\n", error->message);
            return FALSE;
        }
    } else {
        /* Old non key=value format, like this:
         *    "[(APN),(PAP|CHAP|MSCHAPV2),(Username),(Password)]"
         */
        g_printerr ("warning: positional input arguments format is deprecated, use key-value format instead\n");
        split = g_strsplit (str, ",", -1);

        if (g_strv_length (split) > 4) {
            g_printerr ("error: couldn't parse input string, too many arguments\n");
            return FALSE;
	    }

        if (g_strv_length (split) > 0) {
            /* APN */
            props->access_string = g_strdup (split[0]);

            /* Use authentication method */
            if (split[1]) {
                if (!mbimcli_read_auth_protocol_from_string (split[1], &props->auth_protocol, &error)) {
                    g_printerr ("error: couldn't parse auth protocol: %s\n", error->message);
                    return FALSE;
                }
                /* Username */
                if (split[2]) {
                    props->username = g_strdup (split[2]);
                    /* Password */
                    props->password = g_strdup (split[3]);
                }
            }
        }

        if (props->auth_protocol == MBIM_AUTH_PROTOCOL_NONE) {
            if (props->username || props->password) {
                g_printerr ("error: username or password requires an auth protocol\n");
                return FALSE;
            }
        } else {
            if (!props->username) {
                g_printerr ("error: auth protocol requires a username\n");
                return FALSE;
            }
        }
    }

    return TRUE;
}

static void
home_provider_ready (MbimDevice   *device,
                     GAsyncResult *res)
{
    g_autoptr(MbimMessage)   response = NULL;
    g_autoptr(GError)        error = NULL;
    g_autoptr(MbimProvider)  provider = NULL;
    g_autofree gchar        *provider_state_str = NULL;
    g_autofree gchar        *cellular_class_str = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_home_provider_response_parse (response, &provider, &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    provider_state_str = mbim_provider_state_build_string_from_mask (provider->provider_state);
    cellular_class_str = mbim_cellular_class_build_string_from_mask (provider->cellular_class);

    g_print ("[%s] Home provider:\n"
             "\t   Provider ID: '%s'\n"
             "\t Provider name: '%s'\n"
             "\t         State: '%s'\n"
             "\tCellular class: '%s'\n"
             "\t          RSSI: '%u'\n"
             "\t    Error rate: '%u'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (provider->provider_id),
             VALIDATE_UNKNOWN (provider->provider_name),
             VALIDATE_UNKNOWN (provider_state_str),
             VALIDATE_UNKNOWN (cellular_class_str),
             provider->rssi,
             provider->error_rate);

    shutdown (TRUE);
}

static void
preferred_providers_ready (MbimDevice   *device,
                           GAsyncResult *res)
{
    g_autoptr(MbimMessage)       response = NULL;
    g_autoptr(GError)            error = NULL;
    g_autoptr(MbimProviderArray) providers = NULL;
    guint                        n_providers;
    guint                        i;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_preferred_providers_response_parse (response, &n_providers, &providers, &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!n_providers)
        g_print ("[%s] No preferred providers given\n", mbim_device_get_path_display (device));
    else
        g_print ("[%s] Preferred providers (%u):\n", mbim_device_get_path_display (device), n_providers);

    for (i = 0; i < n_providers; i++) {
        g_autofree gchar *provider_state_str = NULL;
        g_autofree gchar *cellular_class_str = NULL;

        provider_state_str = mbim_provider_state_build_string_from_mask (providers[i]->provider_state);
        cellular_class_str = mbim_cellular_class_build_string_from_mask (providers[i]->cellular_class);

        g_print ("\tProvider [%u]:\n"
                 "\t\t    Provider ID: '%s'\n"
                 "\t\t  Provider name: '%s'\n"
                 "\t\t          State: '%s'\n"
                 "\t\t Cellular class: '%s'\n"
                 "\t\t           RSSI: '%u'\n"
                 "\t\t     Error rate: '%u'\n",
                 i,
                 VALIDATE_UNKNOWN (providers[i]->provider_id),
                 VALIDATE_UNKNOWN (providers[i]->provider_name),
                 VALIDATE_UNKNOWN (provider_state_str),
                 VALIDATE_UNKNOWN (cellular_class_str),
                 providers[i]->rssi,
                 providers[i]->error_rate);
    }

    shutdown (TRUE);
}

static void
visible_providers_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    g_autoptr(MbimMessage)       response = NULL;
    g_autoptr(GError)            error = NULL;
    g_autoptr(MbimProviderArray) providers = NULL;
    guint                        n_providers;
    guint                        i;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_visible_providers_response_parse (response, &n_providers, &providers, &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!n_providers)
        g_print ("[%s] No visible providers given\n", mbim_device_get_path_display (device));
    else
        g_print ("[%s] Visible providers (%u):\n", mbim_device_get_path_display (device), n_providers);

    for (i = 0; i < n_providers; i++) {
        g_autofree gchar *provider_state_str = NULL;
        g_autofree gchar *cellular_class_str = NULL;

        provider_state_str = mbim_provider_state_build_string_from_mask (providers[i]->provider_state);
        cellular_class_str = mbim_cellular_class_build_string_from_mask (providers[i]->cellular_class);

        g_print ("\tProvider [%u]:\n"
                 "\t\t    Provider ID: '%s'\n"
                 "\t\t  Provider name: '%s'\n"
                 "\t\t          State: '%s'\n"
                 "\t\t Cellular class: '%s'\n"
                 "\t\t           RSSI: '%u'\n"
                 "\t\t     Error rate: '%u'\n",
                 i,
                 VALIDATE_UNKNOWN (providers[i]->provider_id),
                 VALIDATE_UNKNOWN (providers[i]->provider_name),
                 VALIDATE_UNKNOWN (provider_state_str),
                 VALIDATE_UNKNOWN (cellular_class_str),
                 providers[i]->rssi,
                 providers[i]->error_rate);
    }

    shutdown (TRUE);
}

static void
register_state_ready (MbimDevice   *device,
                      GAsyncResult *res,
                      gpointer      user_data)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    MbimNwError             nw_error;
    MbimRegisterState       register_state;
    MbimRegisterMode        register_mode;
    MbimDataClass           available_data_classes;
    g_autofree gchar       *available_data_classes_str = NULL;
    MbimCellularClass       cellular_class;
    g_autofree gchar       *cellular_class_str = NULL;
    g_autofree gchar       *provider_id = NULL;
    g_autofree gchar       *provider_name = NULL;
    g_autofree gchar       *roaming_text = NULL;
    MbimRegistrationFlag    registration_flag;
    g_autofree gchar       *registration_flag_str = NULL;
    MbimDataClass           preferred_data_classes;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    /* MBIMEx 2.0 support */
    if (mbim_device_check_ms_mbimex_version (device, 2, 0)) {
        if (!mbim_message_ms_basic_connect_v2_register_state_response_parse (response,
                                                                             &nw_error,
                                                                             &register_state,
                                                                             &register_mode,
                                                                             &available_data_classes,
                                                                             &cellular_class,
                                                                             &provider_id,
                                                                             &provider_name,
                                                                             &roaming_text,
                                                                             &registration_flag,
                                                                             &preferred_data_classes,
                                                                             &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        g_debug ("Successfully parsed response as MBIMEx 2.0 Register State");
    }
    /* MBIM 1.0 support */
    else {
        if (!mbim_message_register_state_response_parse (response,
                                                         &nw_error,
                                                         &register_state,
                                                         &register_mode,
                                                         &available_data_classes,
                                                         &cellular_class,
                                                         &provider_id,
                                                         &provider_name,
                                                         &roaming_text,
                                                         &registration_flag,
                                                         &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        g_debug ("Successfully parsed response as MBIM 1.0 Register State");
    }

    if (GPOINTER_TO_UINT (user_data))
        g_print ("[%s] Successfully launched automatic registration\n\n",
                 mbim_device_get_path_display (device));

    available_data_classes_str = mbim_data_class_build_string_from_mask (available_data_classes);
    cellular_class_str         = mbim_cellular_class_build_string_from_mask (cellular_class);
    registration_flag_str      = mbim_registration_flag_build_string_from_mask (registration_flag);

    g_print ("[%s] Registration status:\n"
             "\t         Network error: '%s'\n"
             "\t        Register state: '%s'\n"
             "\t         Register mode: '%s'\n"
             "\tAvailable data classes: '%s'\n"
             "\tCurrent cellular class: '%s'\n"
             "\t           Provider ID: '%s'\n"
             "\t         Provider name: '%s'\n"
             "\t          Roaming text: '%s'\n"
             "\t    Registration flags: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (mbim_nw_error_get_string (nw_error)),
             VALIDATE_UNKNOWN (mbim_register_state_get_string (register_state)),
             VALIDATE_UNKNOWN (mbim_register_mode_get_string (register_mode)),
             VALIDATE_UNKNOWN (available_data_classes_str),
             VALIDATE_UNKNOWN (cellular_class_str),
             VALIDATE_UNKNOWN (provider_id),
             VALIDATE_UNKNOWN (provider_name),
             VALIDATE_UNKNOWN (roaming_text),
             VALIDATE_UNKNOWN (registration_flag_str));

    if (mbim_device_check_ms_mbimex_version (device, 2, 0)) {
        g_autofree gchar *preferred_data_classes_str = NULL;

        preferred_data_classes_str = mbim_data_class_build_string_from_mask (preferred_data_classes);
        g_print ("\tPreferred data classes: '%s'\n",
                 VALIDATE_UNKNOWN (preferred_data_classes_str));
    }

    shutdown (TRUE);
}

static void
signal_state_ready (MbimDevice   *device,
                    GAsyncResult *res)
{
    g_autoptr(MbimMessage)          response = NULL;
    g_autoptr(GError)               error = NULL;
    guint32                         rssi;
    guint32                         error_rate;
    guint32                         signal_strength_interval;
    guint32                         rssi_threshold;
    guint32                         error_rate_threshold;
    guint32                         rsrp_snr_count;
    g_autoptr(MbimRsrpSnrInfoArray) rsrp_snr = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    /* MBIMEx 2.0 support */
    if (mbim_device_check_ms_mbimex_version (device, 2, 0)) {
        if (!mbim_message_ms_basic_connect_v2_signal_state_response_parse (response,
                                                                           &rssi,
                                                                           &error_rate,
                                                                           &signal_strength_interval,
                                                                           &rssi_threshold,
                                                                           &error_rate_threshold,
                                                                           &rsrp_snr_count,
                                                                           &rsrp_snr,
                                                                           &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
    }
    /* MBIM 1.0 support */
    else {
        if (!mbim_message_signal_state_response_parse (response,
                                                       &rssi,
                                                       &error_rate,
                                                       &signal_strength_interval,
                                                       &rssi_threshold,
                                                       &error_rate_threshold,
                                                       &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
    }

    g_print ("[%s] Signal state:\n"
             "\t          RSSI [0-31,99]: '%u'\n"
             "\t     Error rate [0-7,99]: '%u'\n"
             "\tSignal strength interval: '%u'\n"
             "\t          RSSI threshold: '%u'\n",
             mbim_device_get_path_display (device),
             rssi,
             error_rate,
             signal_strength_interval,
             rssi_threshold);

    if (error_rate_threshold == 0xFFFFFFFF)
        g_print ("\t    Error rate threshold: 'unspecified'\n");
    else
        g_print ("\t    Error rate threshold: '%u'\n", error_rate_threshold);

    if (mbim_device_check_ms_mbimex_version (device, 2, 0)) {
        g_print ("\n");
        if (rsrp_snr_count == 0) {
            g_print ("[%s] RSRP/SNR info: 'n/a'\n", mbim_device_get_path_display (device));
        } else {
            guint i;

            for (i = 0; i < rsrp_snr_count; i++) {
                g_autofree gchar *system_type_str = NULL;
                MbimRsrpSnrInfo  *info;

                info = rsrp_snr[i];

                system_type_str = mbim_data_class_build_string_from_mask (info->system_type);
                g_print ("[%s] RSRP/SNR info: '%s'\n",
                         mbim_device_get_path_display (device),
                         system_type_str);

                if (info->rsrp >= 127)
                    g_print ("\t           RSRP: 'unknown'\n");
                else
                    g_print ("\t           RSRP: '%d dBm'\n", -157 + info->rsrp);

                if (info->snr >= 128)
                    g_print ("\t            SNR: 'unknown'\n");
                else
                    g_print ("\t            SNR: '%.1lf dB'\n", -23.5 + (info->snr * 0.5));

                if (info->rsrp_threshold == 0)
                    g_print ("\t RSRP threshold: 'default'\n");
                else if (info->rsrp_threshold == 0xFFFFFFFF)
                    g_print ("\t RSRP threshold: 'unspecified'\n");
                else
                    g_print ("\t RSRP threshold: '%u'\n", info->rsrp_threshold);

                if (info->snr_threshold == 0)
                    g_print ("\t  SNR threshold: 'default'\n");
                else if (info->snr_threshold == 0xFFFFFFFF)
                    g_print ("\t  SNR threshold: 'unspecified'\n");
                else
                    g_print ("\t  SNR threshold: '%u'\n", info->snr_threshold);
                g_print ("\n");
            }
        }
    }

    shutdown (TRUE);
}

enum {
    PACKET_SERVICE_STATUS,
    PACKET_SERVICE_ATTACH,
    PACKET_SERVICE_DETACH
};

static void
packet_service_ready (MbimDevice   *device,
                      GAsyncResult *res,
                      gpointer      user_data)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    guint32                 nw_error;
    MbimPacketServiceState  packet_service_state;
    MbimDataClass           highest_available_data_class;
    MbimDataClassV3         highest_available_data_class_v3;
    g_autofree gchar       *highest_available_data_class_str = NULL;
    guint64                 uplink_speed;
    guint64                 downlink_speed;
    MbimFrequencyRange      frequency_range;
    MbimDataSubclass        data_subclass;
    g_autoptr(MbimTai)      tai = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    /* MBIMEx 3.0 support */
    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        if (!mbim_message_ms_basic_connect_v3_packet_service_response_parse (response,
                                                                             &nw_error,
                                                                             &packet_service_state,
                                                                             &highest_available_data_class_v3,
                                                                             &uplink_speed,
                                                                             &downlink_speed,
                                                                             &frequency_range,
                                                                             &data_subclass,
                                                                             &tai,
                                                                             &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        g_debug ("Successfully parsed response as MBIM 3.0 Packet Service");
    }
    /* MBIMEx 2.0 support */
    else if (mbim_device_check_ms_mbimex_version (device, 2, 0)) {
        if (!mbim_message_ms_basic_connect_v2_packet_service_response_parse (response,
                                                                             &nw_error,
                                                                             &packet_service_state,
                                                                             &highest_available_data_class,
                                                                             &uplink_speed,
                                                                             &downlink_speed,
                                                                             &frequency_range,
                                                                             &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        g_debug ("Successfully parsed response as MBIM 2.0 Packet Service");
    }
    /* MBIM 1.0 support */
    else {
        if (!mbim_message_packet_service_response_parse (response,
                                                         &nw_error,
                                                         &packet_service_state,
                                                         &highest_available_data_class,
                                                         &uplink_speed,
                                                         &downlink_speed,
                                                         &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        g_debug ("Successfully parsed response as MBIM 1.0 Packet Service");
    }

    switch (GPOINTER_TO_UINT (user_data)) {
    case PACKET_SERVICE_ATTACH:
        g_print ("[%s] Successfully attached to packet service\n\n",
                 mbim_device_get_path_display (device));
        break;
    case PACKET_SERVICE_DETACH:
        g_print ("[%s] Successfully detached from packet service\n\n",
                 mbim_device_get_path_display (device));
        break;
    default:
        break;
    }

    if (mbim_device_check_ms_mbimex_version (device, 3, 0))
        highest_available_data_class_str = mbim_data_class_v3_build_string_from_mask (highest_available_data_class_v3);
    else
        highest_available_data_class_str = mbim_data_class_build_string_from_mask (highest_available_data_class);

    g_print ("[%s] Packet service status:\n"
             "\t         Network error: '%s'\n"
             "\t  Packet service state: '%s'\n"
             "\tAvailable data classes: '%s'\n"
             "\t          Uplink speed: '%" G_GUINT64_FORMAT " bps'\n"
             "\t        Downlink speed: '%" G_GUINT64_FORMAT " bps'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (mbim_nw_error_get_string (nw_error)),
             VALIDATE_UNKNOWN (mbim_packet_service_state_get_string (packet_service_state)),
             VALIDATE_UNKNOWN (highest_available_data_class_str),
             uplink_speed,
             downlink_speed);

    if (mbim_device_check_ms_mbimex_version (device, 2, 0)) {
        g_autofree gchar *frequency_range_str = NULL;

        frequency_range_str = mbim_frequency_range_build_string_from_mask (frequency_range);
        g_print ("\t       Frequency range: '%s'\n",
                 VALIDATE_UNKNOWN (frequency_range_str));
    }

    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        g_autofree gchar *data_subclass_str  = NULL;
        g_autofree gchar *tai_plmn_mcc_str = NULL;
        g_autofree gchar *tai_plmn_mnc_str = NULL;

        /* Mobile Country Code of 3 decimal digits; The
         * least significant 12 bits contains BCD-encoded
         * 3 decimal digits sequentially for the MCC, with
         * the last digit of the MCC in the least significant
         * 4 bits. The unused bits in the UINT16 integer
         * must be zeros. */
        tai_plmn_mcc_str = g_strdup_printf ("%03x", tai->plmn_mcc & 0x0FFF);

        /* Mobile Network Code of either 3 or 2 decimal
         * digits; The most significant bit indicates
         * whether the MNC has 2 decimal digits or 3
         * decimal digits. If this bit has 1, the MNC has 2
         * decimal digits and the least significant 8 bits
         * contains them in BCD-encoded form
         * sequentially, with the last digit of the MNC in
         * the least significant 4 bits. If the most
         * significant bit has 0, the MNC has 3 decimal
         * digits and the least significant 12 bits contains
         * them in BCD-encoded form sequentially, with
         * the last digit of the MNC in the least
         * esignificant 4 bits. The unused bits in the
         * UINT16 integer must be zeros. */
        if (tai->plmn_mnc & 0x8000)
            tai_plmn_mnc_str = g_strdup_printf ("%02x", tai->plmn_mnc & 0x00FF);
        else
            tai_plmn_mnc_str = g_strdup_printf ("%03x", tai->plmn_mnc & 0x0FFF);

        data_subclass_str = mbim_data_subclass_build_string_from_mask (data_subclass);
        g_print ("\t        Data sub class: '%s'\n"
                 "\t          TAI PLMN MCC: '%s'\n"
                 "\t          TAI PLMN MNC: '%s'\n"
                 "\t              TAI  TAC: '%u'\n",
                 VALIDATE_UNKNOWN (data_subclass_str),
                 tai_plmn_mcc_str,
                 tai_plmn_mnc_str,
                 tai->tac);
    }

    shutdown (TRUE);
}

static void
packet_statistics_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    g_autoptr(MbimMessage) response = NULL;
    g_autoptr(GError)      error = NULL;
    guint32                in_discards;
    guint32                in_errors;
    guint64                in_octets;
    guint64                in_packets;
    guint64                out_octets;
    guint64                out_packets;
    guint32                out_errors;
    guint32                out_discards;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_packet_statistics_response_parse (response,
                                                        &in_discards,
                                                        &in_errors,
                                                        &in_octets,
                                                        &in_packets,
                                                        &out_octets,
                                                        &out_packets,
                                                        &out_errors,
                                                        &out_discards,
                                                        &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Packet statistics:\n"
             "\t   Octets (in): '%" G_GUINT64_FORMAT "'\n"
             "\t  Packets (in): '%" G_GUINT64_FORMAT "'\n"
             "\t Discards (in): '%u'\n"
             "\t   Errors (in): '%u'\n"
             "\t  Octets (out): '%" G_GUINT64_FORMAT "'\n"
             "\t Packets (out): '%" G_GUINT64_FORMAT "'\n"
             "\tDiscards (out): '%u'\n"
             "\t  Errors (out): '%u'\n",
             mbim_device_get_path_display (device),
             in_octets,
             in_packets,
             in_discards,
             in_errors,
             out_octets,
             out_packets,
             out_discards,
             out_errors);

    shutdown (TRUE);
}

static void
provisioned_contexts_ready (MbimDevice   *device,
                            GAsyncResult *res)
{
    g_autoptr(MbimMessage)                        response = NULL;
    g_autoptr(MbimProvisionedContextElementArray) provisioned_contexts = NULL;
    g_autoptr(GError)                             error = NULL;
    guint32 provisioned_contexts_count;
    guint   i;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_provisioned_contexts_response_parse (
            response,
            &provisioned_contexts_count,
            &provisioned_contexts,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Provisioned contexts (%u):\n",
             mbim_device_get_path_display (device),
             provisioned_contexts_count);

    for (i = 0; i < provisioned_contexts_count; i++) {
        g_print ("\tContext ID %u:\n"
                 "\t   Context type: '%s'\n"
                 "\t  Access string: '%s'\n"
                 "\t       Username: '%s'\n"
                 "\t       Password: '%s'\n"
                 "\t    Compression: '%s'\n"
                 "\t  Auth protocol: '%s'\n",
                 provisioned_contexts[i]->context_id,
                 VALIDATE_UNKNOWN (mbim_context_type_get_string (
                     mbim_uuid_to_context_type (&provisioned_contexts[i]->context_type))),
                 VALIDATE_EMPTY (provisioned_contexts[i]->access_string),
                 VALIDATE_EMPTY (provisioned_contexts[i]->user_name),
                 VALIDATE_EMPTY (provisioned_contexts[i]->password),
                 VALIDATE_UNKNOWN (mbim_compression_get_string (
                     provisioned_contexts[i]->compression)),
                 VALIDATE_UNKNOWN (mbim_auth_protocol_get_string (
                     provisioned_contexts[i]->auth_protocol)));
    }

    shutdown (TRUE);
}

typedef struct {
    guint             context_id;
    MbimCompression   compression;
    MbimAuthProtocol  auth_protocol;
    MbimContextType   context_type;
    gchar            *access_string;
    gchar            *username;
    gchar            *password;
    gchar            *provider_id;
} ProvisionedContextProperties;

static void
provisioned_context_properties_clear (ProvisionedContextProperties *props)
{
    g_free (props->access_string);
    g_free (props->username);
    g_free (props->password);
    g_free (props->provider_id);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(ProvisionedContextProperties, provisioned_context_properties_clear)

static gboolean
set_provisioned_contexts_foreach_cb (const gchar                   *key,
                                     const gchar                   *value,
                                     GError                       **error,
                                     ProvisionedContextProperties  *props)
{
    if (g_ascii_strcasecmp (key, "context-id") == 0) {
        if (!mbimcli_read_uint_from_string (value, &props->context_id)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "Couldn't parse context-id as integer : '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "context-type") == 0) {
        if (!mbimcli_read_context_type_from_string (value, &props->context_type, error)) {
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "auth") == 0) {
        if (!mbimcli_read_auth_protocol_from_string (value, &props->auth_protocol, error)) {
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "compression") == 0) {
        if (!mbimcli_read_compression_from_string (value, &props->compression, error)) {
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "username") == 0) {
        g_free (props->username);
        props->username = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "password") == 0) {
        g_free (props->password);
        props->password = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "access-string") == 0) {
        g_free (props->access_string);
        props->access_string = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "provider-id") == 0) {
        g_free (props->provider_id);
        props->provider_id = g_strdup (value);
    } else {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "unrecognized option '%s'", key);
        return FALSE;
    }

    return TRUE;
}

typedef struct {
    guint32  signal_strength;
    guint32  rssi;
    guint32  error_rate;
} SignalStateProperties;

static gboolean
set_signal_state_foreach_cb (const gchar            *key,
                             const gchar            *value,
                             GError                **error,
                             SignalStateProperties  *props)
{
    if (g_ascii_strcasecmp (key, "signal-strength-interval") == 0) {
        if (!mbimcli_read_uint_from_string (value, &props->signal_strength)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "Couldn't parse signal-strength as integer : '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "rssi-threshold") == 0) {
        if (!mbimcli_read_uint_from_string (value, &props->rssi)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "Couldn't parse rssi as integer : '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "error-rate-threshold") == 0) {
        if (!mbimcli_read_uint_from_string (value, &props->error_rate)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "Couldn't parse error-rate as integer : '%s'", value);
            return FALSE;
        }
    } else {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "unrecognized option '%s'", key);
        return FALSE;
    }

    return TRUE;
}

static void
network_idle_hint_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    g_autoptr(MbimMessage)    response = NULL;
    g_autoptr(GError)         error = NULL;
    MbimNetworkIdleHintState  network_state;
    const gchar              *network_state_str = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_network_idle_hint_response_parse (
            response,
            &network_state,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    network_state_str = mbim_network_idle_hint_state_get_string (network_state);
    g_print ("[%s] Network idle hint state: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (network_state_str));

    shutdown (TRUE);
}

static void
emergency_mode_ready (MbimDevice   *device,
                      GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    MbimEmergencyModeState  emergency_state;
    const gchar            *emergency_state_str = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_emergency_mode_response_parse (
            response,
            &emergency_state,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    emergency_state_str = mbim_emergency_mode_state_get_string (emergency_state);
    g_print ("[%s] Emergency mode: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (emergency_state_str));

    shutdown (TRUE);
}

static void
set_service_activation_ready (MbimDevice   *device,
                              GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    MbimNwError             nw_error;
    guint32                 result_data_size;
    const guint8           *result_data = NULL;
    g_autofree gchar       *result_data_str = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_service_activation_response_parse (response,
                                                         &nw_error,
                                                         &result_data_size,
                                                         &result_data,
                                                         &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    result_data_str = mbim_common_str_hex (result_data, result_data_size, ':');
    g_print ("[%s] Service activation response received successfully:\n"
             "\t         Network error: '%s'\n"
             "\t                  Data: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (mbim_nw_error_get_string (nw_error)),
             result_data_str);

    shutdown (TRUE);
}

void
mbimcli_basic_connect_run (MbimDevice   *device,
                           GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;
    g_autoptr(GError)      error = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to get capabilities? */
    if (query_device_caps_flag) {
        g_debug ("Asynchronously querying device capabilities...");
        request = (mbim_message_device_caps_query_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_device_caps_ready,
                             NULL);
        return;
    }

    /* Request to get subscriber ready status? */
    if (query_subscriber_ready_status_flag) {
        g_debug ("Asynchronously querying subscriber ready status...");
        request = (mbim_message_subscriber_ready_status_query_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_subscriber_ready_status_ready,
                             NULL);
        return;
    }

    /* Request to get radio state? */
    if (query_radio_state_flag) {
        g_debug ("Asynchronously querying radio state...");
        request = (mbim_message_radio_state_query_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_radio_state_ready,
                             NULL);
        return;
    }

    /* Request to set radio state? */
    if (set_radio_state_str) {
        MbimRadioSwitchState radio_state;

        if (g_ascii_strcasecmp (set_radio_state_str, "on") == 0) {
            radio_state = MBIM_RADIO_SWITCH_STATE_ON;
        } else if (g_ascii_strcasecmp (set_radio_state_str, "off") == 0) {
            radio_state = MBIM_RADIO_SWITCH_STATE_OFF;
        } else {
            g_printerr ("error: invalid radio state: '%s'\n", set_radio_state_str);
            shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously setting radio state to %s...",
                 radio_state == MBIM_RADIO_SWITCH_STATE_ON ? "on" : "off");
        request = mbim_message_radio_state_set_new (radio_state, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_radio_state_ready,
                             NULL);
        return;
    }

    /* Request to query device services? */
    if (query_device_services_flag) {
        g_debug ("Asynchronously querying device services...");
        request = (mbim_message_device_services_query_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_device_services_ready,
                             NULL);
        return;
    }

    /* Query PIN state? */
    if (query_pin_flag) {
        g_debug ("Asynchronously querying PIN state...");
        request = (mbim_message_pin_query_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)pin_ready,
                             GUINT_TO_POINTER (FALSE));
        return;
    }

    /* Set PIN? */
    if (set_pin_enter_str ||
        set_pin_change_str ||
        set_pin_enable_str ||
        set_pin_disable_str ||
        set_pin_enter_puk_str) {
        MbimPinType       pin_type;
        MbimPinOperation  pin_operation;
        g_autofree gchar *pin = NULL;
        g_autofree gchar *new_pin = NULL;

        if (set_pin_enter_puk_str) {
            g_debug ("Asynchronously entering PUK...");
            pin_type = MBIM_PIN_TYPE_PUK1;
            pin_operation = MBIM_PIN_OPERATION_ENTER;
            set_pin_input_parse (set_pin_enter_puk_str, &pin, &new_pin, &pin_type);
        } else {
            pin_type = MBIM_PIN_TYPE_PIN1;
            if (set_pin_change_str) {
                g_debug ("Asynchronously changing PIN...");
                pin_operation = MBIM_PIN_OPERATION_CHANGE;
                set_pin_input_parse (set_pin_change_str, &pin, &new_pin, NULL);
            } else if (set_pin_enable_str) {
                g_debug ("Asynchronously enabling PIN...");
                pin_operation = MBIM_PIN_OPERATION_ENABLE;
                set_pin_input_parse (set_pin_enable_str, &pin, NULL, NULL);
            } else if (set_pin_disable_str) {
                g_debug ("Asynchronously disabling PIN...");
                pin_operation = MBIM_PIN_OPERATION_DISABLE;
                set_pin_input_parse (set_pin_disable_str, &pin, NULL, &pin_type);
            } else if (set_pin_enter_str) {
                g_debug ("Asynchronously entering PIN...");
                pin_operation = MBIM_PIN_OPERATION_ENTER;
                set_pin_input_parse (set_pin_enter_str, &pin, NULL, &pin_type);
            } else
                g_assert_not_reached ();
        }

        if (!pin || pin_type == MBIM_PIN_TYPE_UNKNOWN) {
            shutdown (FALSE);
            return;
        }

        request = (mbim_message_pin_set_new (pin_type,
                                             pin_operation,
                                             pin,
                                             new_pin,
                                             &error));
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)pin_ready,
                             GUINT_TO_POINTER (TRUE));
        return;
    }

    /* Query PIN list? */
    if (query_pin_list_flag) {
        g_debug ("Asynchronously querying PIN list...");
        request = (mbim_message_pin_list_query_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)pin_list_ready,
                             GUINT_TO_POINTER (FALSE));
        return;
    }

    /* Query home provider? */
    if (query_home_provider_flag) {
        request = mbim_message_home_provider_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)home_provider_ready,
                             NULL);
        return;
    }

    /* Query preferred providers? */
    if (query_preferred_providers_flag) {
        request = mbim_message_preferred_providers_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)preferred_providers_ready,
                             NULL);
        return;
    }

    /* Query visible providers? */
    if (query_visible_providers_flag) {
        request = mbim_message_visible_providers_query_new (MBIM_VISIBLE_PROVIDERS_ACTION_FULL_SCAN, NULL);
        mbim_device_command (ctx->device,
                             request,
                             120, /* longer timeout, needs to scan */
                             ctx->cancellable,
                             (GAsyncReadyCallback)visible_providers_ready,
                             NULL);
        return;
    }

    /* Query registration status? */
    if (query_register_state_flag) {
        request = mbim_message_register_state_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)register_state_ready,
                             GUINT_TO_POINTER (FALSE));
        return;
    }

    /* Launch automatic registration? */
    if (set_register_state_automatic_flag) {
        request = mbim_message_register_state_set_new (NULL, MBIM_REGISTER_ACTION_AUTOMATIC, 0, &error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             120, /* longer timeout, needs to look for the home network */
                             ctx->cancellable,
                             (GAsyncReadyCallback)register_state_ready,
                             GUINT_TO_POINTER (TRUE));
        return;
    }

    /* Query signal status? */
    if (query_signal_state_flag) {
        request = mbim_message_signal_state_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)signal_state_ready,
                             NULL);
        return;
    }

    /* Query packet service status? */
    if (query_packet_service_flag) {
        request = mbim_message_packet_service_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)packet_service_ready,
                             GUINT_TO_POINTER (PACKET_SERVICE_STATUS));
        return;
    }

    /* Launch packet attach or detach? */
    if (set_packet_service_attach_flag ||
        set_packet_service_detach_flag) {
        MbimPacketServiceAction action;

        if (set_packet_service_attach_flag)
            action = MBIM_PACKET_SERVICE_ACTION_ATTACH;
        else if (set_packet_service_detach_flag)
            action = MBIM_PACKET_SERVICE_ACTION_DETACH;
        else
            g_assert_not_reached ();

        request = mbim_message_packet_service_set_new (action, &error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             120,
                             ctx->cancellable,
                             (GAsyncReadyCallback)packet_service_ready,
                             GUINT_TO_POINTER (set_packet_service_attach_flag ?
                                               PACKET_SERVICE_ATTACH :
                                               PACKET_SERVICE_DETACH));
        return;
    }

    /* Query connection status? */
    if (query_connect_str) {
        guint32 session_id = 0;

        if (!connect_session_id_parse (query_connect_str, TRUE, &session_id, &error)) {
            g_printerr ("error: couldn't parse session ID: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
            request = mbim_message_ms_basic_connect_v3_connect_query_new (session_id,
                                                                      &error);
        } else {
            request = mbim_message_connect_query_new (session_id,
                                                      MBIM_ACTIVATION_STATE_UNKNOWN,
                                                      MBIM_VOICE_CALL_STATE_NONE,
                                                      MBIM_CONTEXT_IP_TYPE_DEFAULT,
                                                      mbim_uuid_from_context_type (MBIM_CONTEXT_TYPE_INTERNET),
                                                      0,
                                                      &error);
        }
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)connect_ready,
                             GUINT_TO_POINTER (CONNECTION_STATUS));
        return;
    }

    /* Connect? */
    if (set_connect_activate_str) {
        g_auto(ConnectActivateProperties) props = {
            .session_id    = 0,
            .access_string = NULL,
            .auth_protocol = MBIM_AUTH_PROTOCOL_NONE,
            .username      = NULL,
            .password      = NULL,
            .ip_type       = MBIM_CONTEXT_IP_TYPE_DEFAULT,
            .compression   = MBIM_COMPRESSION_NONE,
            .context_type  = MBIM_CONTEXT_TYPE_INTERNET,
            .media_type    = MBIM_ACCESS_MEDIA_TYPE_UNKNOWN,
        };

        if (!set_connect_activate_parse (set_connect_activate_str, &props, device)) {
            shutdown (FALSE);
            return;
        }

        if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
            request = mbim_message_ms_basic_connect_v3_connect_set_new (props.session_id,
                                                                        MBIM_ACTIVATION_COMMAND_ACTIVATE,
                                                                        props.compression,
                                                                        props.auth_protocol,
                                                                        props.ip_type,
                                                                        mbim_uuid_from_context_type (props.context_type),
                                                                        props.media_type,
                                                                        props.access_string,
                                                                        props.username,
                                                                        props.password,
                                                                        NULL, /* unnamed IEs */
                                                                        &error);
        } else {
            request = mbim_message_connect_set_new (props.session_id,
                                                    MBIM_ACTIVATION_COMMAND_ACTIVATE,
                                                    props.access_string,
                                                    props.username,
                                                    props.password,
                                                    props.compression,
                                                    props.auth_protocol,
                                                    props.ip_type,
                                                    mbim_uuid_from_context_type (props.context_type),
                                                    &error);
        }

        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             120,
                             ctx->cancellable,
                             (GAsyncReadyCallback)connect_ready,
                             GUINT_TO_POINTER (CONNECT));
        return;
    }

    /* Query IP configuration? */
    if (query_ip_configuration_str) {
        guint32 session_id = 0;

        if (!connect_session_id_parse (query_ip_configuration_str, TRUE, &session_id, &error)) {
            g_printerr ("error: couldn't parse session ID: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        ip_configuration_query (ctx->device, ctx->cancellable, session_id);
        return;
    }

    /* Disconnect? */
    if (set_connect_deactivate_str) {
        guint32 session_id = 0;

        if (!connect_session_id_parse (set_connect_deactivate_str, TRUE, &session_id, &error)) {
            g_printerr ("error: couldn't parse session ID: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
            request = mbim_message_ms_basic_connect_v3_connect_set_new (session_id,
                                                                        MBIM_ACTIVATION_COMMAND_DEACTIVATE,
                                                                        MBIM_COMPRESSION_NONE,
                                                                        MBIM_AUTH_PROTOCOL_NONE,
                                                                        MBIM_CONTEXT_IP_TYPE_DEFAULT,
                                                                        mbim_uuid_from_context_type (MBIM_CONTEXT_TYPE_INTERNET),
                                                                        MBIM_ACCESS_MEDIA_TYPE_UNKNOWN,
                                                                        NULL,
                                                                        NULL,
                                                                        NULL,
                                                                        NULL,
                                                                        &error);
        } else {
            request = mbim_message_connect_set_new (session_id,
                                                    MBIM_ACTIVATION_COMMAND_DEACTIVATE,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    MBIM_COMPRESSION_NONE,
                                                    MBIM_AUTH_PROTOCOL_NONE,
                                                    MBIM_CONTEXT_IP_TYPE_DEFAULT,
                                                    mbim_uuid_from_context_type (MBIM_CONTEXT_TYPE_INTERNET),
                                                    &error);
        }

        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             60,
                             ctx->cancellable,
                             (GAsyncReadyCallback)connect_ready,
                             GUINT_TO_POINTER (DISCONNECT));
        return;
    }

    /* Packet statistics? */
    if (query_packet_statistics_flag) {
        request = mbim_message_packet_statistics_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)packet_statistics_ready,
                             NULL);
        return;
    }

    /* Query IP packet filters? */
    if (query_ip_packet_filters_str) {
        guint32 session_id = 0;

        if (!connect_session_id_parse (query_ip_packet_filters_str, TRUE, &session_id, &error)) {
            g_printerr ("error: couldn't parse session ID: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
            g_debug ("Asychronously querying v3.0 IP packet filter......");
            request = mbim_message_ms_basic_connect_v3_ip_packet_filters_query_new (session_id, 0, NULL, &error);
        } else {
            g_debug ("Asychronously querying v1.0 IP packet filter......");
            request = mbim_message_ip_packet_filters_query_new (session_id, 0, NULL, &error);
        }

        if (!request) {
            g_printerr ("error: couldn't create IP packet filters request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)ip_packet_filters_ready,
                             NULL);
        return;
    }

    /* Set IP packet filters? */
    if (set_ip_packet_filters_str) {
        g_auto(SetIpPacketFiltersProperties) props = {
            .v3 = FALSE,
            .session_id = 0,
            .array = NULL,
        };

        props.v3 = mbim_device_check_ms_mbimex_version (device, 3, 0);
        if (props.v3)
            props.array = g_ptr_array_new_with_free_func ((GDestroyNotify)mbim_packet_filter_v3_free);
        else
            props.array = g_ptr_array_new_with_free_func ((GDestroyNotify)mbim_packet_filter_free);

        if (!set_ip_packet_filters_parse (set_ip_packet_filters_str, &props, device)) {
            shutdown (FALSE);
            return;
        }

        if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
            g_debug ("Asychronously set v3.0 IP packet filter......");
            request = mbim_message_ms_basic_connect_v3_ip_packet_filters_set_new (props.session_id,
                                                                                  props.array->len,
                                                                                  (const MbimPacketFilterV3 *const *)props.array->pdata,
                                                                                  &error);
        } else {
            g_debug ("Asychronously set v1.0 IP packet filter......");
            request = mbim_message_ip_packet_filters_set_new (props.session_id,
                                                              props.array->len,
                                                              (const MbimPacketFilter *const *)props.array->pdata,
                                                              &error);
        }

        if (!request) {
            g_printerr ("error: couldn't create IP packet filters request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)ip_packet_filters_ready,
                             NULL);
        return;
    }

    /* Provisioned contexts? */
    if (query_provisioned_contexts_flag) {
        request = mbim_message_provisioned_contexts_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)provisioned_contexts_ready,
                             NULL);
        return;
    }

    /* Set provisioned context */
    if (set_provisioned_contexts_str) {
        g_auto(ProvisionedContextProperties) props = {
            .access_string   = NULL,
            .context_id      = 0,
            .auth_protocol   = MBIM_AUTH_PROTOCOL_NONE,
            .username        = NULL,
            .password        = NULL,
            .compression     = MBIM_COMPRESSION_NONE,
            .context_type    = MBIM_CONTEXT_TYPE_INVALID,
            .provider_id     = NULL
        };

        if (!mbimcli_parse_key_value_string (set_provisioned_contexts_str,
                                             &error,
                                             (MbimParseKeyValueForeachFn)set_provisioned_contexts_foreach_cb,
                                             &props)) {
            g_printerr ("error: couldn't parse input string: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_provisioned_contexts_set_new ((guint32)props.context_id,
                                                             mbim_uuid_from_context_type (props.context_type),
                                                             props.access_string,
                                                             props.username,
                                                             props.password,
                                                             props.compression,
                                                             props.auth_protocol,
                                                             props.provider_id,
                                                             &error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             60,
                             ctx->cancellable,
                             (GAsyncReadyCallback)provisioned_contexts_ready,
                             NULL);
        return;
    }

    /* Set signal state */
    if (set_signal_state_str) {
        SignalStateProperties props = {
            .signal_strength = 0,
            .rssi            = 0,
            .error_rate      = 0
        };

        if (!mbimcli_parse_key_value_string (set_signal_state_str,
                                             &error,
                                             (MbimParseKeyValueForeachFn)set_signal_state_foreach_cb,
                                             &props)) {
            g_printerr ("error: couldn't parse input string: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_signal_state_set_new (props.signal_strength,
                                                     props.rssi,
                                                     props.error_rate,
                                                     &error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             60,
                             ctx->cancellable,
                             (GAsyncReadyCallback)signal_state_ready,
                             NULL);
        return;
    }

    /* Set network idle hint state */
    if (set_network_idle_hint_str) {
        MbimNetworkIdleHintState  network_state;

        if (!mbimcli_read_network_idle_hint_state_from_string (set_network_idle_hint_str, &network_state, &error)) {
            g_printerr ("error: couldn't read idle hint state: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_network_idle_hint_set_new (network_state, &error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)network_idle_hint_ready,
                             NULL);
        return;
    }

    /* Get network idle hint state */
    if (query_network_idle_hint_flag) {
        request = mbim_message_network_idle_hint_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)network_idle_hint_ready,
                             NULL);
        return;
    }

    /* Set emergency mode state */
    if (set_emergency_mode_str) {
        MbimEmergencyModeState  emergency_state;

        if (!mbimcli_read_emergency_mode_state_from_string (set_emergency_mode_str, &emergency_state, &error)) {
            g_printerr ("error: couldn't read emergency mode state: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_emergency_mode_set_new (emergency_state, &error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)emergency_mode_ready,
                             NULL);
        return;
    }

    /* Query emergency mode state */
    if (query_emergency_mode_flag) {
        request = mbim_message_emergency_mode_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)emergency_mode_ready,
                             NULL);
        return;
    }

    /* Request to set service activation */
    if (set_service_activation_str) {
        guint8 *data = NULL;
        gsize   data_size = 0;

        data = mbimcli_read_buffer_from_string (set_service_activation_str, -1, &data_size, &error);
        if (!data) {
            g_printerr ("error: couldn't parse the input: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_service_activation_set_new (data_size,
                                                           data,
                                                           &error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)set_service_activation_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
