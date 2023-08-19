project('userspace-descriptors', 'c',
  default_options: [
    'c_std=c99',
    'warning_level=3',
    'default_library=static',
    'buildtype=release',
  ])

subdir('src')

if get_option('build-examples').enabled()
  subdir('examples')
endif
