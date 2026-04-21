# SPDX-License-Identifier: CC0-1.0

softisp_pipeline_sources = [
    'softisp.cpp',
]

softisp_pipeline = library('pipeline_softisp',
    softisp_pipeline_sources,
    name_prefix : '',
    include_directories : [
        libcamera_includes,
        libcamera_internal_includes,
    ],
    dependencies : [libcamera_private],
    install : true,
    install_dir : libcamera_libdir / 'pipeline'
)

# Register the pipeline with the global pipeline list
pipelines += 'softisp'
