# SPDX-License-Identifier: CC0-1.0

# Common sources for both modules
softisp_common_sources = [
    'softisp.cpp',
]

# Module 1: For "softisp" pipeline (real cameras)
softisp_module_sources = softisp_common_sources + [
    'softisp_module.cpp',
]

ipa_softisp = library('ipa_softisp',
    softisp_module_sources,
    name_prefix : '',
    include_directories : [
        libcamera_includes,
        libcamera_internal_includes,
    ],
    dependencies : [libcamera_private, onnxruntime_dep],
    install : true,
    install_dir : libcamera_libdir / 'ipa'
)

# Module 2: For "virtual-softisp" pipeline (virtual cameras)
softisp_virtual_module_sources = softisp_common_sources + [
    'softisp_virtual_module.cpp',
]

ipa_softisp_virtual = library('ipa_softisp_virtual',
    softisp_virtual_module_sources,
    name_prefix : '',
    include_directories : [
        libcamera_includes,
        libcamera_internal_includes,
    ],
    dependencies : [libcamera_private, onnxruntime_dep],
    install : true,
    install_dir : libcamera_libdir / 'ipa'
)
