/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details:
 *
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 */

#include <config.h>
#include <string.h>

#include "mbim-message.h"
#include "mbim-message-private.h"
#include "mbim-cid.h"
#include "mbim-enums.h"
#include "mbim-utils.h"
#include "mbim-basic-connect.h"
#include "mbim-ussd.h"
#include "mbim-auth.h"
#include "mbim-stk.h"

#if defined ENABLE_TEST_MESSAGE_TRACES
static void
test_message_trace (const guint8 *computed,
                    guint32       computed_size,
                    const guint8 *expected,
                    guint32       expected_size)
{
    gchar *message_str;
    gchar *expected_str;

    message_str = __mbim_utils_str_hex (computed, computed_size, ':');
    expected_str = __mbim_utils_str_hex (expected, expected_size, ':');

    /* Dump all message contents */
    g_print ("\n"
             "Message str:\n"
             "'%s'\n"
             "Expected str:\n"
             "'%s'\n",
             message_str,
             expected_str);

    /* If they are different, tell which are the different bytes */
    if (computed_size != expected_size ||
        memcmp (computed, expected, expected_size)) {
        guint32 i;

        for (i = 0; i < MIN (computed_size, expected_size); i++) {
            if (computed[i] != expected[i])
                g_print ("Byte [%u] is different (computed: 0x%02X vs expected: 0x%02x)\n", i, computed[i], expected[i]);
        }
    }

    g_free (message_str);
    g_free (expected_str);
}
#else
#define test_message_trace(...)
#endif

