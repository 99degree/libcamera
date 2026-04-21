# SPDX-License-Identifier: CC0-1.0

ipa_name = 'ipa_softisp'

softisp_sources = files([
    'softisp.cpp',
    'softisp_module.cpp',
])

mod = shared_module(ipa_name,
    softisp_sources,
    name_prefix : '',
    include_directories : [ipa_includes],
    dependencies : [libcamera_private, libipa_dep, onnxruntime_dep],
    install : true,
    install_dir : ipa_install_dir
)

if ipa_sign_module
    custom_target(ipa_name + '.so.sign',
        input : mod,
        output : ipa_name + '.so.sign',
        command : [ipa_sign, ipa_priv_key, '@INPUT@', '@OUTPUT@'],
        install : false,
        build_by_default : true
    )
endif

# Build test programs if tests are enabled
if get_option('test')
    softisp_test_sources = files([
        'softisp_test.cpp',
    ])

    softisp_test = executable('softisp_test',
        softisp_test_sources,
        dependencies : [onnxruntime_dep],
        install : false,
        build_by_default : false
    )

    softisp_full_test_sources = files([
        'softisp_full_test.cpp',
    ])

    softisp_full_test = executable('softisp_full_test',
        softisp_full_test_sources,
        dependencies : [onnxruntime_dep],
        install : false,
        build_by_default : false
    )
endif

ipa_names += ipa_name
