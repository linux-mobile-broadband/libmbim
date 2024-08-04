/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2020 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libmbim-glib.h>

#include "mbimcli.h"
#include "mbimcli-helpers.h"

/* Context */
typedef struct {
    MbimDevice *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gchar   *set_sar_config_str;
static gboolean query_sar_config_flag;
static gchar   *set_transmission_status_str;
static gboolean query_transmission_status_flag;

static GOptionEntry entries[] = {
    { "ms-set-sar-config", 0, 0, G_OPTION_ARG_STRING, &set_sar_config_str,
      "Set SAR config",
      "[(device|os),(enabled|disabled)[,[{antenna_index,backoff_index}...]]]"
    },
    { "ms-query-sar-config", 0, 0, G_OPTION_ARG_NONE, &query_sar_config_flag,
      "Query SAR config",
      NULL
    },
    { "ms-set-transmission-status", 0, 0, G_OPTION_ARG_STRING, &set_transmission_status_str,
      "Set transmission status and hysteresis timer (in seconds)",
      "[(enabled|disabled),(timer)]"
    },
    { "ms-query-transmission-status", 0, 0, G_OPTION_ARG_NONE, &query_transmission_status_flag,
      "Query transmission status",
      NULL
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

GOptionGroup *
mbimcli_ms_sar_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("ms-sar",
                               "Microsoft SAR options:",
                               "Show Microsoft SAR Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_ms_sar_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = !!set_sar_config_str +
                query_sar_config_flag +
                !!set_transmission_status_str +
                query_transmission_status_flag;

    if (n_actions > 1) {
        g_printerr ("error: too many Microsoft SAR actions requested\n");
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
ms_sar_ready (MbimDevice   *device,
              GAsyncResult *res)
{
    g_autoptr(MbimMessage)              response = NULL;
    g_autoptr(GError)                   error = NULL;
    MbimSarControlMode                  mode;
    const gchar                        *mode_str;
    MbimSarBackoffState                 backoff_state;
    const gchar                        *backoff_state_str;
    MbimSarWifiHardwareState            wifi_integration;
    const gchar                        *wifi_integration_str;
    guint32                             config_states_count;
    g_autoptr(MbimSarConfigStateArray)  config_states = NULL;
    guint32                             i;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_sar_config_response_parse (
            response,
            &mode,
            &backoff_state,
            &wifi_integration,
            &config_states_count,
            &config_states,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    mode_str             = mbim_sar_control_mode_get_string (mode);
    backoff_state_str    = mbim_sar_backoff_state_get_string (backoff_state);
    wifi_integration_str = mbim_sar_wifi_hardware_state_get_string (wifi_integration);

    g_print ("[%s] SAR config:\n"
             "\t                Mode: %s\n"
             "\t       Backoff state: %s\n"
             "\tWi-Fi hardware state: %s\n"
             "\t       Config states: %u\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (mode_str),
             VALIDATE_UNKNOWN (backoff_state_str),
             VALIDATE_UNKNOWN (wifi_integration_str),
             config_states_count);


    for (i = 0; i < config_states_count; i++) {
        g_print ("\t\t[%u]\n", i);
        if (config_states[i]->antenna_index == 0xFFFFFFFF)
            g_print ("\t\t  Antenna index: all\n");
        else
            g_print ("\t\t  Antenna index: %u\n", config_states[i]->antenna_index);
        g_print ("\t\t  Backoff index: %u\n", config_states[i]->backoff_index);
    }

    shutdown (TRUE);
}

static void
modem_transmission_status_ready (MbimDevice   *device,
                                 GAsyncResult *res)
{
    g_autoptr(MbimMessage)              response = NULL;
    g_autoptr(GError)                   error = NULL;
    MbimTransmissionNotificationStatus  channel_notification;
    const gchar                        *channel_notification_str;
    MbimTransmissionState               transmission_status;
    const gchar                        *transmission_status_str;
    guint32                             hysteresis_timer;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_sar_transmission_status_response_parse (
            response,
            &channel_notification,
            &transmission_status,
            &hysteresis_timer,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    channel_notification_str = mbim_transmission_notification_status_get_string (channel_notification);
    transmission_status_str  = mbim_transmission_state_get_string (transmission_status);

    g_print ("[%s] Transmission status:\n"
             "\t        notification: %s\n"
             "\t              status: %s\n"
             "\t    hysteresis timer: (%u)\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (channel_notification_str),
             VALIDATE_UNKNOWN (transmission_status_str),
             hysteresis_timer);

    shutdown (TRUE);
}

static gboolean
sar_config_input_parse (const gchar         *str,
                        MbimSarControlMode  *mode,
                        MbimSarBackoffState *state,
                        GPtrArray          **states_array,
                        GError             **error)
{
    g_auto(GStrv) split = NULL;

    g_assert (mode != NULL);
    g_assert (state != NULL);
    g_assert (states_array!= NULL);

    /* Format of the string is:
     *    "(mode:device or os),(state: enabled or disabled)[,[{antenna_index,backoff_index}...]]"
     *    i.e. array of {antenna_index,backoff_index} is optional
     */
    split = g_strsplit (str, ",", 3);

    if (g_strv_length (split) < 2) {
        g_set_error_literal (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                             "couldn't parse input string, missing arguments");
        return FALSE;
    }

    if (!mbimcli_read_sar_control_mode_from_string (split[0], mode, error)) {
        return FALSE;
    }

    if (!mbimcli_read_sar_backoff_state_from_string (split[1], state, error)) {
        return FALSE;
    }

    /* Check whether we have the optional item array: [{antenna_index,backoff_index}...] */
    if (split[2]) {
        const gchar      *array_begin;
        const gchar      *array_end;
        g_autofree gchar *array_str = NULL;

        array_begin = strchr (split[2], '[');
        array_end   = strchr (split[2], ']');

        if (!array_begin || !array_end || (array_begin > array_end)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "invalid SAR config state array: '%s'", split[2]);
            return FALSE;
        }

        array_str = g_strndup (&array_begin[1], array_end - array_begin - 1);
        if (!mbimcli_parse_sar_config_state_array (array_str, states_array, error)) {
            return FALSE;
        }
    } else
        *states_array = NULL;

    return TRUE;
}

static gboolean
transmission_status_input_parse (const gchar                        *str,
                                 MbimTransmissionNotificationStatus *notification,
                                 guint                              *hysteresis_timer)
{
    g_auto(GStrv) split = NULL;

    g_assert (notification != NULL);
    g_assert (hysteresis_timer != NULL);

    /* Format of the string is:
     *    "(notification: enabled or disabled),(seconds: 1~5)"
     */
    split = g_strsplit (str, ",", -1);

    if (g_strv_length (split) < 2) {
        g_printerr ("error: couldn't parse input string, missing arguments\n");
        return FALSE;
    }

    if (g_ascii_strcasecmp (split[0], "disabled") == 0) {
        *notification = MBIM_TRANSMISSION_NOTIFICATION_STATUS_DISABLED;
    } else if (g_ascii_strcasecmp (split[0], "enabled") == 0) {
        *notification = MBIM_TRANSMISSION_NOTIFICATION_STATUS_ENABLED;
    } else {
        g_printerr ("error: invalid state: '%s', it must be enabled or disabled\n", split[0]);
        return FALSE;
    }

    if (!mbimcli_read_uint_from_string (split[1], hysteresis_timer)) {
        g_printerr ("error: couldn't parse input string, invalid seconds '%s'\n", split[1]);
        return FALSE;
    }

    if (*hysteresis_timer < 1 || *hysteresis_timer > 5) {
        g_printerr ("error: the seconds of hysteresis_timer is %u, it must in range [1,5]\n", *hysteresis_timer);
        return FALSE;
    }

    return TRUE;
}

void
mbimcli_ms_sar_run (MbimDevice   *device,
                    GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;
    g_autoptr(GError)      error   = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to set SAR config */
    if (set_sar_config_str) {
        g_autoptr(GPtrArray) states_array = NULL;
        MbimSarControlMode   mode;
        MbimSarBackoffState  state;

        g_debug ("Asynchronously setting SAR config");
        if (!sar_config_input_parse (set_sar_config_str, &mode, &state, &states_array, &error)) {
            g_printerr ("error: couldn't parse SAR config: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_ms_sar_config_set_new (mode,
                                                      state,
                                                      states_array ? states_array->len : 0,
                                                      states_array ? (const MbimSarConfigState **)states_array->pdata : NULL,
                                                      NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)ms_sar_ready,
                             NULL);
        return;
    }

    /* Request to querying SAR config */
    if (query_sar_config_flag) {
        g_debug ("Asynchronously querying SAR config...");
        request = (mbim_message_ms_sar_config_query_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)ms_sar_ready,
                             NULL);
        return;
    }

    /* Request to set transmission status */
    if (set_transmission_status_str) {
        MbimTransmissionNotificationStatus notification     = MBIM_TRANSMISSION_NOTIFICATION_STATUS_DISABLED;
        guint32                            hysteresis_timer = 0;
        g_debug ("Asynchronously set transmission status");

        if (!transmission_status_input_parse (set_transmission_status_str,
                                              &notification,
                                              &hysteresis_timer)) {
            shutdown (FALSE);
            return;
        }

        request = mbim_message_ms_sar_transmission_status_set_new (notification, hysteresis_timer, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)modem_transmission_status_ready,
                             NULL);
        return;
    }

    /* Request to query transmission status */
    if (query_transmission_status_flag) {
        g_debug ("Asynchronously query transmission status");
        request = mbim_message_ms_sar_transmission_status_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)modem_transmission_status_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
