# DummySoftISP IPA module (uses same code as softisp but different module name)
dummysoftisp_includes = include_directories('.')

# Use the same softisp.cpp source
dummysoftisp_module_sources = [
  '../softisp/softisp.cpp',
  'softisp_module.cpp',
]

ipa_name = 'ipa_dummysoftisp'
ipa_dummysoftisp = shared_module(
  ipa_name,
  dummysoftisp_module_sources,
  name_prefix: '',
  include_directories: [ipa_includes, dummysoftisp_includes],
  dependencies: [libcamera_private, libipa_dep, ],
  install: true,
  install_dir: ipa_install_dir,
)

# Add to list of IPA modules
ipa_names += ipa_name
