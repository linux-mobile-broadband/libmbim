/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2022 Ulrich Mohr
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

#include "mbimcli.h"
#include "mbimcli-helpers.h"

/* Context */
typedef struct {
    MbimDevice   *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gchar *delete_str = NULL;
static gchar *read_str = NULL;
static gboolean query_config_flag;
static gboolean query_message_store_status_flag;

static GOptionEntry entries[] = {
    { "sms-query-configuration", 0, 0, G_OPTION_ARG_NONE, &query_config_flag,
      "Query SMS configuration",
      NULL
    },
    { "sms-query-message-store-status", 0, 0, G_OPTION_ARG_NONE, &query_message_store_status_flag,
      "Query SMS message store status",
      NULL
    },
    { "sms-delete", 0, 0, G_OPTION_ARG_STRING, &delete_str,
      "Delete all SMS matching a given filter",
      "[(all|new|old|sent|draft|index=N)]"
    },
    { "sms-read", 0, 0, G_OPTION_ARG_STRING, &read_str,
      "Read all SMS matching a given filter",
      "[(all|new|old|sent|draft|index=N)]"
    },
    { NULL }
};

GOptionGroup *
mbimcli_sms_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("sms",
                                "Simple message service options:",
                                "Show SMS service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
mbimcli_sms_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!delete_str +
                 !!read_str +
                 query_config_flag +
                 query_message_store_status_flag);
    if (n_actions > 1) {
        g_printerr ("error: too many SIM actions requested\n");
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
delete_sms_ready (MbimDevice   *device,
                  GAsyncResult *res,
                  gpointer      user_data)
{
    g_autoptr(MbimMessage) response = NULL;
    g_autoptr(GError)      error = NULL;
    MbimSmsFlag            filter = GPOINTER_TO_INT (user_data);

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_sms_delete_response_parse (response, &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (filter != MBIM_SMS_FLAG_INDEX)
        g_print ("Successfully deleted %s sms\n", mbim_sms_flag_get_string (filter));
    else
        g_print ("Successfully deleted sms\n");

    shutdown (TRUE);
    return;
}

static void
read_sms_ready (MbimDevice   *device,
                GAsyncResult *res,
                gpointer      user_data)
{
    g_autoptr(MbimMessage)               response = NULL;
    g_autoptr(GError)                    error = NULL;
    g_autoptr(MbimSmsPduReadRecordArray) pdu_messages = NULL;
    guint32                              num_messages = 0;
    MbimSmsFlag                          filter = GPOINTER_TO_INT (user_data);

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_sms_read_response_parse (response, NULL, &num_messages, &pdu_messages, NULL, &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (num_messages && pdu_messages) {
        guint32 i = 0;

        g_print ("Successfully read sms\n");
        if (filter != MBIM_SMS_FLAG_INDEX)
            g_print ("Got %d messages\n", num_messages);
        while (pdu_messages[i]) {
            g_print ("  PDU on index %u, status %s\n", pdu_messages[i]->message_index, mbim_sms_status_get_string (pdu_messages[i]->message_status));
            i++;
        }
    } else {
        if (filter == MBIM_SMS_FLAG_ALL)
            g_print ("No messages found\n");
        else if (filter != MBIM_SMS_FLAG_INDEX)
            g_print ("No %s messages found\n", mbim_sms_flag_get_string (filter) );
        else
            g_print ("Message not found\n");
    }

    shutdown (TRUE);
    return;
}

static void
query_sms_config_ready (MbimDevice   *device,
                        GAsyncResult *res,
                        gpointer      user_data)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    MbimSmsStorageState     storage_state = MBIM_SMS_STORAGE_STATE_NOT_INITIALIZED;
    const gchar            *storage_state_str;
    MbimSmsFormat           format = MBIM_SMS_FORMAT_PDU;
    const gchar            *format_str;
    guint32                 max_messages = 0;
    guint32                 cdma_short_message_size = 0;
    g_autofree gchar       *sc_address = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_sms_configuration_response_parse (response,
                                                        &storage_state,
                                                        &format,
                                                        &max_messages,
                                                        &cdma_short_message_size,
                                                        &sc_address,
                                                        &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    storage_state_str = mbim_sms_storage_state_get_string (storage_state);
    format_str = mbim_sms_format_get_string (format);

    g_print ("[%s] SMS capabilities retrieved:\n"
             "\t          Storage state: '%s'\n"
             "\t                 Format: '%s'\n"
             "\t       Max PDU messages: '%u'\n"
             "\tCDMA short message size: '%u'\n"
             "\t Service center address: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (storage_state_str),
             VALIDATE_UNKNOWN (format_str),
             max_messages,
             cdma_short_message_size,
             sc_address);

    shutdown (TRUE);
    return;
}

static void
query_sms_message_store_status_ready (MbimDevice   *device,
                                      GAsyncResult *res,
                                      gpointer      user_data)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    MbimSmsStatusFlag       status = MBIM_SMS_STATUS_FLAG_NONE;
    const gchar            *status_str;
    guint                   message_index = 0;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_sms_message_store_status_response_parse (response,
                                                               &status,
                                                               &message_index,
                                                               &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    status_str = mbim_sms_status_flag_build_string_from_mask (status);

    g_print ("[%s] SMS message store status retrieved:\n"
             "\t       Status: '%s'\n"
             "\tMessage index: '%u'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (status_str),
             message_index);

    shutdown (TRUE);
    return;
}

static gboolean
op_parse (const gchar *str,
          MbimSmsFlag *filter,
          guint32     *index)
{
    gboolean          status       = FALSE;
    g_auto(GStrv)     filter_parts = NULL;
    g_autoptr(GError) error        = NULL;

    g_assert (filter != NULL);

    /* according to the mbim specification, index must be > 0 and 0 is used when not needed */
    *index = 0;

    filter_parts = g_strsplit (str, "=", -1);
    if (!filter_parts[0]) {
        g_printerr ("error: invalid sms filter: %s\n", str);
        return FALSE;
    }

    status = mbimcli_read_sms_flag_from_string (filter_parts[0], filter, &error);
    if (!status) {
        g_printerr ("error: invalid sms flag: %s\n", error->message);
        return FALSE;
    }

    if (*filter == MBIM_SMS_FLAG_INDEX) {
        status = filter_parts[1] && mbimcli_read_uint_from_string (filter_parts[1], index);
        if (!status) {
            if (!filter_parts[1])
                g_printerr ("error: required index not given\n");
            else
                g_printerr ("error: couln't parse sms index, should be a number\n");
            return FALSE;
        }
        /* status must be TRUE; validate index */
        if (*index == 0) {
            g_printerr ("error: index must be > 0\n");
            return FALSE;
        }
    } else if (filter_parts[1]) {
        g_printerr ("error: unexpected assignment for the given operation\n");
        return FALSE;
    }

    return status;
}

void
mbimcli_sms_run (MbimDevice   *device,
                 GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;
    g_autoptr(GError)      error = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    if (delete_str) {
        MbimSmsFlag filter;
        guint32 index;

        if (!op_parse (delete_str, &filter, &index)) {
            shutdown (FALSE);
            return;
        }
        request = mbim_message_sms_delete_set_new (filter, index, &error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)delete_sms_ready,
                             GINT_TO_POINTER (filter));
        return;
    }

    if (read_str) {
        MbimSmsFlag filter;
        guint32 index;

        if (!op_parse (read_str, &filter, &index)) {
            shutdown (FALSE);
            return;
        }
        request = mbim_message_sms_read_query_new (MBIM_SMS_FORMAT_PDU, filter, index, &error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)read_sms_ready,
                             GINT_TO_POINTER (filter));
        return;
    }

    if (query_config_flag) {
        request = mbim_message_sms_configuration_query_new (&error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_sms_config_ready,
                             NULL);
        return;
    }

    if (query_message_store_status_flag) {
        request = mbim_message_sms_message_store_status_query_new (&error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_sms_message_store_status_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
