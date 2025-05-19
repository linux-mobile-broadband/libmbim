/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2025 Dan Williams <dan@ioncontrol.co>
 */

#include <glib.h>

#include <libmbim-glib.h>

#ifndef __MBIMCLI_HELPERS_QUECTEL_H__
#define __MBIMCLI_HELPERS_QUECTEL_H__

gboolean mbimcli_helpers_quectel_set_command_input_parse (const gchar             *str,
                                                          gchar                  **command_str,
                                                          MbimQuectelCommandType  *command_type);

#endif /* __MBIMCLI_HELPERS_QUECTEL_H__ */
