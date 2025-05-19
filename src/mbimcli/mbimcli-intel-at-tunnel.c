/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2023 chenhaotian <jackbb_wu@compal.com>
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
static gchar *query_at_command_str;

static GOptionEntry entries[] = {
    { "intel-at-tunnel-set-at-command", 0, 0, G_OPTION_ARG_STRING, &query_at_command_str,
      "send AT command to modem, and receive AT response",
      "\"<AT command>\""
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

GOptionGroup *
mbimcli_intel_at_tunnel_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("intel-at-tunnel",
                               "Intel AT Tunnel options:",
                               "Show Intel AT Tunnel Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);
   return group;
}

gboolean
mbimcli_intel_at_tunnel_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = !!query_at_command_str;

    if (n_actions > 1) {
        g_printerr ("error: too many Intel AT Tunnel actions requested\n");
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
intel_at_tunnel_at_command_ready (MbimDevice   *device,
                                  GAsyncResult *res)
{
    g_autoptr(GError)      error     = NULL;
    guint32                ret_size  = 0;
    const guint8           *ret_str  = NULL;
    g_autoptr(MbimMessage) response  = NULL;

    response = mbim_device_command_finish (device, res, &error);

    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_intel_at_tunnel_at_command_response_parse (
            response,
            &ret_size,
            &ret_str,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("%.*s\n", ret_size, ret_str);

    shutdown (TRUE);
}

void
mbimcli_intel_at_tunnel_run (MbimDevice   *device,
                             GCancellable  *cancellable)
{
    g_autoptr(MbimMessage) request  = NULL;
    g_autofree gchar       *req_str = NULL;
    guint32                req_size = 0;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to send AT command */
    if (query_at_command_str) {
        req_str = g_strdup_printf ("%s\r\n", query_at_command_str);
        req_size = strlen (req_str);

        request = mbim_message_intel_at_tunnel_at_command_set_new (req_size, (const guint8 *)req_str, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)intel_at_tunnel_at_command_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
