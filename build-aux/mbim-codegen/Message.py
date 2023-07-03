# -*- Mode: python; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (C) 2013 - 2018 Aleksander Morgado <aleksander@aleksander.es>
#

import string
import utils


"""
Flag the values which always need to be read
"""
def flag_always_read_field(fields, field_name):
    for field in fields:
        if field['name'] == field_name:
            if field['format'] != 'guint32':
                    raise ValueError('Fields to always read \'%s\' must be a guint32' % field_name)
            field['always-read'] = True
            return
    raise ValueError('Couldn\'t find field to always read \'%s\'' % field_name)


"""
Validate fields in the dictionary
"""
def validate_fields(fields):
    for field in fields:
        # Look for condition fields, which need to be always read
        if 'available-if' in field:
            condition = field['available-if']
            flag_always_read_field(fields, condition['field'])

        # Look for array size fields, which need to be always read
        if field['format'] == 'byte-array':
            pass
        elif field['format'] == 'ref-byte-array':
            pass
        elif field['format'] == 'uicc-ref-byte-array':
            pass
        elif field['format'] == 'ref-byte-array-no-offset':
            pass
        elif field['format'] == 'unsized-byte-array':
            pass
        elif field['format'] == 'uuid':
            pass
        elif field['format'] == 'guint16':
            pass
        elif field['format'] == 'guint32':
            pass
        elif field['format'] == 'guint32-array':
            flag_always_read_field(fields, field['array-size-field'])
        elif field['format'] == 'guint64':
            pass
        elif field['format'] == 'string':
            pass
        elif field['format'] == 'string-array':
            flag_always_read_field(fields, field['array-size-field'])
        elif field['format'] == 'struct':
            if 'struct-type' not in field:
                raise ValueError('Field type \'struct\' requires \'struct-type\' field')
        elif field['format'] == 'ms-struct':
            if 'struct-type' not in field:
                raise ValueError('Field type \'ms-struct\' requires \'struct-type\' field')
        elif field['format'] == 'struct-array':
            flag_always_read_field(fields, field['array-size-field'])
            if 'struct-type' not in field:
                raise ValueError('Field type \'struct-array\' requires \'struct-type\' field')
        elif field['format'] == 'ref-struct-array':
            flag_always_read_field(fields, field['array-size-field'])
            if 'struct-type' not in field:
                raise ValueError('Field type \'ref-struct-array\' requires \'struct-type\' field')
        elif field['format'] == 'ms-struct-array':
            if 'struct-type' not in field:
                raise ValueError('Field type \'ms-struct-array\' requires \'struct-type\' field')
        elif field['format'] == 'ref-ipv4':
            pass
        elif field['format'] == 'ipv4-array':
            flag_always_read_field(fields, field['array-size-field'])
        elif field['format'] == 'ref-ipv6':
            pass
        elif field['format'] == 'ipv6-array':
            flag_always_read_field(fields, field['array-size-field'])
        elif field['format'] == 'tlv':
            pass
        elif field['format'] == 'tlv-string':
            pass
        elif field['format'] == 'tlv-guint16-array':
            pass
        elif field['format'] == 'tlv-list':
            pass
        else:
            raise ValueError('Cannot handle field type \'%s\'' % field['format'])


