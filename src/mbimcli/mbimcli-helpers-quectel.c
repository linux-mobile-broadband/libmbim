/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2025 Dan Williams <dan@ioncontrol.co>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libmbim-glib.h>

#include "mbimcli-helpers.h"
#include "mbimcli-helpers-quectel.h"

gboolean
mbimcli_helpers_quectel_set_command_input_parse (const gchar             *str,
                                                 gchar                  **command_str,
                                                 MbimQuectelCommandType  *command_type)
{
    g_auto(GStrv)           split            = NULL;
    guint                   num_parts        = 0;
    g_autofree gchar       *new_command      = NULL;
    MbimQuectelCommandType  new_command_type = *command_type;

    g_assert (command_str != NULL);

    /* Format of the string is:
     *    "[\"Command\"]"
     * or:
     *    "[(Command type),(\"Command\")]"
     */
    split = g_strsplit (str, ",", -1);
    num_parts = g_strv_length (split);

    if (num_parts == 0 || !split[0]) {
        g_printerr ("error: The input string is empty, please re-enter.\n");
        return FALSE;
    }

    /* The at command may have multiple commas, like:at+qcfg="usbcfg",0x2C7C,0x6008,0x00FF ,
     * so we need to take the first split to see if it is the command type.
     * If it is, then combine the remaining splits into a string.
     * If not, combine the splits into a string. */

    if (num_parts > 1) {
        g_autofree gchar *command_type_str = g_ascii_strdown (split[0], -1);

        if (mbimcli_read_quectel_command_type_from_string (command_type_str, &new_command_type, NULL)) {
            /* Valid Quectel command-type found */
            new_command = g_strjoinv (",", split + 1);
        } else {
            /* No valid Quectel command-type found; assume plain AT command */
            new_command = g_strdup (str);
        }
    } else {
        new_command = g_strdup (str);
    }

    if (!g_str_has_prefix (new_command, "AT") && !g_str_has_prefix (new_command, "at")) {
        g_printerr ("error: Wrong AT command '%s', command must start with \"AT\".\n", new_command);
        return FALSE;
    }

    *command_str = g_strdup (new_command);
    *command_type = new_command_type;
    return TRUE;
}
