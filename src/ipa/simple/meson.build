# SPDX-License-Identifier: CC0-1.0

subdir('algorithms')
subdir('data')

ipa_name = 'ipa_soft_simple'

soft_simple_sources = files([
    'ipa_context.cpp',
    'soft_simple.cpp',
])

libplacebo = subproject('libplacebo')
libplacebo_dep = dependency('libplacebo', required : false)

#libplacebo = cc.find_library('libplacebo', required : false)

#if not libplacebo.found()
#    cmake = import('cmake')

#    libplacebo_vars = cmake.subproject_options()
#    libplacebo_vars.add_cmake_defines({'CMAKE_POSITION_INDEPENDENT_CODE': 'ON'})
#    libplacebo_vars.append_compile_args('c', '-Wno-unused-value')
#    libplacebo_wrap = cmake.subproject('libplacebo', options : libplacebo_vars)
#    libplacebo = libplacebo_wrap.dependency('libplacebo')
#    libplacebo = subproject('libplacebo')
#endif

#libplacebo_ipa_deps = [
#    libplacebo,
#]

soft_simple_sources += soft_simple_ipa_algorithms

mod = shared_module(ipa_name, soft_simple_sources,
                    name_prefix : '',
                    include_directories : [ipa_includes],
                    dependencies : [libcamera_private, libipa_dep, libplacebo_dep],
                    install : true,
                    install_dir : ipa_install_dir)

if ipa_sign_module
    custom_target(ipa_name + '.so.sign',
                  input : mod,
                  output : ipa_name + '.so.sign',
                  command : [ipa_sign, ipa_priv_key, '@INPUT@', '@OUTPUT@'],
                  install : false,
                  build_by_default : true)
endif

ipa_names += ipa_name
