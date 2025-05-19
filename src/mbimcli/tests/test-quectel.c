/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2025 Dan Williams <dan@ioncontrol.co>
 */

#include <config.h>

#include "mbimcli-helpers-quectel.h"

#define UNSET_CMD_TYPE 999

typedef struct {
    const gchar                  *name;
    const gchar                  *type_str;
    const gchar                  *cmd_str;
    const MbimQuectelCommandType  expected_cmd_type;
    gboolean                      expect_error;
} testcase;

static void
test_set_command_input_parse_common (gconstpointer user_data)
{
    const testcase         *tc          = user_data;
    g_autofree gchar       *input_str   = NULL;
    g_autofree gchar       *parsed_cmd  = NULL;
    MbimQuectelCommandType  parsed_type = UNSET_CMD_TYPE;
    gboolean                success;

    if (tc->type_str != NULL) {
        input_str = g_strdup_printf("%s,%s", tc->type_str, tc->cmd_str);
    } else {
        input_str = g_strdup (tc->cmd_str);
    }
    success = mbimcli_helpers_quectel_set_command_input_parse (input_str,
                                                               &parsed_cmd,
                                                               &parsed_type);
    g_assert (success != tc->expect_error);
    if (!tc->expect_error) {
        g_assert_cmpstr  (parsed_cmd, ==, tc->cmd_str);
    }
    g_assert_cmpuint (parsed_type, ==, tc->expected_cmd_type);
}

int main (int argc, char **argv)
{
    gsize i;
    const testcase test_data[] = {
        /* Untyped AT commands */
        {
            .name              = "no-type",
            .cmd_str           = "at+cversion;+qgmr;+csub",
            .expected_cmd_type = UNSET_CMD_TYPE,
        },
        {
            .name              = "no-type-commas",
            .cmd_str           = "at+qcfg=\"usbcfg\",0x2C7C,0x6008,0x00FF",
            .expected_cmd_type = UNSET_CMD_TYPE,
        },
        {
            .name              = "bad-no-type",
            .cmd_str           = "bm+cversion;+qgmr;+csub",
            .expected_cmd_type = UNSET_CMD_TYPE,
            .expect_error      = TRUE,
        },
        /* AT-type AT commands */
        {
            .name              = "at-type",
            .type_str          = "at",
            .cmd_str           = "at+cversion;+qgmr;+csub",
            .expected_cmd_type = MBIM_QUECTEL_COMMAND_TYPE_AT,
        },
        {
            .name              = "at-type-commas",
            .type_str          = "at",
            .cmd_str           = "at+qcfg=\"usbcfg\",0x2C7C,0x6008,0x00FF",
            .expected_cmd_type = MBIM_QUECTEL_COMMAND_TYPE_AT,
        },
        {
            .name              = "bad-at-type",
            .type_str          = "at",
            .cmd_str           = "bm+cversion;+qgmr;+csub",
            .expected_cmd_type = UNSET_CMD_TYPE,
            .expect_error      = TRUE,
        },
    };

    g_test_init (&argc, &argv, NULL);

    for (i = 0; i < G_N_ELEMENTS(test_data); i++) {
        g_autofree gchar *test_desc;

        test_desc = g_strdup_printf("/mbimcli/quectel/set-command-input-parse/%s", test_data[i].name);
        g_test_add_data_func (test_desc, &test_data[i], test_set_command_input_parse_common);
    }

    return g_test_run ();
}