static void
test_message_builder_basic_connect_pin_set_raw (void)
{
    MbimMessage *message;
    MbimMessageCommandBuilder *builder;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x50, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xa2, 0x89, 0xcc, 0x33, /* service id */
        0xbc, 0xbb, 0x8b, 0x4f,
        0xb6, 0xb0, 0x13, 0x3e,
        0xc2, 0xaa, 0xe6, 0xdf,
        0x04, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x20, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x02, 0x00, 0x00, 0x00, /* pin type */
        0x00, 0x00, 0x00, 0x00, /* pin operation */
        0x18, 0x00, 0x00, 0x00, /* pin offset */
        0x08, 0x00, 0x00, 0x00, /* pin size */
        0x00, 0x00, 0x00, 0x00, /* new pin offset */
        0x00, 0x00, 0x00, 0x00, /* new pin size */
        0x31, 0x00, 0x31, 0x00, /* pin string */
        0x31, 0x00, 0x31, 0x00
    };

    /* PIN set message */
    builder = _mbim_message_command_builder_new (1,
                                                 MBIM_SERVICE_BASIC_CONNECT,
                                                 MBIM_CID_BASIC_CONNECT_PIN,
                                                 MBIM_MESSAGE_COMMAND_TYPE_SET);
    _mbim_message_command_builder_append_guint32 (builder, (guint32)MBIM_PIN_TYPE_PIN1);
    _mbim_message_command_builder_append_guint32 (builder, (guint32)MBIM_PIN_OPERATION_ENTER);
    _mbim_message_command_builder_append_string  (builder, "1111");
    _mbim_message_command_builder_append_string  (builder, "");
    message = _mbim_message_command_builder_complete (builder);

    g_assert (message != NULL);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_BASIC_CONNECT_PIN);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_basic_connect_pin_set (void)
{
    GError *error = NULL;
    MbimMessage *message;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x50, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xa2, 0x89, 0xcc, 0x33, /* service id */
        0xbc, 0xbb, 0x8b, 0x4f,
        0xb6, 0xb0, 0x13, 0x3e,
        0xc2, 0xaa, 0xe6, 0xdf,
        0x04, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x20, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x02, 0x00, 0x00, 0x00, /* pin type */
        0x00, 0x00, 0x00, 0x00, /* pin operation */
        0x18, 0x00, 0x00, 0x00, /* pin offset */
        0x08, 0x00, 0x00, 0x00, /* pin size */
        0x00, 0x00, 0x00, 0x00, /* new pin offset */
        0x00, 0x00, 0x00, 0x00, /* new pin size */
        0x31, 0x00, 0x31, 0x00, /* pin string */
        0x31, 0x00, 0x31, 0x00
    };

    /* PIN set message */
    message = mbim_message_pin_set_new (MBIM_PIN_TYPE_PIN1,
                                        MBIM_PIN_OPERATION_ENTER,
                                        "1111",
                                        "",
                                        &error);
    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_BASIC_CONNECT_PIN);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_basic_connect_connect_set_raw (void)
{
    MbimMessage *message;
    MbimMessageCommandBuilder *builder;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x7C, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x0C, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x4C, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* session id */
        0x01, 0x00, 0x00, 0x00, /* activation command */
        0x3C, 0x00, 0x00, 0x00, /* access string offset */
        0x10, 0x00, 0x00, 0x00, /* access string size */
        0x00, 0x00, 0x00, 0x00, /* username offset */
        0x00, 0x00, 0x00, 0x00, /* username size */
        0x00, 0x00, 0x00, 0x00, /* password offset */
        0x00, 0x00, 0x00, 0x00, /* password size */
        0x00, 0x00, 0x00, 0x00, /* compression */
        0x01, 0x00, 0x00, 0x00, /* auth protocol */
        0x01, 0x00, 0x00, 0x00, /* ip type */
        0x7E, 0x5E, 0x2A, 0x7E, /* context type */
        0x4E, 0x6F, 0x72, 0x72,
        0x73, 0x6B, 0x65, 0x6E,
        0x7E, 0x5E, 0x2A, 0x7E,
        /* data buffer */
        0x69, 0x00, 0x6E, 0x00, /* access string */
        0x74, 0x00, 0x65, 0x00,
        0x72, 0x00, 0x6E, 0x00,
        0x65, 0x00, 0x74, 0x00
    };

    /* CONNECT set message */
    builder = _mbim_message_command_builder_new (1,
                                                 MBIM_SERVICE_BASIC_CONNECT,
                                                 MBIM_CID_BASIC_CONNECT_CONNECT,
                                                 MBIM_MESSAGE_COMMAND_TYPE_SET);
    _mbim_message_command_builder_append_guint32 (builder, 0x01);
    _mbim_message_command_builder_append_guint32 (builder, (guint32)MBIM_ACTIVATION_COMMAND_ACTIVATE);
    _mbim_message_command_builder_append_string  (builder, "internet");
    _mbim_message_command_builder_append_string  (builder, "");
    _mbim_message_command_builder_append_string  (builder, "");
    _mbim_message_command_builder_append_guint32 (builder, (guint32)MBIM_COMPRESSION_NONE);
    _mbim_message_command_builder_append_guint32 (builder, (guint32)MBIM_AUTH_PROTOCOL_PAP);
    _mbim_message_command_builder_append_guint32 (builder, (guint32)MBIM_CONTEXT_IP_TYPE_IPV4);
    _mbim_message_command_builder_append_uuid    (builder, mbim_uuid_from_context_type (MBIM_CONTEXT_TYPE_INTERNET));
    message = _mbim_message_command_builder_complete (builder);

    g_assert (message != NULL);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_BASIC_CONNECT_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_basic_connect_connect_set (void)
{
    GError *error = NULL;
    MbimMessage *message;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x7C, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x0C, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x4C, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* session id */
        0x01, 0x00, 0x00, 0x00, /* activation command */
        0x3C, 0x00, 0x00, 0x00, /* access string offset */
        0x10, 0x00, 0x00, 0x00, /* access string size */
        0x00, 0x00, 0x00, 0x00, /* username offset */
        0x00, 0x00, 0x00, 0x00, /* username size */
        0x00, 0x00, 0x00, 0x00, /* password offset */
        0x00, 0x00, 0x00, 0x00, /* password size */
        0x00, 0x00, 0x00, 0x00, /* compression */
        0x01, 0x00, 0x00, 0x00, /* auth protocol */
        0x01, 0x00, 0x00, 0x00, /* ip type */
        0x7E, 0x5E, 0x2A, 0x7E, /* context type */
        0x4E, 0x6F, 0x72, 0x72,
        0x73, 0x6B, 0x65, 0x6E,
        0x7E, 0x5E, 0x2A, 0x7E,
        /* data buffer */
        0x69, 0x00, 0x6E, 0x00, /* access string */
        0x74, 0x00, 0x65, 0x00,
        0x72, 0x00, 0x6E, 0x00,
        0x65, 0x00, 0x74, 0x00
    };

    /* CONNECT set message */
    message = (mbim_message_connect_set_new (
                   0x01,
                   MBIM_ACTIVATION_COMMAND_ACTIVATE,
                   "internet",
                   "",
                   "",
                   MBIM_COMPRESSION_NONE,
                   MBIM_AUTH_PROTOCOL_PAP,
                   MBIM_CONTEXT_IP_TYPE_IPV4,
                   mbim_uuid_from_context_type (MBIM_CONTEXT_TYPE_INTERNET),
                   &error));
    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_BASIC_CONNECT_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_basic_connect_service_activation_set (void)
{
    GError *error = NULL;
    MbimMessage *message;
    const guint8 buffer [] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08
    };
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x38, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x0E, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x08, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08 };

    /* CONNECT set message */
    message = (mbim_message_service_activation_set_new (
                   sizeof (buffer),
                   buffer,
                   &error));
    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_BASIC_CONNECT_SERVICE_ACTIVATION);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_basic_connect_device_service_subscriber_list_set (void)
{
    GError *error = NULL;
    MbimEventEntry **entries;
    MbimMessage *message;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x78, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x13, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x48, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x02, 0x00, 0x00, 0x00, /* 0x00 Events count */
        0x14, 0x00, 0x00, 0x00, /* 0x04 Event 1 offset */
        0x1C, 0x00, 0x00, 0x00, /* 0x08 Event 1 length */
        0x30, 0x00, 0x00, 0x00, /* 0x0C Event 2 offset */
        0x18, 0x00, 0x00, 0x00, /* 0x10 Event 2 length */
        /* data buffer, event 1 */
        0xA2, 0x89, 0xCC, 0x33, /* 0x14 Event 1, service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x02, 0x00, 0x00, 0x00, /* 0x24 Event 1, cid count */
        0x0B, 0x00, 0x00, 0x00, /* 0x28 Event 1, cid 1 */
        0x09, 0x00, 0x00, 0x00, /* 0x2C Event 1, cid 2 */
        /* data buffer, event 2 */
        0x53, 0x3F, 0xBE, 0xEB, /* 0x30 Event 1, service id */
        0x14, 0xFE, 0x44, 0x67,
        0x9F, 0x90, 0x33, 0xA2,
        0x23, 0xE5, 0x6C, 0x3F,
        0x01, 0x00, 0x00, 0x00, /* 0x40 Event 2, cid count */
        0x02, 0x00, 0x00, 0x00, /* 0x44 Event 2, cid 1 */
    };

    /* Device Service Subscribe List set message */
    entries = g_new0 (MbimEventEntry *, 3);

    entries[0] = g_new (MbimEventEntry, 1);
    memcpy (&(entries[0]->device_service_id), MBIM_UUID_BASIC_CONNECT, sizeof (MbimUuid));
    entries[0]->cids_count = 2;
    entries[0]->cids = g_new0 (guint32, 2);
    entries[0]->cids[0] = MBIM_CID_BASIC_CONNECT_SIGNAL_STATE;
    entries[0]->cids[1] = MBIM_CID_BASIC_CONNECT_REGISTER_STATE;

    entries[1] = g_new (MbimEventEntry, 1);
    memcpy (&(entries[1]->device_service_id), MBIM_UUID_SMS, sizeof (MbimUuid));
    entries[1]->cids_count = 1;
    entries[1]->cids = g_new0 (guint32, 1);
    entries[1]->cids[0] = MBIM_CID_SMS_READ;

    message = (mbim_message_device_service_subscriber_list_set_new (
                   2,
                   (const MbimEventEntry *const *)entries,
                   &error));
    mbim_event_entry_array_free (entries);

    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_BASIC_CONNECT_DEVICE_SERVICE_SUBSCRIBER_LIST);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_ussd_set (void)
{
    GError *error = NULL;
    MbimMessage *message;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x50, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xE5, 0x50, 0xA0, 0xC8, /* service id */
        0x5E, 0x82, 0x47, 0x9E,
        0x82, 0xF7, 0x10, 0xAB,
        0xF4, 0xC3, 0x35, 0x1F,
        0x01, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x20, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* 0x00 Action */
        0x01, 0x00, 0x00, 0x00, /* 0x04 Data coding scheme */
        0x10, 0x00, 0x00, 0x00, /* 0x08 Payload offset */
        0x10, 0x00, 0x00, 0x00, /* 0x0C Payload length */
        /* data buffer, payload */
        0x01, 0x02, 0x03, 0x04, /* 0x10 Payload */
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F, 0x00
    };
    const guint8 payload [] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F, 0x00
    };

    /* USSD set message */
    message = (mbim_message_ussd_set_new (
                   MBIM_USSD_ACTION_CONTINUE,
                   1, /* dcs */
                   sizeof (payload),
                   payload,
                   &error));

    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_USSD);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_USSD);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_auth_akap_query (void)
{
    GError *error = NULL;
    MbimMessage *message;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x60, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0x1D, 0x2B, 0x5F, 0xF7, /* service id */
        0x0A, 0xA1, 0x48, 0xB2,
        0xAA, 0x52, 0x50, 0xF1,
        0x57, 0x67, 0x17, 0x4E,
        0x02, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* command_type */
        0x30, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x00, 0x01, 0x02, 0x03, /* 0x00 Rand */
        0x04, 0x05, 0x06, 0x07, /* 0x04 */
        0x08, 0x09, 0x0A, 0x0B, /* 0x08 */
        0x0C, 0x0D, 0x0E, 0x0F, /* 0x0C */
        0xFF, 0xFE, 0xFD, 0xFC, /* 0x10 Autn */
        0xFB, 0xFA, 0xF9, 0xF8, /* 0x14 */
        0xF7, 0xF6, 0xF5, 0xF4, /* 0x18 */
        0xF3, 0xF2, 0xF1, 0xF0, /* 0x1C */
        0x28, 0x00, 0x00, 0x00, /* 0x20 Network name (offset) */
        0x08, 0x00, 0x00, 0x00, /* 0x24 Network name (length) */
        /* data buffer */
        0x31, 0x00, 0x31, 0x00, /* 0x28 Network name */
        0x31, 0x00, 0x31, 0x00
    };

    const guint8 rand [] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F
    };
    const guint8 autn [] = {
        0xFF, 0xFE, 0xFD, 0xFC,
        0xFB, 0xFA, 0xF9, 0xF8,
        0xF7, 0xF6, 0xF5, 0xF4,
        0xF3, 0xF2, 0xF1, 0xF0
    };

    /* AKAP Auth message */
    message = (mbim_message_auth_akap_query_new (
                   rand,
                   autn,
                   "1111",
                   &error));

    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_AUTH);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_AUTH_AKAP);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_QUERY);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_stk_pac_set (void)
{
    GError *error = NULL;
    MbimMessage *message;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x50, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xD8, 0xF2, 0x01, 0x31, /* service id */
        0xFC, 0xB5, 0x4E, 0x17,
        0x86, 0x02, 0xD6, 0xED,
        0x38, 0x16, 0x16, 0x4C,
        0x01, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x20, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F,
        0xFF, 0xFE, 0xFD, 0xFC,
        0xFB, 0xFA, 0xF9, 0xF8,
        0xF7, 0xF6, 0xF5, 0xF4,
        0xF3, 0xF2, 0xF1, 0xF0
    };

    const guint8 pac_host_control [] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F,
        0xFF, 0xFE, 0xFD, 0xFC,
        0xFB, 0xFA, 0xF9, 0xF8,
        0xF7, 0xF6, 0xF5, 0xF4,
        0xF3, 0xF2, 0xF1, 0xF0
    };

    /* STK PAC set message */
    message = (mbim_message_stk_pac_set_new (
                   pac_host_control,
                   &error));

    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_STK);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_STK_PAC);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_stk_terminal_response_set (void)
{
    GError *error = NULL;
    MbimMessage *message;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x50, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xD8, 0xF2, 0x01, 0x31, /* service id */
        0xFC, 0xB5, 0x4E, 0x17,
        0x86, 0x02, 0xD6, 0xED,
        0x38, 0x16, 0x16, 0x4C,
        0x02, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x20, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x1C, 0x00, 0x00, 0x00, /* 0x00 Response length */
        0x04, 0x05, 0x06, 0x07, /* 0x04 Response */
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F,
        0xFF, 0xFE, 0xFD, 0xFC,
        0xFB, 0xFA, 0xF9, 0xF8,
        0xF7, 0xF6, 0xF5, 0xF4,
        0xF3, 0xF2, 0xF1, 0xF0
    };

    const guint8 response [] = {
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F,
        0xFF, 0xFE, 0xFD, 0xFC,
        0xFB, 0xFA, 0xF9, 0xF8,
        0xF7, 0xF6, 0xF5, 0xF4,
        0xF3, 0xF2, 0xF1, 0xF0
    };

    /* STK Terminal Response set message */
    message = (mbim_message_stk_terminal_response_set_new (
                   sizeof (response),
                   response,
                   &error));

    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_STK);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_STK_TERMINAL_RESPONSE);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_stk_envelope_set (void)
{
    GError *error = NULL;
    MbimMessage *message;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x58, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xD8, 0xF2, 0x01, 0x31, /* service id */
        0xFC, 0xB5, 0x4E, 0x17,
        0x86, 0x02, 0xD6, 0xED,
        0x38, 0x16, 0x16, 0x4C,
        0x03, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x28, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x1C, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F,
        0xFF, 0xFE, 0xFD, 0xFC,
        0xFB, 0xFA, 0xF9, 0xF8,
        0xF7, 0xF6, 0xF5, 0xF4,
        0xF3, 0xF2, 0xF1, 0xF0,
        0xFF, 0xFE, 0xFD, 0xFC,
        0xFB, 0xFA, 0xF9, 0xF8
    };

    const guint8 databuffer [] = {
        0x1C, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F,
        0xFF, 0xFE, 0xFD, 0xFC,
        0xFB, 0xFA, 0xF9, 0xF8,
        0xF7, 0xF6, 0xF5, 0xF4,
        0xF3, 0xF2, 0xF1, 0xF0,
        0xFF, 0xFE, 0xFD, 0xFC,
        0xFB, 0xFA, 0xF9, 0xF8
    };

    /* STK Envelop set message */
    message = (mbim_message_stk_envelope_set_new (
                   sizeof (databuffer),
                   databuffer,
                   &error));

    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_STK);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_STK_ENVELOPE);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_basic_connect_ip_packet_filters_set_none (void)
{
    GError *error = NULL;
    MbimMessage *message;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x38, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x17, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x08, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* 0x00 session id */
        0x00, 0x00, 0x00, 0x00  /* 0x04 packet filters count */
    };

    /* IP packet filters set message */
    message = mbim_message_ip_packet_filters_set_new (1, 0, NULL, &error);

    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_BASIC_CONNECT_IP_PACKET_FILTERS);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_builder_basic_connect_ip_packet_filters_set_one (void)
{
    GError *error = NULL;
    MbimMessage *message;
    MbimPacketFilter **filters;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x5C, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x17, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x2C, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* 0x00 session id */
        0x01, 0x00, 0x00, 0x00, /* 0x04 packet filters count */
        0x10, 0x00, 0x00, 0x00, /* 0x08 packet filter 1 offset */
        0x1C, 0x00, 0x00, 0x00, /* 0x0C packet filter 1 length */
        /* databuffer, packet filter 1 */
        0x08, 0x00, 0x00, 0x00, /* 0x10 0x00 filter size */
        0x0C, 0x00, 0x00, 0x00, /* 0x14 0x04 filter offset (from beginning of struct) */
        0x14, 0x00, 0x00, 0x00, /* 0x18 0x08 mask offset (from beginning of struct) */
        0x01, 0x02, 0x03, 0x04, /* 0x1C 0x0C filter */
        0x05, 0x06, 0x07, 0x08,
        0xF1, 0xF2, 0xF3, 0xF4, /* 0x24 0x14 mask */
        0xF5, 0xF6, 0xF7, 0xF8,
    };

    const guint8 filter[] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
    };
    const guint8 mask[] = {
        0xF1, 0xF2, 0xF3, 0xF4,
        0xF5, 0xF6, 0xF7, 0xF8,
    };


    filters = g_new0 (MbimPacketFilter *, 2);
    filters[0] = g_new (MbimPacketFilter, 1);
    filters[0]->filter_size = 8;
    filters[0]->packet_filter = g_new (guint8, 8);
    memcpy (filters[0]->packet_filter, filter, 8);
    filters[0]->packet_mask = g_new (guint8, 8);
    memcpy (filters[0]->packet_mask, mask, 8);

    /* IP packet filters set message */
    message = (mbim_message_ip_packet_filters_set_new (
                   1,
                   1,
                   (const MbimPacketFilter * const*)filters,
                   &error));

    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_BASIC_CONNECT_IP_PACKET_FILTERS);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);

    mbim_packet_filter_array_free (filters);
}

