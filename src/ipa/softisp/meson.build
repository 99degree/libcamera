# SPDX-License-Identifier: CC0-1.0

# Common sources for both modules
softisp_common_sources = [
    'softisp.cpp',
]

# Module 1: For "softisp" pipeline (real cameras)
softisp_module_sources = softisp_common_sources + [
    'softisp_module.cpp',
]

ipa_name = 'ipa_softisp'
ipa_softisp = shared_module(ipa_name,
    softisp_module_sources,
    name_prefix : '',
    include_directories : [ipa_includes],
    dependencies : [libcamera_private, libipa_dep, onnxruntime_dep],
    install : true,
    install_dir : ipa_install_dir
)

# Module 2: For "virtual_softisp" pipeline (virtual cameras)
softisp_virtual_module_sources = softisp_common_sources + [
    'softisp_virtual_module.cpp',
]

ipa_name = 'ipa_softisp_virtual'
ipa_softisp_virtual = shared_module(ipa_name,
    softisp_virtual_module_sources,
    name_prefix : '',
    include_directories : [ipa_includes],
    dependencies : [libcamera_private, libipa_dep, onnxruntime_dep],
    install : true,
    install_dir : ipa_install_dir
)

# Add to list of IPA modules
ipa_names += 'ipa_softisp'
ipa_names += 'ipa_softisp_virtual'
