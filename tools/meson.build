executable('softisp-save',
  'softisp-save.cpp',
  include_directories: [
    include_directories('..'),
    include_directories('../src/libcamera'),
    include_directories('../src/libcamera/internal'),
  ],
  link_with: [
    libcamera,
  ],
  dependencies: [
    dependency('onnxruntime'),
    dependency('threads'),
  ],
  define_variable: { 'WITH_LIBCAMERA': 'true' },
  install: false
)
