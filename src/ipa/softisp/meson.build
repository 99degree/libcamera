# SPDX-License-Identifier: CC0-1.0

# SoftISP IPA module - Minimal stub implementation

softisp_sources = files([
    'softisp.cpp',
    'softisp_module.cpp',
])

ipa_softisp = shared_module('ipa_softisp',
    softisp_sources,
    name_prefix : '',
    include_directories : [ipa_includes],
    dependencies : [libcamera_private, libipa_dep],
    install : true,
    install_dir : ipa_install_dir,
)
