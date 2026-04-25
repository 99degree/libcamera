# SPDX-License-Identifier: CC0-1.0
# SoftISP IPA module - ONNX-based implementation

# Find ONNX Runtime
onnxruntime_dep = dependency('onnxruntime', version: '>=1.14.0')

softisp_sources = files([
  'softisp.cpp',          # Skeleton (includes all SoftIsp_*.cpp)
  'onnx_engine.cpp',      # Standalone class
  'softisp_module.cpp',   # Module entry point
])

# Use the exact same pattern as simple pipeline
mod = shared_module('ipa_softisp', softisp_sources,
  name_prefix : '',
  include_directories : [ipa_includes],
  dependencies : [libcamera_private, libipa_dep, onnxruntime_dep],
  install : true,
  install_dir : ipa_install_dir,
  # Disable -Werror for this module due to ONNX Runtime header warnings
  cpp_args : ['-Wno-error'],
)
