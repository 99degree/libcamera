executable('softisp-test',
  'softisp-test.cpp',
  include_directories: [
    include_directories('..'),
    include_directories('../libcamera'),
    include_directories('../libcamera/internal'),
    include_directories('../ipa'),
  ],
  link_with: [
    libcamera,
    libcamera_ipa_softisp,
  ],
  dependencies: [
    dependency('threads'),
    dependency('libdrm'),
  ],
  install: false
)
