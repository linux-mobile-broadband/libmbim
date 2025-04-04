/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbim-proxy -- A proxy to communicate with MBIM ports
 *
 * Copyright (C) 2014-2023 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2014 Smith Micro Software, Inc.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <glib-unix.h>

#include <libmbim-glib.h>

#define PROGRAM_NAME    "mbim-proxy"
#define PROGRAM_VERSION PACKAGE_VERSION

#define EMPTY_TIMEOUT_DEFAULT 300

/* Globals */
static GMainLoop *loop;
static MbimProxy *proxy;
static guint timeout_id;
static guint client_connected_once = FALSE;

/* Main options */
static gboolean verbose_flag;
static gboolean verbose_full_flag;
static gboolean version_flag;
static gboolean no_exit_flag;
static gint     empty_timeout = -1;

static GOptionEntry main_entries[] = {
    { "no-exit", 0, 0, G_OPTION_ARG_NONE, &no_exit_flag,
      "Don't exit after being idle without clients/devices",
      NULL
    },
    { "empty-timeout", 0, 0, G_OPTION_ARG_INT, &empty_timeout,
      "If no clients/devices, exit after this timeout. If set to 0, equivalent to --no-exit.",
      "[SECS]"
    },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose_flag,
      "Run action with verbose logs, including the debug ones",
      NULL
    },
    { "verbose-full", 0, 0, G_OPTION_ARG_NONE, &verbose_full_flag,
      "Run action with verbose logs, including the debug ones and personal info",
      NULL
    },
    { "version", 'V', 0, G_OPTION_ARG_NONE, &version_flag,
      "Print version",
      NULL
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

static gboolean
quit_cb (gpointer user_data)
{
    if (loop) {
        g_warning ("Caught signal, stopping the loop...");
        g_idle_add ((GSourceFunc) g_main_loop_quit, loop);
    }

    return FALSE;
}

static void
log_handler (const gchar    *log_domain,
             GLogLevelFlags  log_level,
             const gchar    *message,
             gpointer        user_data)
{
    const gchar *log_level_str;
    time_t       now;
    gchar        time_str[64];
    struct tm   *local_time;
    gboolean     err = FALSE;

    switch (log_level) {
    case G_LOG_LEVEL_WARNING:
        log_level_str = "-Warning **";
        err = TRUE;
        break;

    case G_LOG_LEVEL_CRITICAL:
    case G_LOG_FLAG_FATAL:
    case G_LOG_LEVEL_ERROR:
        log_level_str = "-Error **";
        err = TRUE;
        break;

    case G_LOG_LEVEL_DEBUG:
        log_level_str = "[Debug]";
        break;

    case G_LOG_LEVEL_MESSAGE:
    case G_LOG_LEVEL_INFO:
        log_level_str = "";
        break;

    case G_LOG_LEVEL_MASK:
    case G_LOG_FLAG_RECURSION:
    default:
        g_assert_not_reached ();
    }

    if (!verbose_flag && !verbose_full_flag && !err)
        return;

    now = time ((time_t *) NULL);
    local_time = localtime (&now);
    strftime (time_str, 64, "%d %b %Y, %H:%M:%S", local_time);

    g_fprintf (err ? stderr : stdout,
               "[%s] %s %s\n",
               time_str,
               log_level_str,
               message);
}

G_GNUC_NORETURN
static void
print_version_and_exit (void)
{
    g_print ("\n"
             PROGRAM_NAME " " PROGRAM_VERSION "\n"
             "Copyright (C) 2013-2023 Aleksander Morgado\n"
             "Copyright (C) 2014-2018 Greg Suarez\n"
             "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl-2.0.html>\n"
             "This is free software: you are free to change and redistribute it.\n"
             "There is NO WARRANTY, to the extent permitted by law.\n"
             "\n");
    exit (EXIT_SUCCESS);
}

/*****************************************************************************/

static gboolean
stop_loop_cb (void)
{
    timeout_id = 0;
    if (loop)
        g_main_loop_quit (loop);
    return FALSE;
}

static void
proxy_n_clients_changed (MbimProxy *_proxy)
{
    /* once a client has connected only exit if there are no devices */
    if (client_connected_once)
        return;

    if (mbim_proxy_get_n_clients (proxy) == 0) {
        g_assert (timeout_id == 0);
        g_assert (empty_timeout > 0);
        timeout_id = g_timeout_add_seconds (empty_timeout,
                                            (GSourceFunc)stop_loop_cb,
                                            NULL);
        return;
    }

    /* At least one client, remove timeout if any */
    if (timeout_id) {
        g_source_remove (timeout_id);
        timeout_id = 0;
    }

    client_connected_once = TRUE;
}

static void
proxy_n_devices_changed (MbimProxy *_proxy)
{
    if (mbim_proxy_get_n_devices (proxy) == 0) {
        g_assert (timeout_id == 0);
        g_assert (empty_timeout > 0);
        timeout_id = g_timeout_add_seconds (empty_timeout,
                                            (GSourceFunc)stop_loop_cb,
                                            NULL);
        return;
    }

    /* At least one device, remove timeout if any */
    if (timeout_id) {
        g_source_remove (timeout_id);
        timeout_id = 0;
    }
}

/*****************************************************************************/

int main (int argc, char **argv)
{
    g_autoptr(GError)         error = NULL;
    g_autoptr(GOptionContext) context = NULL;

    setlocale (LC_ALL, "");

    /* Setup option context, process it and destroy it */
    context = g_option_context_new ("- Proxy for MBIM devices");
    g_option_context_add_main_entries (context, main_entries, NULL);
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("error: %s\n", error->message);
        exit (EXIT_FAILURE);
    }

    if (version_flag)
        print_version_and_exit ();

    g_log_set_handler (NULL,  G_LOG_LEVEL_MASK, log_handler, NULL);
    g_log_set_handler ("Mbim", G_LOG_LEVEL_MASK, log_handler, NULL);
    if (verbose_flag && verbose_full_flag) {
        g_printerr ("error: cannot specify --verbose and --verbose-full at the same time\n");
        exit (EXIT_FAILURE);
    } else if (verbose_flag) {
        mbim_utils_set_traces_enabled (TRUE);
        mbim_utils_set_show_personal_info (FALSE);
    } else if (verbose_full_flag) {
        mbim_utils_set_traces_enabled (TRUE);
        mbim_utils_set_show_personal_info (TRUE);
    }

    /* Setup signals */
    g_unix_signal_add (SIGINT,  quit_cb, NULL);
    g_unix_signal_add (SIGHUP,  quit_cb, NULL);
    g_unix_signal_add (SIGTERM, quit_cb, NULL);

    /* Setup empty timeout */
    if (empty_timeout < 0)
        empty_timeout = EMPTY_TIMEOUT_DEFAULT;

    /* Setup proxy */
    proxy = mbim_proxy_new (&error);
    if (!proxy) {
        g_printerr ("error: %s\n", error->message);
        exit (EXIT_FAILURE);
    }

    /* Don't exit the proxy when no clients/devices are found */
    if (!no_exit_flag && empty_timeout != 0) {
        g_debug ("proxy will exit after %d secs if unused", empty_timeout);
        proxy_n_clients_changed (proxy);
        g_signal_connect (proxy,
                          "notify::" MBIM_PROXY_N_CLIENTS,
                          G_CALLBACK (proxy_n_clients_changed),
                          NULL);
        g_signal_connect (proxy,
                          "notify::" MBIM_PROXY_N_DEVICES,
                          G_CALLBACK (proxy_n_devices_changed),
                          NULL);
    } else
        g_debug ("proxy will remain running if unused");

    /* Loop */
    loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (loop);
    g_main_loop_unref (loop);

    /* Cleanup; releases socket and such */
    g_object_unref (proxy);

    g_debug ("exiting 'mbim-proxy'...");

    return EXIT_SUCCESS;
}
