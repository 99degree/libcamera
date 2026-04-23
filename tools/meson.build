# SPDX-License-Identifier: CC0-1.0
# SoftISP tools

libcamera_includes = include_directories('../../include')
onnxruntime_dep = dependency('onnxruntime')

# Unified SoftISP tool suite (replaces 10 separate tools)
softisp_tool_sources = [
  'softisp-tool.cpp',
]

softisp_tool = executable('softisp-tool', softisp_tool_sources,
  include_directories : [ libcamera_includes, ],
  dependencies : [ onnxruntime_dep ],
  cpp_args : [ '-Wno-extra-semi', '-Wno-c++98-compat-extra-semi' ]
)

# SoftISP frame saver (pipeline test tool, similar to libcamera-cam)
softisp_save_sources = [
  'softisp-save.cpp',
]

softisp_save = executable('softisp-save', softisp_save_sources,
  include_directories : [ libcamera_includes, ],
  dependencies : []
)
