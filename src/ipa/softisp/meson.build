# SPDX-License-Identifier: CC0-1.0
# SoftISP IPA module - ONNX-based implementation

# Find ONNX Runtime
onnxruntime_dep = dependency('onnxruntime', version: '>=1.14.0')

# Source files:
# - softisp.cpp: Skeleton that includes all SoftIsp_*.cpp method files
# - softisp_module.cpp: Module entry point (ipaModuleInfo, ipaCreate)
softisp_sources = files([
    'softisp.cpp',           # Skeleton (includes all methods)
    'onnx_engine.cpp',       # ONNX engine implementation
    'softisp_module.cpp',    # Module entry point ONLY
])

# Build the IPA module
mod = shared_module('ipa_softisp',
    softisp_sources,
    name_prefix : '',
    include_directories : [ipa_includes],
    dependencies : [libcamera_private, libipa_dep, onnxruntime_dep],
    install : true,
    install_dir : ipa_install_dir,
    cpp_args : ['-Wno-error'],  # Disable -Werror for ONNX warnings
)
