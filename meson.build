project(
    'pywm',
    'c',
    version: '0.1.0',
    license: 'MIT',
    meson_version: '>=0.48.0',
    default_options : [
        'c_std=c11',
        'warning_level=2'
    ],
)

add_project_arguments(
	'-Wno-unused-parameter',
	language: 'c',
)

add_project_arguments(
	'-DWLR_USE_UNSTABLE',
	language: 'c',
)

if get_option('custom_renderer')
    add_project_arguments(
        '-DWM_CUSTOM_RENDERER',
        language: 'c',
    )
endif


# Dependencies

wlroots        = subproject('wlroots', default_options: ['default_library=static']).get_variable('wlroots')
# wlroots        = dependency('wlroots')
wayland_server = dependency('wayland-server')
wayland_client = dependency('wayland-client')
wayland_cursor = dependency('wayland-cursor')
wayland_egl    = dependency('wayland-egl')
wayland_protos = dependency('wayland-protocols', version: '>=1.14')
xkbcommon      = dependency('xkbcommon')
libinput       = dependency('libinput')
pixman         = dependency('pixman-1')
pthread        = meson.get_compiler('c').find_library('pthread')
math           = meson.get_compiler('c').find_library('m')


subdir('protocols')

if get_option('custom_renderer')
    subdir('src/wm/shaders')
endif


deps = [
    wlroots,
    wayland_server,
    server_protos,
    xkbcommon,
    libinput,
    pixman,
    pthread,
    math
]


sources = [
    'src/wm/wm.c',
    'src/wm/wm_server.c',
    'src/wm/wm_renderer.c',
    'src/wm/wm_seat.c',
    'src/wm/wm_keyboard.c',
    'src/wm/wm_pointer.c',
    'src/wm/wm_cursor.c',
    'src/wm/wm_layout.c',
    'src/wm/wm_output.c',
    'src/wm/wm_content.c',
    'src/wm/wm_view.c',
    'src/wm/wm_view_xdg.c',
    'src/wm/wm_view_layer.c',
    'src/wm/wm_view_xwayland.c',
    'src/wm/wm_widget.c',
    'src/wm/wm_config.c',
    'src/wm/wm_idle_inhibit.c',
    'src/wm/wm_drag.c',
]

if get_option('custom_renderer')
    sources += [
        texture_shaders_c,
        primitive_shaders_c
    ]
endif

py_sources = [
    'src/py/_pywmmodule.c',
    'src/py/_pywm_callbacks.c',
    'src/py/_pywm_view.c',
    'src/py/_pywm_widget.c'
]

incs = include_directories('include')


executable(
	'pywm',
	sources + ['src/main.c'],
    include_directories: incs,
	dependencies: deps,
)

python = import('python').find_installation('python3')

python.extension_module(
    '_pywm',
    sources + py_sources,
    include_directories: incs,
    dependencies: deps + [python.dependency()],
)
