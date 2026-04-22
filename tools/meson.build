# SPDX-License-Identifier: CC0-1.0
softisp_test_app_sources = [
  'softisp-test-app.cpp',
]
softisp_test_app = executable('softisp-test-app',
  softisp_test_app_sources,
  include_directories : [
    libcamera_includes,
  ],
  dependencies : [libcamera_private]
)

# ONNX test app
softisp_onnx_test_sources = [
	'softisp-onnx-test.cpp',
]
softisp_onnx_test = executable('softisp-onnx-test', softisp_onnx_test_sources,
	include_directories : [ libcamera_includes, ],
	dependencies : [ libcamera_private, onnxruntime_dep ]
)