static void
test_message_builder_basic_connect_ip_packet_filters_set_two (void)
{
    GError *error = NULL;
    MbimMessage *message;
    MbimPacketFilter **filters;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x88, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x17, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x58, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* 0x00 session id */
        0x02, 0x00, 0x00, 0x00, /* 0x04 packet filters count */
        0x18, 0x00, 0x00, 0x00, /* 0x08 packet filter 1 offset */
        0x1C, 0x00, 0x00, 0x00, /* 0x0C packet filter 1 length */
        0x34, 0x00, 0x00, 0x00, /* 0x10 packet filter 2 offset */
        0x24, 0x00, 0x00, 0x00, /* 0x14 packet filter 2 length */
        /* databuffer, packet filter 2 */
        0x08, 0x00, 0x00, 0x00, /* 0x18 0x00 filter size */
        0x0C, 0x00, 0x00, 0x00, /* 0x1C 0x04 filter offset (from beginning of struct) */
        0x14, 0x00, 0x00, 0x00, /* 0x20 0x08 mask offset (from beginning of struct) */
        0x01, 0x02, 0x03, 0x04, /* 0x24 0x0C filter */
        0x05, 0x06, 0x07, 0x08,
        0xF1, 0xF2, 0xF3, 0xF4, /* 0x2C 0x14 mask */
        0xF5, 0xF6, 0xF7, 0xF8,
        /* databuffer, packet filter 2 */
        0x0C, 0x00, 0x00, 0x00, /* 0x34 0x00 filter size */
        0x0C, 0x00, 0x00, 0x00, /* 0x38 0x04 filter offset (from beginning of struct) */
        0x18, 0x00, 0x00, 0x00, /* 0x3C 0x08 mask offset (from beginning of struct) */
        0x01, 0x02, 0x03, 0x04, /* 0x40 0x0C filter */
        0x05, 0x06, 0x07, 0x08,
        0x05, 0x06, 0x07, 0x08,
        0xF1, 0xF2, 0xF3, 0xF4, /* 0x4C 0x18 mask */
        0xF5, 0xF6, 0xF7, 0xF8,
        0xF5, 0xF6, 0xF7, 0xF8,
    };

    const guint8 filter1[] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
    };
    const guint8 mask1[] = {
        0xF1, 0xF2, 0xF3, 0xF4,
        0xF5, 0xF6, 0xF7, 0xF8,
    };
    const guint8 filter2[] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x05, 0x06, 0x07, 0x08,
    };
    const guint8 mask2[] = {
        0xF1, 0xF2, 0xF3, 0xF4,
        0xF5, 0xF6, 0xF7, 0xF8,
        0xF5, 0xF6, 0xF7, 0xF8,
    };

    filters = g_new0 (MbimPacketFilter *, 3);
    filters[0] = g_new (MbimPacketFilter, 1);
    filters[0]->filter_size = 8;
    filters[0]->packet_filter = g_new (guint8, 8);
    memcpy (filters[0]->packet_filter, filter1, 8);
    filters[0]->packet_mask = g_new (guint8, 8);
    memcpy (filters[0]->packet_mask, mask1, 8);
    filters[1] = g_new (MbimPacketFilter, 1);
    filters[1]->filter_size = 12;
    filters[1]->packet_filter = g_new (guint8, 12);
    memcpy (filters[1]->packet_filter, filter2, 12);
    filters[1]->packet_mask = g_new (guint8, 12);
    memcpy (filters[1]->packet_mask, mask2, 12);

    /* IP packet filters set message */
    message = (mbim_message_ip_packet_filters_set_new (
                   1,
                   2,
                   (const MbimPacketFilter * const*)filters,
                   &error));

    g_assert_no_error (error);
    g_assert (message != NULL);
    mbim_message_set_transaction_id (message, 1);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_BASIC_CONNECT_IP_PACKET_FILTERS);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);

    mbim_packet_filter_array_free (filters);
}

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libmbim-glib/message/builder/basic-connect/pin/set/raw", test_message_builder_basic_connect_pin_set_raw);
    g_test_add_func ("/libmbim-glib/message/builder/basic-connect/pin/set", test_message_builder_basic_connect_pin_set);
    g_test_add_func ("/libmbim-glib/message/builder/basic-connect/connect/set/raw", test_message_builder_basic_connect_connect_set_raw);
    g_test_add_func ("/libmbim-glib/message/builder/basic-connect/connect/set", test_message_builder_basic_connect_connect_set);
    g_test_add_func ("/libmbim-glib/message/builder/basic-connect/service-activation/set", test_message_builder_basic_connect_service_activation_set);
    g_test_add_func ("/libmbim-glib/message/builder/basic-connect/device-service-subscriber-list/set", test_message_builder_basic_connect_device_service_subscriber_list_set);
    g_test_add_func ("/libmbim-glib/message/builder/ussd/set", test_message_builder_ussd_set);
    g_test_add_func ("/libmbim-glib/message/builder/auth/akap/query", test_message_builder_auth_akap_query);
    g_test_add_func ("/libmbim-glib/message/builder/stk/pac/set", test_message_builder_stk_pac_set);
    g_test_add_func ("/libmbim-glib/message/builder/stk/terminal-response/set", test_message_builder_stk_terminal_response_set);
    g_test_add_func ("/libmbim-glib/message/builder/stk/envelope/set", test_message_builder_stk_envelope_set);
    g_test_add_func ("/libmbim-glib/message/builder/basic-connect/ip-packet-filters/set/none", test_message_builder_basic_connect_ip_packet_filters_set_none);
    g_test_add_func ("/libmbim-glib/message/builder/basic-connect/ip-packet-filters/set/one", test_message_builder_basic_connect_ip_packet_filters_set_one);
    g_test_add_func ("/libmbim-glib/message/builder/basic-connect/ip-packet-filters/set/two", test_message_builder_basic_connect_ip_packet_filters_set_two);

    return g_test_run ();
}
