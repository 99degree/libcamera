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

# Full inference test app
softisp_inference_test_sources = [
	'softisp-onnx-inference-test.cpp',
]
# Enhanced ONNX inspector with mapping
softisp_inspector_sources = [
	'softisp-onnx-inspector.cpp',
]
softisp_inspector = executable('softisp-onnx-inspector', softisp_inspector_sources,
	include_directories : [ libcamera_includes, ],
	dependencies : [ libcamera_private, onnxruntime_dep ]
)

softisp_inference_test = executable('softisp-onnx-inference-test', softisp_inference_test_sources,
	include_directories : [ libcamera_includes, ],
	dependencies : [ libcamera_private, onnxruntime_dep ]
)

shape_check = executable('softisp-onnx-shape-check', 'softisp-onnx-shape-check.cpp',
	include_directories : [ libcamera_includes, ],
	dependencies : [ libcamera_private, onnxruntime_dep ]
)

all_info = executable('softisp-onnx-all-info', 'softisp-onnx-all-info.cpp',
	include_directories : [ libcamera_includes, ],
	dependencies : [ libcamera_private, onnxruntime_dep ]
)

types_check = executable('softisp-onnx-types', 'softisp-onnx-types.cpp',
	include_directories : [ libcamera_includes, ],
	dependencies : [ libcamera_private, onnxruntime_dep ]
)

applier_debug = executable('softisp-applier-debug', 'softisp-applier-debug.cpp',
	include_directories : [ libcamera_includes, ],
	dependencies : [ libcamera_private, onnxruntime_dep ]
)

applier_run = executable('softisp-applier-run', 'softisp-applier-run.cpp',
	include_directories : [ libcamera_includes, ],
	dependencies : [ libcamera_private, onnxruntime_dep ]
)

check_coeffs = executable('softisp-check-coeffs', 'softisp-check-coeffs.cpp',
	include_directories : [ libcamera_includes, ],
	dependencies : [ libcamera_private, onnxruntime_dep ]
)
