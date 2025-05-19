# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2021 Iñigo Martinez <inigomartinez@gmail.com>

deps = [
  libmbim_common_dep,
  libmbim_glib_dep,
]

helpers_sources = files(
  'mbimcli-helpers.c',
  'mbimcli-helpers-quectel.c',
)

mbimcli_helpers = static_library(
  'mbimcli-helpers',
  sources: helpers_sources,
  include_directories: ['.', top_inc],
  dependencies: deps,
)

mbimcli_helpers_dep = declare_dependency(
  include_directories: ['.', top_inc],
  dependencies: deps,
  link_with: mbimcli_helpers,
)

mbimcli_sources = files(
  'mbimcli.c',
  'mbimcli-atds.c',
  'mbimcli-basic-connect.c',
  'mbimcli-compal.c',
  'mbimcli-dss.c',
  'mbimcli-fibocom.c',
  'mbimcli-google.c',
  'mbimcli-intel-firmware-update.c',
  'mbimcli-intel-mutual-authentication.c',
  'mbimcli-intel-thermal-rf.c',
  'mbimcli-intel-tools.c',
  'mbimcli-link-management.c',
  'mbimcli-ms-basic-connect-extensions.c',
  'mbimcli-ms-firmware-id.c',
  'mbimcli-ms-host-shutdown.c',
  'mbimcli-ms-sar.c',
  'mbimcli-ms-uicc-low-level-access.c',
  'mbimcli-ms-voice-extensions.c',
  'mbimcli-phonebook.c',
  'mbimcli-quectel.c',
  'mbimcli-sms.c',
)

sources = mbimcli_sources + helpers_sources + files(
  'mbimcli-link-management.c',
)

mbimcli = executable(
  'mbimcli',
  sources: sources,
  include_directories: top_inc,
  dependencies: deps,
  install: true,
)

if enable_bash_completion
  install_data(
    'mbimcli',
    install_dir: bash_completion_completionsdir,
  )
endif

subdir('tests')