"""
The Message class takes care of all message handling
"""
class Message:

    """
    Constructor
    """
    def __init__(self, service, mbimex_service, mbimex_version, dictionary):
        # The message service, e.g. "Basic Connect"
        self.service = service
        self.mbimex_service = mbimex_service
        self.mbimex_version = mbimex_version

        # The name of the specific message, e.g. "Something"
        self.name = dictionary['name']

        # Query
        if 'query' in dictionary:
            self.has_query = True
            self.query = dictionary['query']
            self.query_since = dictionary['since-ex']['query'] if 'since-ex' in dictionary else dictionary['since'] if 'since' in dictionary else None
            if self.query_since is None:
                raise ValueError('Message ' + self.name + ' (query) requires a "since" tag specifying the major version where it was introduced')
            validate_fields(self.query)
        else:
            self.has_query = False
            self.query = []

        # Set
        if 'set' in dictionary:
            self.has_set = True
            self.set = dictionary['set']
            self.set_since = dictionary['since-ex']['set'] if 'since-ex' in dictionary else dictionary['since'] if 'since' in dictionary else None
            if self.set_since is None:
                raise ValueError('Message ' + self.name + ' (set) requires a "since" tag specifying the major version where it was introduced')
            validate_fields(self.set)
        else:
            self.has_set = False
            self.set = []


        # Response
        if 'response' in dictionary:
            self.has_response = True
            self.response = dictionary['response']
            self.response_since = dictionary['since-ex']['response'] if 'since-ex' in dictionary else dictionary['since'] if 'since' in dictionary else None
            if self.response_since is None:
                raise ValueError('Message ' + self.name + ' (response) requires a "since" tag specifying the major version where it was introduced')
            validate_fields(self.response)
        else:
            self.has_response = False
            self.response = []

        # Notification
        if 'notification' in dictionary:
            self.has_notification = True
            self.notification = dictionary['notification']
            self.notification_since = dictionary['since-ex']['notification'] if 'since-ex' in dictionary else dictionary['since'] if 'since' in dictionary else None
            if self.notification_since is None:
                raise ValueError('Message ' + self.name + ' (notification) requires a "since" tag specifying the major version where it was introduced')
            validate_fields(self.notification)
        else:
            self.has_notification = False
            self.notification = []

        # Build Fullname
        if self.service == 'Basic Connect':
            self.fullname = 'MBIM Message ' + self.name
        elif self.name == "":
            self.fullname = 'MBIM Message ' + self.service
        else:
            self.fullname = 'MBIM Message ' + self.service + ' ' + self.name

        # Build SERVICE enum
        if self.mbimex_service:
            self.service_enum_name = 'MBIM Service ' + self.mbimex_service
        else:
            self.service_enum_name = 'MBIM Service ' + self.service
        self.service_enum_name = utils.build_underscore_name(self.service_enum_name).upper()

        # Build CID enum
        if self.mbimex_service:
            self.cid_enum_name = 'MBIM CID ' + self.mbimex_service
        else:
            self.cid_enum_name = 'MBIM CID ' + self.service
        if self.name != "":
            self.cid_enum_name += (' ' + self.name)
        self.cid_enum_name = utils.build_underscore_name(self.cid_enum_name).upper()


    """
    Emit the message handling implementation
    """
    def emit(self, hfile, cfile):
        if self.has_query:
            utils.add_separator(hfile, 'Message (Query)', self.fullname);
            utils.add_separator(cfile, 'Message (Query)', self.fullname);
            self._emit_message_creator(hfile, cfile, 'query', self.query, self.query_since)
            self._emit_message_printable(cfile, 'query', self.query)

        if self.has_set:
            utils.add_separator(hfile, 'Message (Set)', self.fullname);
            utils.add_separator(cfile, 'Message (Set)', self.fullname);
            self._emit_message_creator(hfile, cfile, 'set', self.set, self.set_since)
            self._emit_message_printable(cfile, 'set', self.set)

        if self.has_response:
            utils.add_separator(hfile, 'Message (Response)', self.fullname);
            utils.add_separator(cfile, 'Message (Response)', self.fullname);
            self._emit_message_parser(hfile, cfile, 'response', self.response, self.response_since)
            self._emit_message_printable(cfile, 'response', self.response)

        if self.has_notification:
            utils.add_separator(hfile, 'Message (Notification)', self.fullname);
            utils.add_separator(cfile, 'Message (Notification)', self.fullname);
            self._emit_message_parser(hfile, cfile, 'notification', self.notification, self.notification_since)
            self._emit_message_printable(cfile, 'notification', self.notification)


    """
    Emit message creator
    """
    def _emit_message_creator(self, hfile, cfile, message_type, fields, since):
        translations = { 'message'            : self.name,
                         'service'            : self.service,
                         'since'              : since,
                         'underscore'         : utils.build_underscore_name (self.fullname),
                         'message_type'       : message_type,
                         'message_type_upper' : message_type.upper(),
                         'service_enum_name'  : self.service_enum_name,
                         'cid_enum_name'      : self.cid_enum_name }

        template = (
            '\n'
            '/**\n'
            ' * ${underscore}_${message_type}_new:\n')

        for field in fields:
            translations['name'] = field['name']
            translations['field'] = utils.build_underscore_name_from_camelcase (field['name'])
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']
            translations['array_size'] = field['array-size'] if 'array-size' in field else ''

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = (' * @${field}: (in)(element-type guint8)(array fixed-size=${array_size}): the \'${name}\' field, given as an array of ${array_size} #guint8 values.\n')
            elif field['format'] == 'unsized-byte-array' or \
                 field['format'] == 'ref-byte-array' or \
                 field['format'] == 'uicc-ref-byte-array' or \
                 field['format'] == 'ref-byte-array-no-offset':
                inner_template = (' * @${field}_size: (in): size of the ${field} array.\n'
                                  ' * @${field}: (in)(element-type guint8)(array length=${field}_size): the \'${name}\' field, given as an array of #guint8 values.\n')
            elif field['format'] == 'uuid':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #MbimUuid.\n')
            elif field['format'] == 'guint16':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #${public}.\n')
            elif field['format'] == 'guint32':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #${public}.\n')
            elif field['format'] == 'guint64':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #${public}.\n')
            elif field['format'] == 'string':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a string.\n')
            elif field['format'] == 'string-array':
                inner_template = (' * @${field}: (in)(type GStrv): the \'${name}\' field, given as an array of strings.\n')
            elif field['format'] == 'struct':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #${struct}.\n')
            elif field['format'] == 'ms-struct':
                raise ValueError('type \'ms-struct\' unsupported as input')
            elif field['format'] == 'struct-array':
                inner_template = (' * @${field}: (in)(array zero-terminated=1)(element-type ${struct}): the \'${name}\' field, given as an array of #${struct} items.\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = (' * @${field}: (in)(array zero-terminated=1)(element-type ${struct}): the \'${name}\' field, given as an array of #${struct} items.\n')
            elif field['format'] == 'ms-struct-array':
                raise ValueError('type \'ms-struct-array\' unsupported as input')
            elif field['format'] == 'ref-ipv4':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #MbimIPv4.\n')
            elif field['format'] == 'ipv4-array':
                inner_template = (' * @${field}: (in)(array zero-terminated=1)(element-type MbimIPv4): the \'${name}\' field, given as an array of #MbimIPv4 items.\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #MbimIPv6.\n')
            elif field['format'] == 'ipv6-array':
                inner_template = (' * @${field}: (in)(array zero-terminated=1)(element-type MbimIPv6): the \'${name}\' field, given as an array of #MbimIPv6 items.\n')
            elif field['format'] == 'tlv':
                inner_template = (' * @${field}: (in)(transfer none): the \'${name}\' field, given as a #${struct} item.\n')
            elif field['format'] == 'tlv-string':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a string.\n')
            elif field['format'] == 'tlv-guint16-array':
                raise ValueError('type \'tlv-guint16-array\' unsupported as input')
            elif field['format'] == 'tlv-list':
                inner_template = (' * @${field}: (in)(element-type MbimTlv)(transfer none): the \'${name}\' field, given as a list of #${struct} items.\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            ' * @error: return location for error or %NULL.\n'
            ' *\n'
            ' * Create a new request for the \'${message}\' ${message_type} command in the \'${service}\' service.\n'
            ' *\n'
            ' * Returns: a newly allocated #MbimMessage, which should be freed with mbim_message_unref().\n'
            ' *\n'
            ' * Since: ${since}\n'
            ' */\n'
            'MbimMessage *${underscore}_${message_type}_new (\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase (field['name'])
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = ('    const guint8 *${field},\n')
            elif field['format'] == 'unsized-byte-array' or \
                 field['format'] == 'ref-byte-array' or \
                 field['format'] == 'uicc-ref-byte-array' or \
                 field['format'] == 'ref-byte-array-no-offset':
                inner_template = ('    const guint32 ${field}_size,\n'
                                  '    const guint8 *${field},\n')
            elif field['format'] == 'uuid':
                inner_template = ('    const MbimUuid *${field},\n')
            elif field['format'] == 'guint16':
                inner_template = ('    ${public} ${field},\n')
            elif field['format'] == 'guint32':
                inner_template = ('    ${public} ${field},\n')
            elif field['format'] == 'guint64':
                inner_template = ('    ${public} ${field},\n')
            elif field['format'] == 'string':
                inner_template = ('    const gchar *${field},\n')
            elif field['format'] == 'string-array':
                inner_template = ('    const gchar *const *${field},\n')
            elif field['format'] == 'struct':
                inner_template = ('    const ${struct} *${field},\n')
            elif field['format'] == 'ms-struct':
                raise ValueError('type \'ms-struct\' unsupported as input')
            elif field['format'] == 'struct-array':
                inner_template = ('    const ${struct} *const *${field},\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = ('    const ${struct} *const *${field},\n')
            elif field['format'] == 'ms-struct-array':
                raise ValueError('type \'ms-struct-array\' unsupported as input')
            elif field['format'] == 'ref-ipv4':
                inner_template = ('    const MbimIPv4 *${field},\n')
            elif field['format'] == 'ipv4-array':
                inner_template = ('    const MbimIPv4 *${field},\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = ('    const MbimIPv6 *${field},\n')
            elif field['format'] == 'ipv6-array':
                inner_template = ('    const MbimIPv6 *${field},\n')
            elif field['format'] == 'tlv':
                inner_template = ('    const MbimTlv *${field},\n')
            elif field['format'] == 'tlv-string':
                inner_template = ('    const gchar *${field},\n')
            elif field['format'] == 'tlv-guint16-array':
                raise ValueError('type \'tlv-guint16-array\' unsupported as input')
            elif field['format'] == 'tlv-list':
                inner_template = ('    const GList *${field},\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error);\n')
        hfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            'MbimMessage *\n'
            '${underscore}_${message_type}_new (\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase (field['name'])
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']
            translations['array_size'] = field['array-size'] if 'array-size' in field else ''

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = ('    const guint8 *${field},\n')
            elif field['format'] == 'unsized-byte-array' or \
                 field['format'] == 'ref-byte-array' or \
                 field['format'] == 'uicc-ref-byte-array' or \
                 field['format'] == 'ref-byte-array-no-offset':
                inner_template = ('    const guint32 ${field}_size,\n'
                                  '    const guint8 *${field},\n')
            elif field['format'] == 'uuid':
                inner_template = ('    const MbimUuid *${field},\n')
            elif field['format'] == 'guint16':
                inner_template = ('    ${public} ${field},\n')
            elif field['format'] == 'guint32':
                inner_template = ('    ${public} ${field},\n')
            elif field['format'] == 'guint64':
                inner_template = ('    ${public} ${field},\n')
            elif field['format'] == 'string':
                inner_template = ('    const gchar *${field},\n')
            elif field['format'] == 'string-array':
                inner_template = ('    const gchar *const *${field},\n')
            elif field['format'] == 'struct':
                inner_template = ('    const ${struct} *${field},\n')
            elif field['format'] == 'ms-struct':
                raise ValueError('type \'ms-struct\' unsupported as input')
            elif field['format'] == 'struct-array':
                inner_template = ('    const ${struct} *const *${field},\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = ('    const ${struct} *const *${field},\n')
            elif field['format'] == 'ms-struct-array':
                raise ValueError('type \'ms-struct-array\' unsupported as input')
            elif field['format'] == 'ref-ipv4':
                inner_template = ('    const MbimIPv4 *${field},\n')
            elif field['format'] == 'ipv4-array':
                inner_template = ('    const MbimIPv4 *${field},\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = ('    const MbimIPv6 *${field},\n')
            elif field['format'] == 'ipv6-array':
                inner_template = ('    const MbimIPv6 *${field},\n')
            elif field['format'] == 'tlv':
                inner_template = ('    const MbimTlv *${field},\n')
            elif field['format'] == 'tlv-string':
                inner_template = ('    const gchar *${field},\n')
            elif field['format'] == 'tlv-guint16-array':
                raise ValueError('type \'tlv-guint16-array\' unsupported as input')
            elif field['format'] == 'tlv-list':
                inner_template = ('    const GList *${field},\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error)\n'
            '{\n'
            '    MbimMessageCommandBuilder *builder;\n'
            '\n'
            '    builder = _mbim_message_command_builder_new (0,\n'
            '                                                 ${service_enum_name},\n'
            '                                                 ${cid_enum_name},\n'
            '                                                 MBIM_MESSAGE_COMMAND_TYPE_${message_type_upper});\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['array_size_field'] = utils.build_underscore_name_from_camelcase(field['array-size-field']) if 'array-size-field' in field else ''
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            translations['struct_underscore'] = utils.build_underscore_name_from_camelcase (translations['struct'])
            translations['array_size'] = field['array-size'] if 'array-size' in field else ''
            translations['pad_array'] = field['pad-array'] if 'pad-array' in field else 'TRUE'

            inner_template = ''
            if 'available-if' in field:
                condition = field['available-if']
                translations['condition_field'] = utils.build_underscore_name_from_camelcase(condition['field'])
                translations['condition_operation'] = condition['operation']
                translations['condition_value'] = condition['value']
                inner_template += (
                    '    if (${condition_field} ${condition_operation} ${condition_value}) {\n')
            else:
                inner_template += ('    {\n')

            if field['format'] == 'byte-array':
                inner_template += ('        _mbim_message_command_builder_append_byte_array (builder, FALSE, FALSE, ${pad_array}, ${field}, ${array_size}, FALSE);\n')
            elif field['format'] == 'unsized-byte-array':
                inner_template += ('        _mbim_message_command_builder_append_byte_array (builder, FALSE, FALSE, ${pad_array}, ${field}, ${field}_size, FALSE);\n')
            elif field['format'] == 'ref-byte-array':
                inner_template += ('        _mbim_message_command_builder_append_byte_array (builder, TRUE, TRUE, ${pad_array}, ${field}, ${field}_size, FALSE);\n')
            elif field['format'] == 'uicc-ref-byte-array':
                inner_template += ('        _mbim_message_command_builder_append_byte_array (builder, TRUE, TRUE, ${pad_array}, ${field}, ${field}_size, TRUE);\n')
            elif field['format'] == 'ref-byte-array-no-offset':
                inner_template += ('        _mbim_message_command_builder_append_byte_array (builder, FALSE, TRUE, ${pad_array}, ${field}, ${field}_size, FALSE);\n')
            elif field['format'] == 'uuid':
                inner_template += ('        _mbim_message_command_builder_append_uuid (builder, ${field});\n')
            elif field['format'] == 'guint16':
                inner_template += ('        _mbim_message_command_builder_append_guint16 (builder, ${field});\n')
            elif field['format'] == 'guint32':
                inner_template += ('        _mbim_message_command_builder_append_guint32 (builder, ${field});\n')
            elif field['format'] == 'guint64':
                inner_template += ('        _mbim_message_command_builder_append_guint64 (builder, ${field});\n')
            elif field['format'] == 'string':
                inner_template += ('        _mbim_message_command_builder_append_string (builder, ${field});\n')
            elif field['format'] == 'string-array':
                inner_template += ('        _mbim_message_command_builder_append_string_array (builder, ${field}, ${array_size_field});\n')
            elif field['format'] == 'struct':
                inner_template += ('        _mbim_message_command_builder_append_${struct_underscore}_struct (builder, ${field});\n')
            elif field['format'] == 'ms-struct':
                raise ValueError('type \'ms-struct\' unsupported as input')
            elif field['format'] == 'struct-array':
                inner_template += ('        _mbim_message_command_builder_append_${struct_underscore}_struct_array (builder, ${field}, ${array_size_field});\n')
            elif field['format'] == 'ref-struct-array':
                inner_template += ('        _mbim_message_command_builder_append_${struct_underscore}_ref_struct_array (builder, ${field}, ${array_size_field});\n')
            elif field['format'] == 'ms-struct-array':
                raise ValueError('type \'ms-struct-array\' unsupported as input')
            elif field['format'] == 'ref-ipv4':
                inner_template += ('        _mbim_message_command_builder_append_ipv4 (builder, ${field}, TRUE);\n')
            elif field['format'] == 'ipv4-array':
                inner_template += ('        _mbim_message_command_builder_append_ipv4_array (builder, ${field}, ${array_size_field});\n')
            elif field['format'] == 'ref-ipv6':
                inner_template += ('        _mbim_message_command_builder_append_ipv6 (builder, ${field}, TRUE);\n')
            elif field['format'] == 'ipv6-array':
                inner_template += ('        _mbim_message_command_builder_append_ipv6_array (builder, ${field}, ${array_size_field});\n')
            elif field['format'] == 'tlv':
                inner_template += ('        _mbim_message_command_builder_append_tlv (builder, ${field});\n')
            elif field['format'] == 'tlv-string':
                inner_template += ('        _mbim_message_command_builder_append_tlv_string (builder, ${field});\n')
            elif field['format'] == 'tlv-guint16-array':
                raise ValueError('type \'tlv-guint16-array\' unsupported as input')
            elif field['format'] == 'tlv-list':
                inner_template += ('        _mbim_message_command_builder_append_tlv_list (builder, ${field});\n')

            else:
                raise ValueError('Cannot handle field type \'%s\'' % field['format'])

            inner_template += ('    }\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            '\n'
            '    return _mbim_message_command_builder_complete (builder);\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit message parser
    """
    def _emit_message_parser(self, hfile, cfile, message_type, fields, since):
        translations = { 'message'            : self.name,
                         'service'            : self.service,
                         'since'              : since,
                         'underscore'         : utils.build_underscore_name (self.fullname),
                         'message_type'       : message_type,
                         'message_type_upper' : message_type.upper() }

        template = (
            '\n'
            '/**\n'
            ' * ${underscore}_${message_type}_parse:\n'
            ' * @message: the #MbimMessage.\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['name'] = field['name']
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            translations['struct_underscore'] = utils.build_underscore_name_from_camelcase (translations['struct'])
            translations['array_size'] = field['array-size'] if 'array-size' in field else ''

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none)(element-type guint8)(array fixed-size=${array_size}): return location for an array of ${array_size} #guint8 values. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'unsized-byte-array' or field['format'] == 'ref-byte-array' or field['format'] == 'uicc-ref-byte-array':
                inner_template = (' * @out_${field}_size: (out)(optional): return location for the size of the ${field} array.\n'
                                  ' * @out_${field}: (out)(optional)(transfer none)(element-type guint8)(array length=out_${field}_size): return location for an array of #guint8 values. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'uuid':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #MbimUuid, or %NULL if the \'${name}\' field is not needed. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'guint16':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #${public}, or %NULL if the \'${name}\' field is not needed.\n')
            elif field['format'] == 'guint32':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #${public}, or %NULL if the \'${name}\' field is not needed.\n')
            elif field['format'] == 'guint64':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #guint64, or %NULL if the \'${name}\' field is not needed.\n')
            elif field['format'] == 'string':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full): return location for a newly allocated string, or %NULL if the \'${name}\' field is not needed. Free the returned value with g_free().\n')
            elif field['format'] == 'string-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full)(type GStrv): return location for a newly allocated array of strings, or %NULL if the \'${name}\' field is not needed. Free the returned value with g_strfreev().\n')
            elif field['format'] == 'struct':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full): return location for a newly allocated #${struct}, or %NULL if the \'${name}\' field is not needed. Free the returned value with ${struct_underscore}_free().\n')
            elif field['format'] == 'ms-struct':
                inner_template = (' * @out_${field}: (out)(optional)(nullable)(transfer full): return location for a newly allocated #${struct}, or %NULL if the \'${name}\' field is not needed. The availability of this field is not always guaranteed, and therefore %NULL may be given as a valid output. Free the returned value with ${struct_underscore}_free().\n')
            elif field['format'] == 'struct-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full)(array zero-terminated=1)(element-type ${struct}): return location for a newly allocated array of #${struct} items, or %NULL if the \'${name}\' field is not needed. Free the returned value with ${struct_underscore}_array_free().\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full)(array zero-terminated=1)(element-type ${struct}): return location for a newly allocated array of #${struct} items, or %NULL if the \'${name}\' field is not needed. Free the returned value with ${struct_underscore}_array_free().\n')
            elif field['format'] == 'ms-struct-array':
                inner_template = (' * @out_${field}_count: (out)(optional)(transfer none): return location for a #guint32, or %NULL if the field is not needed.\n'
                                  ' * @out_${field}: (out)(optional)(nullable)(transfer full)(array zero-terminated=1)(element-type ${struct}): return location for a newly allocated array of #${struct} items, or %NULL if the \'${name}\' field is not needed. The availability of this field is not always guaranteed, and therefore %NULL may be given as a valid output. Free the returned value with ${struct_underscore}_array_free().\n')
            elif field['format'] == 'ref-ipv4':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #MbimIPv4, or %NULL if the \'${name}\' field is not needed. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'ipv4-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full)(array zero-terminated=1)(element-type MbimIPv4): return location for a newly allocated array of #MbimIPv4 items, or %NULL if the \'${name}\' field is not needed. Free the returned value with g_free().\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #MbimIPv6, or %NULL if the \'${name}\' field is not needed. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'ipv6-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full)(array zero-terminated=1)(element-type MbimIPv6): return location for a newly allocated array of #MbimIPv6 items, or %NULL if the \'${name}\' field is not needed. Free the returned value with g_free().\n')
            elif field['format'] == 'tlv':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full): return location for a newly allocated #MbimTlv, or %NULL if the \'${name}\' field is not needed. Free the returned value with mbim_tlv_unref().\n')
            elif field['format'] == 'tlv-string':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full): return location for a newly allocated string, or %NULL if the \'${name}\' field is not needed. Free the returned value with g_free().\n')
            elif field['format'] == 'tlv-guint16-array':
                inner_template = (' * @out_${field}_count: (out)(optional)(transfer none): return location for a #guint32, or %NULL if the field is not needed.\n'
                                  ' * @out_${field}: (out)(optional)(nullable)(transfer full): return location for a newly allocated array of #guint16 items, or %NULL if the \'${name}\' field is not needed. The availability of this field is not always guaranteed, and therefore %NULL may be given as a valid output. Free the returned value with g_free().\n')
            elif field['format'] == 'tlv-list':
                inner_template = (' * @out_${field}: (out)(optional)(element-type MbimTlv)(transfer full): return location for a newly allocated list of #MbimTlv items, or %NULL if the \'${name}\' field is not needed. Free the returned value with g_list_free_full() using mbim_tlv_unref() as #GDestroyNotify.\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            ' * @error: return location for error or %NULL.\n'
            ' *\n'
            ' * Parses and returns parameters of the \'${message}\' ${message_type} command in the \'${service}\' service.\n'
            ' *\n'
            ' * Returns: %TRUE if the message was correctly parsed, %FALSE if @error is set.\n'
            ' *\n'
            ' * Since: ${since}\n'
            ' */\n'
            'gboolean ${underscore}_${message_type}_parse (\n'
            '    const MbimMessage *message,\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = ('    const guint8 **out_${field},\n')
            elif field['format'] == 'unsized-byte-array' or field['format'] == 'ref-byte-array' or field['format'] == 'uicc-ref-byte-array':
                inner_template = ('    guint32 *out_${field}_size,\n'
                                  '    const guint8 **out_${field},\n')
            elif field['format'] == 'uuid':
                inner_template = ('    const MbimUuid **out_${field},\n')
            elif field['format'] == 'guint16':
                inner_template = ('    ${public} *out_${field},\n')
            elif field['format'] == 'guint32':
                inner_template = ('    ${public} *out_${field},\n')
            elif field['format'] == 'guint64':
                inner_template = ('    ${public} *out_${field},\n')
            elif field['format'] == 'string':
                inner_template = ('    gchar **out_${field},\n')
            elif field['format'] == 'string-array':
                inner_template = ('    gchar ***out_${field},\n')
            elif field['format'] == 'struct':
                inner_template = ('    ${struct} **out_${field},\n')
            elif field['format'] == 'ms-struct':
                inner_template = ('    ${struct} **out_${field},\n')
            elif field['format'] == 'struct-array':
                inner_template = ('    ${struct}Array **out_${field},\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = ('    ${struct}Array **out_${field},\n')
            elif field['format'] == 'ms-struct-array':
                inner_template = ('    guint32 *out_${field}_count,\n'
                                  '    ${struct}Array **out_${field},\n')
            elif field['format'] == 'ref-ipv4':
                inner_template = ('    const MbimIPv4 **out_${field},\n')
            elif field['format'] == 'ipv4-array':
                inner_template = ('    MbimIPv4 **out_${field},\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = ('    const MbimIPv6 **out_${field},\n')
            elif field['format'] == 'ipv6-array':
                inner_template = ('    MbimIPv6 **out_${field},\n')
            elif field['format'] == 'tlv':
                inner_template = ('    MbimTlv **out_${field},\n')
            elif field['format'] == 'tlv-string':
                inner_template = ('    gchar **out_${field},\n')
            elif field['format'] == 'tlv-guint16-array':
                inner_template = ('    guint32 *out_${field}_count,\n'
                                  '    guint16 **out_${field},\n')
            elif field['format'] == 'tlv-list':
                inner_template = ('    GList **out_${field},\n')
            else:
                raise ValueError('Cannot handle field type \'%s\'' % field['format'])

            template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error);\n')
        hfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            'gboolean\n'
            '${underscore}_${message_type}_parse (\n'
            '    const MbimMessage *message,\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = ('    const guint8 **out_${field},\n')
            elif field['format'] == 'unsized-byte-array' or field['format'] == 'ref-byte-array' or field['format'] == 'uicc-ref-byte-array':
                inner_template = ('    guint32 *out_${field}_size,\n'
                                  '    const guint8 **out_${field},\n')
            elif field['format'] == 'uuid':
                inner_template = ('    const MbimUuid **out_${field},\n')
            elif field['format'] == 'guint16':
                inner_template = ('    ${public} *out_${field},\n')
            elif field['format'] == 'guint32':
                inner_template = ('    ${public} *out_${field},\n')
            elif field['format'] == 'guint64':
                inner_template = ('    ${public} *out_${field},\n')
            elif field['format'] == 'string':
                inner_template = ('    gchar **out_${field},\n')
            elif field['format'] == 'string-array':
                inner_template = ('    gchar ***out_${field},\n')
            elif field['format'] == 'struct':
                inner_template = ('    ${struct} **out_${field},\n')
            elif field['format'] == 'ms-struct':
                inner_template = ('    ${struct} **out_${field},\n')
            elif field['format'] == 'struct-array':
                inner_template = ('    ${struct}Array **out_${field},\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = ('    ${struct}Array **out_${field},\n')
            elif field['format'] == 'ms-struct-array':
                inner_template = ('    guint32 *out_${field}_count,\n'
                                  '    ${struct}Array **out_${field},\n')
            elif field['format'] == 'ref-ipv4':
                inner_template = ('    const MbimIPv4 **out_${field},\n')
            elif field['format'] == 'ipv4-array':
                inner_template = ('    MbimIPv4 **out_${field},\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = ('    const MbimIPv6 **out_${field},\n')
            elif field['format'] == 'ipv6-array':
                inner_template = ('    MbimIPv6 **out_${field},\n')
            elif field['format'] == 'tlv':
                inner_template = ('    MbimTlv **out_${field},\n')
            elif field['format'] == 'tlv-string':
                inner_template = ('    gchar **out_${field},\n')
            elif field['format'] == 'tlv-guint16-array':
                inner_template = ('    guint32 *out_${field}_count,\n'
                                  '    guint16 **out_${field},\n')
            elif field['format'] == 'tlv-list':
                inner_template = ('    GList **out_${field},\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error)\n'
            '{\n')

        if fields != []:
            template += (
                '    gboolean success = FALSE;\n'
                '    guint32 offset = 0;\n')

        count_allocated_variables = 0
        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            # variables to always read
            inner_template = ''
            if 'always-read' in field:
                inner_template = ('    guint32 _${field};\n')
            # now variables that require memory allocation
            elif field['format'] == 'string':
                count_allocated_variables += 1
                inner_template = ('    gchar *_${field} = NULL;\n')
            elif field['format'] == 'string-array':
                count_allocated_variables += 1
                inner_template = ('    gchar **_${field} = NULL;\n')
            elif field['format'] == 'struct':
                count_allocated_variables += 1
                inner_template = ('    ${struct} *_${field} = NULL;\n')
            elif field['format'] == 'ms-struct':
                count_allocated_variables += 1
                inner_template = ('    ${struct} *_${field} = NULL;\n')
            elif field['format'] == 'struct-array':
                count_allocated_variables += 1
                inner_template = ('    ${struct} **_${field} = NULL;\n')
            elif field['format'] == 'ref-struct-array':
                count_allocated_variables += 1
                inner_template = ('    ${struct} **_${field} = NULL;\n')
            elif field['format'] == 'ms-struct-array':
                count_allocated_variables += 1
                inner_template = ('    ${struct} **_${field} = NULL;\n')
            elif field['format'] == 'ipv4-array':
                count_allocated_variables += 1
                inner_template = ('    MbimIPv4 *_${field} = NULL;\n')
            elif field['format'] == 'ipv6-array':
                count_allocated_variables += 1
                inner_template = ('    MbimIPv6 *_${field} = NULL;\n')
            elif field['format'] == 'tlv':
                count_allocated_variables += 1
                inner_template = ('    MbimTlv *_${field} = NULL;\n')
            elif field['format'] == 'tlv-string':
                count_allocated_variables += 1
                inner_template = ('    gchar *_${field} = NULL;\n')
            elif field['format'] == 'tlv-guint16-array':
                count_allocated_variables += 1
                inner_template = ('    guint16 *_${field} = NULL;\n')
            elif field['format'] == 'tlv-list':
                count_allocated_variables += 1
                inner_template = ('    GList *_${field} = NULL;\n')
            template += (string.Template(inner_template).substitute(translations))

        if message_type == 'response':
            template += (
                '\n'
                '    if (mbim_message_get_message_type (message) != MBIM_MESSAGE_TYPE_COMMAND_DONE) {\n'
                '        g_set_error (error,\n'
                '                     MBIM_CORE_ERROR,\n'
                '                     MBIM_CORE_ERROR_INVALID_MESSAGE,\n'
                '                     \"Message is not a response\");\n'
                '        return FALSE;\n'
                '    }\n')

            if fields != []:
                template += (
                    '\n'
                    '    if (!mbim_message_command_done_get_raw_information_buffer (message, NULL)) {\n'
                    '        g_set_error (error,\n'
                    '                     MBIM_CORE_ERROR,\n'
                    '                     MBIM_CORE_ERROR_INVALID_MESSAGE,\n'
                    '                     \"Message does not have information buffer\");\n'
                    '        return FALSE;\n'
                    '    }\n')
        elif message_type == 'notification':
            template += (
                '\n'
                '    if (mbim_message_get_message_type (message) != MBIM_MESSAGE_TYPE_INDICATE_STATUS) {\n'
                '        g_set_error (error,\n'
                '                     MBIM_CORE_ERROR,\n'
                '                     MBIM_CORE_ERROR_INVALID_MESSAGE,\n'
                '                     \"Message is not a notification\");\n'
                '        return FALSE;\n'
                '    }\n')

            if fields != []:
                template += (
                    '\n'
                    '    if (!mbim_message_indicate_status_get_raw_information_buffer (message, NULL)) {\n'
                    '        g_set_error (error,\n'
                    '                     MBIM_CORE_ERROR,\n'
                    '                     MBIM_CORE_ERROR_INVALID_MESSAGE,\n'
                    '                     \"Message does not have information buffer\");\n'
                    '        return FALSE;\n'
                    '    }\n')
        else:
            raise ValueError('Unexpected message type \'%s\'' % message_type)

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['field_format_underscore'] = utils.build_underscore_name_from_camelcase(field['format'])
            translations['field_name'] = field['name']
            translations['array_size_field'] = utils.build_underscore_name_from_camelcase(field['array-size-field']) if 'array-size-field' in field else ''
            translations['struct_name'] = utils.build_underscore_name_from_camelcase(field['struct-type']) if 'struct-type' in field else ''
            translations['struct_type'] = field['struct-type'] if 'struct-type' in field else ''
            translations['array_size'] = field['array-size'] if 'array-size' in field else ''

            inner_template = (
                '\n'
                '    /* Read the \'${field_name}\' variable */\n')
            if 'available-if' in field:
                condition = field['available-if']
                translations['condition_field'] = utils.build_underscore_name_from_camelcase(condition['field'])
                translations['condition_operation'] = condition['operation']
                translations['condition_value'] = condition['value']
                inner_template += (
                    '    if (!(_${condition_field} ${condition_operation} ${condition_value})) {\n')
                if field['format'] == 'byte-array':
                    inner_template += (
                        '        if (out_${field})\n'
                        '            *out_${field} = NULL;\n')
                elif field['format'] == 'unsized-byte-array' or \
                   field['format'] == 'ref-byte-array' or \
                   field['format'] == 'uicc-ref-byte-array':
                    inner_template += (
                        '        if (out_${field}_size)\n'
                        '            *out_${field}_size = 0;\n'
                        '        if (out_${field})\n'
                        '            *out_${field} = NULL;\n')
                elif field['format'] == 'string' or \
                     field['format'] == 'string-array' or \
                     field['format'] == 'struct' or \
                     field['format'] == 'ms-struct' or \
                     field['format'] == 'struct-array' or \
                     field['format'] == 'ref-struct-array' or \
                     field['format'] == 'ref-ipv4' or \
                     field['format'] == 'ipv4-array' or \
                     field['format'] == 'ref-ipv6' or \
                     field['format'] == 'ipv6-array':
                    inner_template += (
                        '        if (out_${field} != NULL)\n'
                        '            *out_${field} = NULL;\n')
                elif field['format'] == 'ms-struct-array':
                    inner_template += (
                        '        if (out_${field}_count != NULL)\n'
                        '            *out_${field}_count = 0;\n'
                        '        if (out_${field} != NULL)\n'
                        '            *out_${field} = NULL;\n')
                else:
                    raise ValueError('Field format \'%s\' unsupported as optional field' % field['format'])

                inner_template += (
                    '    } else {\n')
            else:
                inner_template += (
                    '    {\n')

            if 'always-read' in field:
                inner_template += (
                    '        if (!_mbim_message_read_guint32 (message, offset, &_${field}, error))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            *out_${field} = _${field};\n'
                    '        offset += 4;\n')
            elif field['format'] == 'byte-array':
                inner_template += (
                    '        const guint8 *tmp;\n'
                    '\n'
                    '        if (!_mbim_message_read_byte_array (message, 0, offset, FALSE, FALSE, ${array_size}, &tmp, NULL, error, FALSE))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            *out_${field} = tmp;\n'
                    '        offset += ${array_size};\n')
            elif field['format'] == 'unsized-byte-array':
                inner_template += (
                    '        const guint8 *tmp;\n'
                    '        guint32 tmpsize;\n'
                    '\n'
                    '        if (!_mbim_message_read_byte_array (message, 0, offset, FALSE, FALSE, 0, &tmp, &tmpsize, error, FALSE))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            *out_${field} = tmp;\n'
                    '        if (out_${field}_size != NULL)\n'
                    '            *out_${field}_size = tmpsize;\n'
                    '        offset += tmpsize;\n')
            elif field['format'] == 'ref-byte-array':
                inner_template += (
                    '        const guint8 *tmp;\n'
                    '        guint32 tmpsize;\n'
                    '\n'
                    '        if (!_mbim_message_read_byte_array (message, 0, offset, TRUE, TRUE, 0, &tmp, &tmpsize, error, FALSE))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            *out_${field} = tmp;\n'
                    '        if (out_${field}_size != NULL)\n'
                    '            *out_${field}_size = tmpsize;\n'
                    '        offset += 8;\n')
            elif field['format'] == 'uicc-ref-byte-array':
                inner_template += (
                    '        const guint8 *tmp;\n'
                    '        guint32 tmpsize;\n'
                    '\n'
                    '        if (!_mbim_message_read_byte_array (message, 0, offset, TRUE, TRUE, 0, &tmp, &tmpsize, error, TRUE))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            *out_${field} = tmp;\n'
                    '        if (out_${field}_size != NULL)\n'
                    '            *out_${field}_size = tmpsize;\n'
                    '        offset += 8;\n')
            elif field['format'] == 'uuid':
                # NOTE: The output MbimUuid address would be broken if the contents of the message are misaligned.
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_uuid (message, offset, out_${field}, NULL, error))\n'
                    '            goto out;\n'
                    '        offset += 16;\n')
            elif field['format'] == 'guint16':
                if 'public-format' in field:
                    translations['public'] = field['public-format'] if 'public-format' in field else field['format']
                    inner_template += (
                        '        if (out_${field} != NULL) {\n'
                        '            guint16 aux;\n'
                        '\n'
                        '            if (!_mbim_message_read_guint16 (message, offset, &aux, error))\n'
                        '                goto out;\n'
                        '            *out_${field} = (${public})aux;\n'
                        '        }\n')
                else:
                    inner_template += (
                        '        if ((out_${field} != NULL) && !_mbim_message_read_guint16 (message, offset, out_${field}, error))\n'
                        '            goto out;\n')
                inner_template += (
                    '        offset += 2;\n')
            elif field['format'] == 'guint32':
                if 'public-format' in field:
                    translations['public'] = field['public-format'] if 'public-format' in field else field['format']
                    inner_template += (
                        '        if (out_${field} != NULL) {\n'
                        '            guint32 aux;\n'
                        '\n'
                        '            if (!_mbim_message_read_guint32 (message, offset, &aux, error))\n'
                        '                goto out;\n'
                        '            *out_${field} = (${public})aux;\n'
                        '        }\n')
                else:
                    inner_template += (
                        '        if ((out_${field} != NULL) && !_mbim_message_read_guint32 (message, offset, out_${field}, error))\n'
                        '            goto out;\n')
                inner_template += (
                    '        offset += 4;\n')
            elif field['format'] == 'guint64':
                if 'public-format' in field:
                    translations['public'] = field['public-format'] if 'public-format' in field else field['format']
                    inner_template += (
                        '        if (out_${field} != NULL) {\n'
                        '            guint64 aux;\n'
                        '\n'
                        '            if (!_mbim_message_read_guint64 (message, offset, &aux, error))\n'
                        '                goto out;\n'
                        '            *out_${field} = (${public})aux;\n'
                        '        }\n')
                else:
                    inner_template += (
                        '        if ((out_${field} != NULL) && !_mbim_message_read_guint64 (message, offset, out_${field}, error))\n'
                        '            goto out;\n')
                inner_template += (
                    '        offset += 8;\n')
            elif field['format'] == 'string':
                translations['encoding'] = 'MBIM_STRING_ENCODING_UTF8' if 'encoding' in field and field['encoding'] == 'utf-8' else 'MBIM_STRING_ENCODING_UTF16'
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_string (message, 0, offset, ${encoding}, &_${field}, NULL, error))\n'
                    '            goto out;\n'
                    '        offset += 8;\n')
            elif field['format'] == 'string-array':
                translations['encoding'] = 'MBIM_STRING_ENCODING_UTF8' if 'encoding' in field and field['encoding'] == 'utf-8' else 'MBIM_STRING_ENCODING_UTF16'
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_string_array (message, _${array_size_field}, 0, offset, ${encoding}, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += (8 * _${array_size_field});\n')
            elif field['format'] == 'struct':
                inner_template += (
                    '        ${struct_type} *tmp;\n'
                    '        guint32 bytes_read = 0;\n'
                    '\n'
                    '        tmp = _mbim_message_read_${struct_name}_struct (message, offset, &bytes_read, error);\n'
                    '        if (!tmp)\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            _${field} = tmp;\n'
                    '        else\n'
                    '             _${struct_name}_free (tmp);\n'
                    '        offset += bytes_read;\n')
            elif field['format'] == 'ms-struct':
                inner_template += (
                    '        ${struct_type} *tmp = NULL;\n'
                    '\n'
                    '        if (!_mbim_message_read_${struct_name}_ms_struct (message, offset, &tmp, error))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            _${field} = tmp;\n'
                    '        else\n'
                    '             _${struct_name}_free (tmp);\n'
                    '        offset += 8;\n')
            elif field['format'] == 'struct-array':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_${struct_name}_struct_array (message, _${array_size_field}, offset, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
            elif field['format'] == 'ref-struct-array':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_${struct_name}_ref_struct_array (message, _${array_size_field}, offset, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += (8 * _${array_size_field});\n')
            elif field['format'] == 'ms-struct-array':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_${struct_name}_ms_struct_array (message, offset, out_${field}_count, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 8;\n')
            elif field['format'] == 'ref-ipv4':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_ipv4 (message, offset, TRUE, out_${field}, NULL, error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
            elif field['format'] == 'ipv4-array':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_ipv4_array (message, _${array_size_field}, offset, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
            elif field['format'] == 'ref-ipv6':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_ipv6 (message, offset, TRUE, out_${field}, NULL, error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
            elif field['format'] == 'ipv6-array':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_ipv6_array (message, _${array_size_field}, offset, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
            elif field['format'] == 'tlv':
                inner_template += (
                    '        MbimTlv *tmp = NULL;\n'
                    '        guint32 bytes_read = 0;\n'
                    '\n'
                    '        if (!_mbim_message_read_tlv (message, offset, &tmp, &bytes_read, error))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            _${field} = tmp;\n'
                    '        else\n'
                    '             mbim_tlv_unref (tmp);\n'
                    '        offset += bytes_read;\n')
            elif field['format'] == 'tlv-string':
                inner_template += (
                    '        gchar *tmp = NULL;\n'
                    '        guint32 bytes_read = 0;\n'
                    '\n'
                    '        if (!_mbim_message_read_tlv_string (message, offset, &tmp, &bytes_read, error))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            _${field} = tmp;\n'
                    '        else\n'
                    '             g_free (tmp);\n'
                    '        offset += bytes_read;\n')
            elif field['format'] == 'tlv-guint16-array':
                inner_template += (
                    '        guint16 *tmp = NULL;\n'
                    '        guint32 bytes_read = 0;\n'
                    '\n'
                    '        if (!_mbim_message_read_tlv_guint16_array (message, offset, out_${field}_count, &tmp, &bytes_read, error))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            _${field} = tmp;\n'
                    '        else\n'
                    '             g_free (tmp);\n'
                    '        offset += bytes_read;\n')
            elif field['format'] == 'tlv-list':
                inner_template += (
                    '        GList *tmp = NULL;\n'
                    '        guint32 bytes_read = 0;\n'
                    '\n'
                    '        if (!_mbim_message_read_tlv_list (message, offset, &tmp, &bytes_read, error))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            _${field} = tmp;\n'
                    '        else\n'
                    '             g_list_free_full (tmp, (GDestroyNotify)mbim_tlv_unref);\n'
                    '        offset += bytes_read;\n')

            inner_template += (
                '    }\n')

            template += (string.Template(inner_template).substitute(translations))

        if fields != []:
            template += (
                '\n'
                '    /* All variables successfully parsed */\n'
                '    success = TRUE;\n'
                '\n'
                ' out:\n'
                '\n')

        if count_allocated_variables > 0:
            template += (
                '    if (success) {\n'
                '        /* Memory allocated variables as output */\n')
            for field in fields:
                translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
                # set as output memory allocated values
                inner_template = ''
                if field['format'] == 'string' or \
                   field['format'] == 'string-array' or \
                   field['format'] == 'struct' or \
                   field['format'] == 'ms-struct' or \
                   field['format'] == 'struct-array' or \
                   field['format'] == 'ref-struct-array' or \
                   field['format'] == 'ms-struct-array' or \
                   field['format'] == 'ipv4-array' or \
                   field['format'] == 'ipv6-array' or \
                   field['format'] == 'tlv' or \
                   field['format'] == 'tlv-string' or \
                   field['format'] == 'tlv-guint16-array' or \
                   field['format'] == 'tlv-list':
                    inner_template = ('        if (out_${field} != NULL)\n'
                                      '            *out_${field} = _${field};\n')
                    template += (string.Template(inner_template).substitute(translations))
            template += (
                '    } else {\n')
            for field in fields:
                translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
                translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
                translations['struct_underscore'] = utils.build_underscore_name_from_camelcase (translations['struct'])
                inner_template = ''
                if field['format'] == 'string' or \
                   field['format'] == 'ipv4-array' or \
                   field['format'] == 'ipv6-array' or \
                   field['format'] == 'tlv-string' or \
                   field['format'] == 'tlv-guint16-array':
                    inner_template = ('        g_free (_${field});\n')
                elif field['format'] == 'string-array':
                    inner_template = ('        g_strfreev (_${field});\n')
                elif field['format'] == 'struct' or field['format'] == 'ms-struct':
                    inner_template = ('        ${struct_underscore}_free (_${field});\n')
                elif field['format'] == 'struct-array' or field['format'] == 'ref-struct-array' or field['format'] == 'ms-struct-array':
                    inner_template = ('        ${struct_underscore}_array_free (_${field});\n')
                elif field['format'] == 'tlv':
                    inner_template = ('        if (_${field})\n'
                                      '            mbim_tlv_unref (_${field});\n')
                elif field['format'] == 'tlv-list':
                    inner_template = ('        g_list_free_full (_${field}, (GDestroyNotify)mbim_tlv_unref);\n')
                template += (string.Template(inner_template).substitute(translations))
            template += (
                '    }\n')

        if fields != []:
            template += (
                '\n'
                '    return success;\n'
                '}\n')
        else:
            template += (
                '\n'
                '    return TRUE;\n'
                '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit message printable
    """
    def _emit_message_printable(self, cfile, message_type, fields):
        translations = { 'message'            : self.name,
                         'underscore'         : utils.build_underscore_name(self.name),
                         'service'            : self.service,
                         'underscore'         : utils.build_underscore_name (self.fullname),
                         'message_type'       : message_type,
                         'message_type_upper' : message_type.upper() }
        template = (
            '\n'
            'static gchar *\n'
            '${underscore}_${message_type}_get_printable (\n'
            '    const MbimMessage *message,\n'
            '    const gchar *line_prefix,\n'
            '    GError **error)\n'
            '{\n'
            '    GString *str;\n')

        if fields != []:
            template += (
                '    GError *inner_error = NULL;\n'
                '    guint32 offset = 0;\n')

        for field in fields:
            if 'always-read' in field:
                translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
                inner_template = ('    guint32 _${field};\n')
                template += (string.Template(inner_template).substitute(translations))

        for field in fields:
            if 'personal-info' in field:
                template += (
                    '    gboolean show_field;\n'
                    '\n'
                    '    show_field = mbim_utils_get_show_personal_info ();\n')
                break

        if fields != []:
            if message_type == 'set' or message_type == 'query':
                template += (
                    '\n'
                    '    if (!mbim_message_command_get_raw_information_buffer (message, NULL))\n'
                    '        return NULL;\n')
            elif message_type == 'response':
                template += (
                    '\n'
                    '    if (!mbim_message_command_done_get_raw_information_buffer (message, NULL))\n'
                    '        return NULL;\n')
            elif message_type == 'notification':
                template += (
                    '\n'
                    '    if (!mbim_message_indicate_status_get_raw_information_buffer (message, NULL))\n'
                    '        return NULL;\n')

        template += (
            '\n'
            '    str = g_string_new ("");\n')

        for field in fields:
            translations['field']                   = utils.build_underscore_name_from_camelcase(field['name'])
            translations['field_format']            = field['format']
            translations['field_format_underscore'] = utils.build_underscore_name_from_camelcase(field['format'])
            translations['public']                  = field['public-format'] if 'public-format' in field else field['format']
            translations['field_name']              = field['name']
            translations['array_size_field']        = utils.build_underscore_name_from_camelcase(field['array-size-field']) if 'array-size-field' in field else ''
            translations['struct_name']             = utils.build_underscore_name_from_camelcase(field['struct-type']) if 'struct-type' in field else ''
            translations['struct_type']             = field['struct-type'] if 'struct-type' in field else ''
            translations['array_size']              = field['array-size'] if 'array-size' in field else ''

            if 'personal-info' in field:
                translations['if_show_field'] = 'if (show_field) '
            else:
                translations['if_show_field'] = ''

            inner_template = (
                '\n'
                '    g_string_append_printf (str, "%s  ${field_name} = ", line_prefix);\n')

            if 'available-if' in field:
                condition = field['available-if']
                translations['condition_field'] = utils.build_underscore_name_from_camelcase(condition['field'])
                translations['condition_operation'] = condition['operation']
                translations['condition_value'] = condition['value']
                inner_template += (
                    '    if (_${condition_field} ${condition_operation} ${condition_value}) {\n')
            else:
                inner_template += (
                    '    {\n')

            if 'always-read' in field:
                inner_template += (
                    '        if (!_mbim_message_read_guint32 (message, offset, &_${field}, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n'
                    '        ${if_show_field}{\n'
                    '            g_string_append_printf (str, "\'%" G_GUINT32_FORMAT "\'", _${field});\n'
                    '        }\n')

            elif field['format'] == 'byte-array' or \
                 field['format'] == 'unsized-byte-array' or \
                 field['format'] == 'ref-byte-array' or \
                 field['format'] == 'uicc-ref-byte-array' or \
                 field['format'] == 'ref-byte-array-no-offset':
                inner_template += (
                    '        guint i;\n'
                    '        const guint8 *tmp;\n'
                    '        guint32 tmpsize;\n'
                    '\n')
                if field['format'] == 'byte-array':
                    inner_template += (
                        '        if (!_mbim_message_read_byte_array (message, 0, offset, FALSE, FALSE, ${array_size}, &tmp, NULL, &inner_error, FALSE))\n'
                        '            goto out;\n'
                        '        tmpsize = ${array_size};\n'
                        '        offset += ${array_size};\n')
                elif field['format'] == 'unsized-byte-array':
                    inner_template += (
                        '        if (!_mbim_message_read_byte_array (message, 0, offset, FALSE, FALSE, 0, &tmp, &tmpsize, &inner_error, FALSE))\n'
                        '            goto out;\n'
                        '        offset += tmpsize;\n')

                elif field['format'] == 'ref-byte-array':
                    inner_template += (
                        '        if (!_mbim_message_read_byte_array (message, 0, offset, TRUE, TRUE, 0, &tmp, &tmpsize, &inner_error, FALSE))\n'
                        '            goto out;\n'
                        '        offset += 8;\n')

                elif field['format'] == 'uicc-ref-byte-array':
                    inner_template += (
                        '        if (!_mbim_message_read_byte_array (message, 0, offset, TRUE, TRUE, 0, &tmp, &tmpsize, &inner_error, TRUE))\n'
                        '            goto out;\n'
                        '        offset += 8;\n')

                elif field['format'] == 'ref-byte-array-no-offset':
                    inner_template += (
                        '        if (!_mbim_message_read_byte_array (message, 0, offset, FALSE, TRUE, 0, &tmp, &tmpsize, &inner_error, FALSE))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')

                inner_template += (
                    '        ${if_show_field}{\n'
                    '            g_string_append (str, "\'");\n'
                    '            for (i = 0; i  < tmpsize; i++)\n'
                    '                g_string_append_printf (str, "%02x%s", tmp[i], (i == (tmpsize - 1)) ? "" : ":" );\n'
                    '            g_string_append (str, "\'");\n'
                    '        }\n')

            elif field['format'] == 'uuid':
                inner_template += (
                    '        MbimUuid          tmp;\n'
                    '        g_autofree gchar *tmpstr = NULL;\n'
                    '\n'
                    '        if (!_mbim_message_read_uuid (message, offset, NULL, &tmp, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += 16;\n'
                    '        tmpstr = mbim_uuid_get_printable (&tmp);\n'
                    '        ${if_show_field}{\n'
                    '            g_string_append_printf (str, "\'%s\'", tmpstr);\n'
                    '        }\n')

            elif field['format'] == 'guint16' or \
                 field['format'] == 'guint32' or \
                 field['format'] == 'guint64':
                inner_template += (
                    '        ${field_format} tmp;\n'
                    '\n')
                if field['format'] == 'guint16' :
                    inner_template += (
                        '        if (!_mbim_message_read_guint16 (message, offset, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 2;\n')
                elif field['format'] == 'guint32' :
                    inner_template += (
                        '        if (!_mbim_message_read_guint32 (message, offset, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')
                elif field['format'] == 'guint64' :
                    inner_template += (
                        '        if (!_mbim_message_read_guint64 (message, offset, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 8;\n')

                if 'public-format' in field:
                    if field['public-format'] == 'gboolean':
                        inner_template += (
                            '        ${if_show_field}{\n'
                            '            g_string_append_printf (str, "\'%s\'", tmp ? "true" : "false");\n'
                            '        }\n'
                            '\n')
                    else:
                        translations['public_underscore']       = utils.build_underscore_name_from_camelcase(field['public-format'])
                        translations['public_underscore_upper'] = utils.build_underscore_name_from_camelcase(field['public-format']).upper()
                        inner_template += (
                            '        ${if_show_field}{\n'
                            '#if defined __${public_underscore_upper}_IS_ENUM__\n'
                            '            g_string_append_printf (str, "\'%s\'", ${public_underscore}_get_string ((${public})tmp));\n'
                            '#elif defined __${public_underscore_upper}_IS_FLAGS__\n'
                            '            g_autofree gchar *tmpstr = NULL;\n'
                            '\n'
                            '            tmpstr = ${public_underscore}_build_string_from_mask ((${public})tmp);\n'
                            '            g_string_append_printf (str, "\'%s\'", tmpstr);\n'
                            '#else\n'
                            '# error neither enum nor flags\n'
                            '#endif\n'
                            '        }\n'
                            '\n')

                elif field['format'] == 'guint16':
                    inner_template += (
                        '        ${if_show_field}{\n'
                        '            g_string_append_printf (str, "\'%" G_GUINT16_FORMAT "\'", tmp);\n'
                        '        }\n')
                elif field['format'] == 'guint32':
                    inner_template += (
                        '        ${if_show_field}{\n'
                        '            g_string_append_printf (str, "\'%" G_GUINT32_FORMAT "\'", tmp);\n'
                        '        }\n')
                elif field['format'] == 'guint64':
                    inner_template += (
                        '        ${if_show_field}{\n'
                        '            g_string_append_printf (str, "\'%" G_GUINT64_FORMAT "\'", tmp);\n'
                        '        }\n')

            elif field['format'] == 'string':
                translations['encoding'] = 'MBIM_STRING_ENCODING_UTF8' if 'encoding' in field and field['encoding'] == 'utf-8' else 'MBIM_STRING_ENCODING_UTF16'
                inner_template += (
                    '        g_autofree gchar *tmp = NULL;\n'
                    '\n'
                    '        if (!_mbim_message_read_string (message, 0, offset, ${encoding}, &tmp, NULL, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += 8;\n'
                    '        ${if_show_field}{\n'
                    '            g_string_append_printf (str, "\'%s\'", tmp);\n'
                    '        }\n')

            elif field['format'] == 'string-array':
                translations['encoding'] = 'MBIM_STRING_ENCODING_UTF8' if 'encoding' in field and field['encoding'] == 'utf-8' else 'MBIM_STRING_ENCODING_UTF16'
                inner_template += (
                    '        g_auto(GStrv) tmp = NULL;\n'
                    '        guint i;\n'
                    '\n'
                    '        if (!_mbim_message_read_string_array (message, _${array_size_field}, 0, offset, ${encoding}, &tmp, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += (8 * _${array_size_field});\n'
                    '\n'
                    '        ${if_show_field}{\n'
                    '            g_string_append (str, "\'");\n'
                    '            for (i = 0; i < _${array_size_field}; i++) {\n'
                    '                g_string_append (str, tmp[i]);\n'
                    '                if (i < (_${array_size_field} - 1))\n'
                    '                    g_string_append (str, ", ");\n'
                    '            }\n'
                    '            g_string_append (str, "\'");\n'
                    '        }\n')

            elif field['format'] == 'struct':
                inner_template += (
                    '        g_autoptr(${struct_type}) tmp = NULL;\n'
                    '        guint32 bytes_read = 0;\n'
                    '\n'
                    '        tmp = _mbim_message_read_${struct_name}_struct (message, offset, &bytes_read, &inner_error);\n'
                    '        if (!tmp)\n'
                    '            goto out;\n'
                    '        offset += bytes_read;\n'
                    '        ${if_show_field}{\n'
                    '            g_autofree gchar *new_line_prefix = NULL;\n'
                    '            g_autofree gchar *struct_str = NULL;\n'
                    '\n'
                    '            g_string_append (str, "{\\n");\n'
                    '            new_line_prefix = g_strdup_printf ("%s    ", line_prefix);\n'
                    '            struct_str = _mbim_message_print_${struct_name}_struct (tmp, new_line_prefix);\n'
                    '            g_string_append (str, struct_str);\n'
                    '            g_string_append_printf (str, "%s  }", line_prefix);\n'
                    '        }\n')

            elif field['format'] == 'ms-struct':
                inner_template += (
                    '        g_autoptr(${struct_type}) tmp = NULL;\n'
                    '\n'
                    '        if (!_mbim_message_read_${struct_name}_ms_struct (message, offset, &tmp, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += 8;\n'
                    '        ${if_show_field}{\n'
                    '            g_autofree gchar *new_line_prefix = NULL;\n'
                    '\n'
                    '            g_string_append (str, "{\\n");\n'
                    '            new_line_prefix = g_strdup_printf ("%s    ", line_prefix);\n'
                    '            if (tmp) {\n'
                    '                g_autofree gchar *struct_str = NULL;\n'
                    '                struct_str = _mbim_message_print_${struct_name}_struct (tmp, new_line_prefix);\n'
                    '                g_string_append (str, struct_str);\n'
                    '            }\n'
                    '            g_string_append_printf (str, "%s  }", line_prefix);\n'
                    '        }\n')

            elif field['format'] == 'struct-array' or field['format'] == 'ref-struct-array' or field['format'] == 'ms-struct-array':
                inner_template += (
                    '        g_autoptr(${struct_type}Array) tmp = NULL;\n')
                if field['format'] == 'ms-struct-array':
                    inner_template += (
                        '        guint32 tmp_count = 0;\n')
                inner_template += (
                    '\n')

                if field['format'] == 'struct-array':
                    inner_template += (
                    '        if (!_mbim_message_read_${struct_name}_struct_array (message, _${array_size_field}, offset, &tmp, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
                elif field['format'] == 'ref-struct-array':
                    inner_template += (
                    '        if (!_mbim_message_read_${struct_name}_ref_struct_array (message, _${array_size_field}, offset, &tmp, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += (8 * _${array_size_field});\n')
                elif field['format'] == 'ms-struct-array':
                    inner_template += (
                    '        if (!_mbim_message_read_${struct_name}_ms_struct_array (message, offset, &tmp_count, &tmp, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += 8;\n')

                inner_template += (
                    '        ${if_show_field}{\n'
                    '            guint i;\n'
                    '            g_autofree gchar *new_line_prefix = NULL;\n'
                    '\n'
                    '            new_line_prefix = g_strdup_printf ("%s        ", line_prefix);\n'
                    '            g_string_append (str, "\'{\\n");\n')

                if field['format'] == 'ms-struct-array':
                    inner_template += (
                        '            for (i = 0; i < tmp_count; i++) {\n')
                else:
                    inner_template += (
                        '            for (i = 0; i < _${array_size_field}; i++) {\n')

                inner_template += (
                    '                g_autofree gchar *struct_str = NULL;\n'
                    '\n'
                    '                g_string_append_printf (str, "%s    [%u] = {\\n", line_prefix, i);\n'
                    '                struct_str = _mbim_message_print_${struct_name}_struct (tmp[i], new_line_prefix);\n'
                    '                g_string_append (str, struct_str);\n'
                    '                g_string_append_printf (str, "%s    },\\n", line_prefix);\n'
                    '            }\n'
                    '            g_string_append_printf (str, "%s  }\'", line_prefix);\n'
                    '        }\n')

            elif field['format'] == 'ref-ipv4' or \
                 field['format'] == 'ipv4-array' or \
                 field['format'] == 'ref-ipv6' or \
                 field['format'] == 'ipv6-array':
                if field['format'] == 'ref-ipv4':
                    inner_template += (
                        '        const MbimIPv4 *tmp;\n')
                elif field['format'] == 'ipv4-array':
                    inner_template += (
                        '        g_autofree MbimIPv4 *tmp = NULL;\n')
                elif field['format'] == 'ref-ipv6':
                    inner_template += (
                        '        const MbimIPv6 *tmp;\n')
                elif field['format'] == 'ipv6-array':
                    inner_template += (
                        '        g_autofree MbimIPv6 *tmp = NULL;\n')

                inner_template += (
                    '        guint array_size;\n'
                    '        guint i;\n'
                    '\n')

                if field['format'] == 'ref-ipv4':
                    inner_template += (
                        '        array_size = 1;\n'
                        '        if (!_mbim_message_read_ipv4 (message, offset, TRUE, &tmp, NULL, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')
                elif field['format'] == 'ipv4-array':
                    inner_template += (
                        '        array_size = _${array_size_field};\n'
                        '        if (!_mbim_message_read_ipv4_array (message, _${array_size_field}, offset, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')
                elif field['format'] == 'ref-ipv6':
                    inner_template += (
                        '        array_size = 1;\n'
                        '        if (!_mbim_message_read_ipv6 (message, offset, TRUE, &tmp, NULL, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')
                elif field['format'] == 'ipv6-array':
                    inner_template += (
                        '        array_size = _${array_size_field};\n'
                        '        if (!_mbim_message_read_ipv6_array (message, _${array_size_field}, offset, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')

                inner_template += (
                    '        ${if_show_field}{\n'
                    '            g_string_append (str, "\'");\n'
                    '            if (tmp) {\n'
                    '                for (i = 0; i < array_size; i++) {\n'
                    '                    g_autoptr(GInetAddress)  addr = NULL;\n'
                    '                    g_autofree gchar        *tmpstr = NULL;\n'
                    '\n')

                if field['format'] == 'ref-ipv4' or \
                   field['format'] == 'ipv4-array':
                    inner_template += (
                        '                    addr = g_inet_address_new_from_bytes ((guint8 *)&(tmp[i].addr), G_SOCKET_FAMILY_IPV4);\n')
                elif field['format'] == 'ref-ipv6' or \
                     field['format'] == 'ipv6-array':
                    inner_template += (
                        '                    addr = g_inet_address_new_from_bytes ((guint8 *)&(tmp[i].addr), G_SOCKET_FAMILY_IPV6);\n')

                inner_template += (
                    '                    tmpstr = g_inet_address_to_string (addr);\n'
                    '                    g_string_append_printf (str, "%s", tmpstr);\n'
                    '                    if (i < (array_size - 1))\n'
                    '                        g_string_append (str, ", ");\n'
                    '                }\n'
                    '            }\n'
                    '            g_string_append (str, "\'");\n'
                    '        }\n')

            elif field['format'] == 'tlv' or \
                 field['format'] == 'tlv-string' or \
                 field['format'] == 'tlv-guint16-array':
                inner_template += (
                    '        g_autoptr(MbimTlv) tmp = NULL;\n'
                    '        guint32 bytes_read = 0;\n'
                    '\n'
                    '        if (!_mbim_message_read_tlv (message, offset, &tmp, &bytes_read, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += bytes_read;\n'
                    '\n'
                    '        ${if_show_field}{\n'
                    '            g_autofree gchar *tlv_str = NULL;\n'
                    '            g_autofree gchar *new_line_prefix = NULL;\n'
                    '\n'
                    '            new_line_prefix = g_strdup_printf ("%s  ", line_prefix);\n'
                    '            tlv_str = _mbim_tlv_print (tmp, new_line_prefix);\n'
                    '            g_string_append_printf (str, "\'%s\'", tlv_str);\n'
                    '        }\n')

            elif field['format'] == 'tlv-list':
                inner_template += (
                    '        GList *tmp = NULL;\n'
                    '        guint32 bytes_read = 0;\n'
                    '\n'
                    '        if (!_mbim_message_read_tlv_list (message, offset, &tmp, &bytes_read, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += bytes_read;\n'
                    '\n'
                    '        ${if_show_field}{\n'
                    '            g_autofree gchar *new_line_prefix = NULL;\n'
                    '            GList *walker = NULL;\n'
                    '\n'
                    '            new_line_prefix = g_strdup_printf ("%s    ", line_prefix);\n'
                    '            g_string_append (str, "\'[ ");\n'
                    '            for (walker = tmp; walker; walker = g_list_next (walker)) {\n'
                    '                g_autofree gchar *tlv_str = NULL;\n'
                    '\n'
                    '                tlv_str = _mbim_tlv_print ((MbimTlv *)walker->data, new_line_prefix);\n'
                    '                g_string_append_printf (str, "%s,", tlv_str);\n'
                    '            }\n'
                    '            g_string_append_printf (str, "\\n%s  ]\'", line_prefix);\n'
                    '        }\n'
                    '        g_list_free_full (tmp, (GDestroyNotify)mbim_tlv_unref);\n')

            else:
                raise ValueError('Field format \'%s\' not printable' % field['format'])

            if 'personal-info' in field:
                inner_template += (
                    '        if (!show_field)\n'
                    '           g_string_append (str, "\'###\'");\n')

            inner_template += (
                '    }\n'
                '    g_string_append (str, "\\n");\n')

            template += (string.Template(inner_template).substitute(translations))

        if fields != []:
            template += (
                '\n'
                ' out:\n'
                '    if (inner_error) {\n'
                '        g_string_append_printf (str, "n/a: %s", inner_error->message);\n'
                '        g_clear_error (&inner_error);\n'
                '    }\n')

        template += (
            '\n'
            '    return g_string_free (str, FALSE);\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit the section content
    """
    def emit_section_content(self, sfile):
        translations = { 'name_dashed' : utils.build_dashed_name(self.name),
                         'underscore'  : utils.build_underscore_name(self.fullname) }

        template = (
            '\n'
            '<SUBSECTION ${name_dashed}>\n')
        sfile.write(string.Template(template).substitute(translations))

        if self.has_query:
            template = (
                '${underscore}_query_new\n')
            sfile.write(string.Template(template).substitute(translations))

        if self.has_set:
            template = (
                '${underscore}_set_new\n')
            sfile.write(string.Template(template).substitute(translations))

        if self.has_response:
            template = (
                '${underscore}_response_parse\n')
            sfile.write(string.Template(template).substitute(translations))

        if self.has_notification:
            template = (
                '${underscore}_notification_parse\n')
            sfile.write(string.Template(template).substitute(translations))
