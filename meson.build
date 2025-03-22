project('chip8', 'c',
  version : '0.2',
  default_options : ['warning_level=3', 'c_std=c17'])

cc = meson.get_compiler('c')
incdir = include_directories('include')
sdl2_dep = dependency('sdl3')
m_dep = cc.find_library('m', required: false)

sources = [
  'src/main.c',
  'src/chip8.c',
]

executable('chip8', sources: sources, dependencies: [sdl2_dep, m_dep], include_directories: incdir)
