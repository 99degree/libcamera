# SPDX-License-Identifier: CC0-1.0

# SoftISP IPA module - Standalone stub

softisp_sources = files([
    'softisp_stub.cpp',
])

ipa_softisp = shared_module('ipa_softisp',
    softisp_sources,
    name_prefix : '',
    include_directories : [ipa_includes],
    dependencies : [libcamera_private, libipa_dep],
    install : true,
    install_dir : ipa_install_dir,
)

# Virtual module
softisp_virtual_sources = files([
    'softisp_stub.cpp',
])

ipa_softisp_virtual = shared_module('ipa_softisp_virtual',
    softisp_virtual_sources,
    name_prefix : '',
    include_directories : [ipa_includes],
    dependencies : [libcamera_private, libipa_dep],
    install : true,
    install_dir : ipa_install_dir,
)
