# SPDX-License-Identifier: CC0-1.0
# SoftISP IPA module - ONNX-based implementation

# Find ONNX Runtime
onnxruntime_dep = dependency('onnxruntime', version: '>=1.14.0')

softisp_sources = files([
    'softisp.cpp',              # Skeleton (includes all SoftIsp_*.cpp)
    'SoftIsp_init.cpp',
    'SoftIsp_start.cpp',
    'SoftIsp_stop.cpp',
    'SoftIsp_configure.cpp',
    'SoftIsp_queueRequest.cpp',
    'SoftIsp_computeParams.cpp',
    'SoftIsp_processStats.cpp',
    'SoftIsp_processFrame.cpp',
    'SoftIsp_logPrefix.cpp',
    'onnx_engine.cpp',
    'softisp_module.cpp',
])

# Use the existing ipa_includes from the parent meson.build
# This should already include:
# - include/ (project root)
# - build/include/ (generated headers)
# - src/ipa/ (IPA base classes)
# - etc.

ipa_softisp = shared_module('ipa_softisp',
    softisp_sources,
    name_prefix : '',
    include_directories : [ipa_includes],  # Use parent's include dirs
    dependencies : [libcamera_private, libipa_dep, onnxruntime_dep],
    install : true,
    install_dir : ipa_install_dir,
)
