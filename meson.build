project('clod', 'c', version: '1.0.0', default_options : ['c_std=c2x'])

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

# Detect the host system
host_os = host_machine.system()

if host_os == 'linux'
    add_project_arguments('-D_GNU_SOURCE', language: 'c')
    add_project_arguments('-DCLOD_USE_POSIX', language: 'c')
    add_project_arguments('-DCLOD_PATH_SEP=\'/\'', language: 'c')

elif host_os == 'windows'
    add_project_arguments('-DCLOD_USE_WINDOWS', language: 'c')
    add_project_arguments('-DCLOD_PATH_SEP=\'\\\'', language: 'c')

elif host_os == 'darwin'
    add_project_arguments('-D_DARWIN_C_SOURCE', language: 'c')
    add_project_arguments('-DCLOD_USE_POSIX', language: 'c')
    add_project_arguments('-DCLOD_PATH_SEP=\'/\'', language: 'c')

endif

subdir('libclod')
subdir('cli')
