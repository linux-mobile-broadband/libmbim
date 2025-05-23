# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

subdir('xml')

doc_module = 'libmbim-glib'

src_dirs = [
  generated_inc,
  libmbim_glib_inc,
]

private_headers = [
  'mbim-helpers.h',
  'mbim-helpers-netlink.h',
  'mbim-message-private.h',
  'mbim-net-port-manager.h',
  'mbim-net-port-manager-wdm.h',
  'mbim-net-port-manager-wwan.h',
  'wwan.h',
]

scan_args = [
  '--rebuild-types',
  '--deprecated-guards="MBIM_DISABLE_DEPRECATED"',
]

glib_prefix = glib_dep.get_pkgconfig_variable('prefix')

fixxref_args = [
  '--html-dir=' + (mbim_prefix / gnome.gtkdoc_html_dir(doc_module)),
  '--extra-dir=' + (glib_prefix / gnome.gtkdoc_html_dir('glib')),
  '--extra-dir=' + (glib_prefix / gnome.gtkdoc_html_dir('gio')),
]

name = doc_module + '-sections.txt'

'''
FIXME: gen_sections should be used in the `input` field but it would add
       a dependency over file paths that are not yet created, so
       the `depends` section is used to generate alternate dependency.
       This is fixed in newer meson versions.
'''
sections_txt = custom_target(
  name,
  input: 'libmbim-glib-common.sections',
  output: name,
  capture: true,
  command: [find_program('cat'), '@INPUT@'] + gen_sections,
  depends: gen_sections_deps,
  build_by_default: true,
)

version_xml = configure_file(
  input: 'version.xml.in',
  output: '@BASENAME@',
  configuration: version_conf,
)

gnome.gtkdoc(
  doc_module,
  main_xml: doc_module + '-docs.xml',
  src_dir: src_dirs,
  ignore_headers: private_headers,
  include_directories: top_inc,
  gobject_typesfile: doc_module + '.types',
  dependencies: libmbim_glib_dep,
  namespace: 'mbim',
  scan_args: scan_args,
  fixxref_args: fixxref_args,
  content_files: [sections_txt, version_xml],
  install: true,
  check: true,
)
