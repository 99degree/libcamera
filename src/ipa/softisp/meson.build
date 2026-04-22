# SPDX-License-Identifier: CC0-1.0

# Define Python3 executable
python3 = find_program('python3')

# --- Robust Path Definitions ---
# Use files() to get absolute paths relative to the project root
# files() automatically resolves paths from the project root
generator_script = files('../../../utils/codegen/ipc/generators/softisp_wrapper_generator.py')
mojom_file = files('../../../include/libcamera/ipa/softisp.mojom')

# --- Generate C++ Wrapper ---
wrapper_gen = custom_target('softisp_wrapper',
  input : mojom_file,
  output : 'softisp_wrapper.h',
  command : [
    python3,
    generator_script,
    '--mojom-file', '@INPUT@',
    '--output-dir', '@OUTDIR@',
  ],
  install : false,
)

# --- Build Configuration ---
softisp_includes = include_directories('.')

softisp_common_sources = [
  'softisp.cpp',
  wrapper_gen,
]

# Module 1: For "softisp" pipeline (real cameras)
softisp_module_sources = softisp_common_sources + [
  'softisp_module.cpp',
]

ipa_name = 'ipa_softisp'
ipa_softisp = shared_module(
  ipa_name,
  softisp_module_sources,
  name_prefix : '',
  include_directories : [ipa_includes, softisp_includes],
  dependencies : [libcamera_private, libipa_dep, onnxruntime_dep],
  install : true,
  install_dir : ipa_install_dir,
)

# Module 2: For "dummysoftisp" pipeline (virtual cameras)
softisp_virtual_module_sources = softisp_common_sources + [
  'softisp_virtual_module.cpp',
]

ipa_name = 'ipa_softisp_virtual'
ipa_softisp_virtual = shared_module(
  ipa_name,
  softisp_virtual_module_sources,
  name_prefix : '',
  include_directories : [ipa_includes, softisp_includes],
  dependencies : [libcamera_private, libipa_dep, onnxruntime_dep],
  install : true,
  install_dir : ipa_install_dir,
)

# Add to list of IPA modules
ipa_names += 'ipa_softisp'
ipa_names += 'ipa_softisp_virtual'
