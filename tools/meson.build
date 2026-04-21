# SPDX-License-Identifier: CC0-1.0

softisp_test_app_sources = [
    'softisp-test-app.cpp',
]

softisp_test_app = executable('softisp-test-app',
    softisp_test_app_sources,
    include_directories : [
        libcamera_includes,
        libcamera_internal_includes,
    ],
    dependencies : [libcamera_private],
    install : true,
    install_dir : libcamera_bindir
)
