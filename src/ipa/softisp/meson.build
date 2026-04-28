# SPDX-License-Identifier: CC0-1.0
# SoftISP IPA module

# Choose ONNX engine backend:
#   'dlopen' = runtime dlopen (default, no compile-time ORT dependency)
#   'ort'    = compile-time ORT C++ API (needs onnxruntime dep)
onnx_backend = 'ort'

softisp_cpp_args = []

if onnx_backend == 'ort'
    onnxruntime_dep = dependency('onnxruntime', version: '>=1.14.0')
    onnx_engine_sources = files('onnx_engine_ort.cpp')
    softisp_cpp_args += '-DUSE_ONNX_ORT'
else
    onnxruntime_dep = []
    onnx_engine_sources = files('onnx_engine_dlopen.cpp')
endif

softisp_sources = files([
    'softisp.cpp',
    'softisp_module.cpp',
]) + onnx_engine_sources

ipa_module = shared_module('ipa_softisp',
    softisp_sources,
    name_prefix : '',
    include_directories : [ipa_includes],
    dependencies : [libcamera_private, libipa_dep, onnxruntime_dep],
    install : true,
    install_dir : ipa_install_dir,
    cpp_args : softisp_cpp_args,
)

# ONNX Simple Test Tool (for debugging)
onnx_simple_test = executable('onnx_simple_test',
    ['onnx_simple_test.cpp'] + onnx_engine_sources,
    include_directories : [ipa_includes],
    dependencies : [libcamera_private, libipa_dep, onnxruntime_dep],
    cpp_args : softisp_cpp_args,
    install : false,
)
