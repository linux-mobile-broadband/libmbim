/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2013 - 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBMBIM_GLIB_MBIM_HELPERS_H_
#define _LIBMBIM_GLIB_MBIM_HELPERS_H_

#if !defined (LIBMBIM_GLIB_COMPILATION)
#error "This is a private header!!"
#endif

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/******************************************************************************/
/* Helpers to read variables from a bytearray without assuming alignment. The
 * caller should ensure the buffer contains the required amount of bytes in each
 * case. */

G_GNUC_INTERNAL
guint16 mbim_helpers_read_unaligned_guint16 (const guint8 *buffer);
G_GNUC_INTERNAL
guint32 mbim_helpers_read_unaligned_guint32 (const guint8 *buffer);
G_GNUC_INTERNAL
guint32 mbim_helpers_read_unaligned_gint32  (const guint8 *buffer);
G_GNUC_INTERNAL
guint64 mbim_helpers_read_unaligned_guint64 (const guint8 *buffer);

/******************************************************************************/

G_GNUC_INTERNAL
gboolean mbim_helpers_check_user_allowed (uid_t    uid,
                                          GError **error);

G_GNUC_INTERNAL
gchar *mbim_helpers_get_devpath (const gchar  *cdc_wdm_path,
                                 GError      **error);

G_GNUC_INTERNAL
gchar *mbim_helpers_get_devname (const gchar  *cdc_wdm_path,
                                 GError      **error);

G_GNUC_INTERNAL
gboolean mbim_helpers_list_links_wdm (GFile         *sysfs_file,
                                      GCancellable  *cancellable,
                                      GPtrArray     *previous_links,
                                      GPtrArray    **out_links,
                                      GError       **error);

G_GNUC_INTERNAL
gboolean mbim_helpers_list_links_wwan (const gchar   *base_ifname,
                                       GFile         *sysfs_file,
                                       GCancellable  *cancellable,
                                       GPtrArray     *previous_links,
                                       GPtrArray    **out_links,
                                       GError       **error);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_HELPERS_H_ */
